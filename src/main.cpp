#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#include "mpu9250.h"
#include "esp_sleep.h"
#include <driver/gpio.h>

#include "BLE-HAL.h"
#include <BLEInterface.h>

#include <FastLED.h>

#include <HSV.h>

// ============================================================
// HARDWARE CONFIGURATION
// ============================================================

// ESP32-C3 SuperMini
#define IMU_SDA_PIN 6
#define IMU_SCL_PIN 5
#define IMU_INT_PIN 4

// FastLED
#define NUM_LEDS 15
#define DATA_PIN 2
#define LED_BRIGHTNESS 50

// Status LED
#define STATUS_LED_PIN 8

// ============================================================
// MPU9250
// ============================================================
/* Mpu9250 object, I2C bus,  0x68 address */
bfs::Mpu9250 imu(&Wire, bfs::Mpu9250::I2C_ADDR_PRIM);

// ============================================================
// BLE
// ============================================================

#define BLE_DEVICE_NAME "POI"

#define BLE_SERVICE_UUID \
    "c5b6fc84-1450-4f82-83c7-ef4dc0e948de"

#define BLE_CHAR_UUID \
    "309d5cfd-4ad1-45f6-81c8-fd6f512ae200"

BLEInterface *ble_driver = nullptr;

// ============================================================
// FASTLED
// ============================================================

CRGB leds[NUM_LEDS];

// ============================================================
// FIXED POINT CONFIGURATION
// ============================================================
//
// All acceleration values use:
//
//      real value * 1000
//
// Example:
//
//      0.5 m/s^2  -> 500
//      1.0 m/s^2  -> 1000
//      9.81 m/s^2 -> 9810
//      18 m/s^2   -> 18000
//
// This gives us 3 decimal places.
// ============================================================

#define FP_SCALE 1000UL

// ============================================================
// PHYSICAL CONSTANTS
// ============================================================

// Gravity
#define GRAVITY_MSS 9810UL

// Ignore acceleration below 0.5 m/s^2
#define DEADZONE_MSS 1000UL

// Minimum acceleration required to reset sleep timer
#define MINIMUM_ACCL_MSS DEADZONE_MSS

// ============================================================
// FILTER
// ============================================================
//
// Alpha:
//
//     0.08 = 80 / 1000
//
// Lower = smoother
// Higher = faster
//
// ============================================================

#define FILTER_SCALE 1000UL

RTC_DATA_ATTR uint32_t FILTER_ALPHA = 80;

// ============================================================
// HSV CONVERSION RANGE
// ============================================================
#define NORMALIZED_SCALE 1000UL

// ============================================================
// AUTOMATIC ACCELERATION RANGE
// ============================================================
//
// Initial value:
//     18 m/s^2
// ============================================================

uint32_t MAX_ACCL_MSS = 18000;

// ============================================================
// SLEEP CONFIGURATION
// ============================================================

#define NO_MOTION_TIMEOUT_MS 10000UL

uint32_t lastMotionTime = 0;

// ============================================================
// FILTER STATE
// ============================================================

uint32_t filteredAcceleration = 0;

// ============================================================
// LED UPDATE
// ============================================================

#define LED_UPDATE_MS 20UL

uint32_t lastLedUpdate = 0;

// ============================================================
// CLEAR MPU9250 INTERRUPT
// ============================================================
// Function to acknowledge/clear the MPU9250 interrupt hardware pin
void clearimuInterrupt()
{
    Wire.beginTransmission(bfs::Mpu9250::I2C_ADDR_PRIM);
    Wire.write(0x3A); // Read INT_STATUS register
    Wire.endTransmission();
    Wire.requestFrom(bfs::Mpu9250::I2C_ADDR_PRIM, 1);
    while (Wire.available())
        Wire.read();
}

// ============================================================
// ENTER DEEP SLEEP
// ============================================================

void enterDeepSleep()
{
    Serial.println();
    Serial.println("==============================");
    Serial.println("Preparing for Deep Sleep");
    Serial.println("==============================");

    // --------------------------------------------------------
    // Turn LEDs off
    // --------------------------------------------------------

    FastLED.clear();
    FastLED.show();

    // --------------------------------------------------------
    // Enable MPU9250 Wake-on-Motion
    // --------------------------------------------------------
    Serial.println("Enabling MPU9250 WOM...");

    // Switch imu to Low Power WOM Mode
    // threshold in mg, also, theres a max-value, as per code:
    // > /* Check threshold in limits, 4 - 1020 mg */
    // WOM Rate is more or less sensitivity!
    imu.EnableWom(1020, bfs::Mpu9250::WOM_RATE_15_63HZ);

    // --------------------------------------------------------
    // Configure MPU9250 interrupt
    //
    // INT_PIN_CFG = 0x37
    // 0x30 = latch interrupt + clear on any read
    // --------------------------------------------------------

    // Latch interrupt and clear on any read
    Wire.beginTransmission(bfs::Mpu9250::I2C_ADDR_PRIM);
    Wire.write(0x37);
    Wire.write(0x30);
    Wire.endTransmission();

    // --------------------------------------------------------
    // Clear any previous interrupt
    // --------------------------------------------------------

    clearimuInterrupt();
    delay(50);

    // --------------------------------------------------------
    // Configure ESP32-C3 GPIO wakeup
    //
    // Wake when IMU_INT_PIN goes HIGH.
    // --------------------------------------------------------
    esp_err_t err =
        esp_deep_sleep_enable_gpio_wakeup(
            1ULL << IMU_INT_PIN,
            ESP_GPIO_WAKEUP_GPIO_HIGH);

    if (err != ESP_OK)
    {
        Serial.print("Failed to configure GPIO wakeup: ");
        Serial.println(err);
        return;
    }

    // --------------------------------------------------------
    // Sleep
    // --------------------------------------------------------

    Serial.println("GPIO wakeup configured.");

    Serial.print("Waiting for motion on GPIO ");
    Serial.println(IMU_INT_PIN);

    Serial.println("Entering Deep Sleep...");

    Serial.flush();

    esp_deep_sleep_start();
}

// ============================================================
// GET ACCELERATION MAGNITUDE
// ============================================================
//
// Returns dynamic acceleration in:
//
//     milli m/s^2
//
// Example:
//
//     0.5 m/s^2 -> 500
//     5.0 m/s^2 -> 5000
//
// Gravity is removed by:
//
//     abs(|a| - 9.81)
// ============================================================

uint32_t getAbsoluteAcceleration()
{
    // --------------------------------------------------------
    // Convert sensor floats into fixed-point integers
    // --------------------------------------------------------

    int32_t ax =
        (int32_t)(imu.accel_x_mps2() * FP_SCALE);

    int32_t ay =
        (int32_t)(imu.accel_y_mps2() * FP_SCALE);

    int32_t az =
        (int32_t)(imu.accel_z_mps2() * FP_SCALE);

    // --------------------------------------------------------
    // Calculate:
    //
    // sqrt(ax^2 + ay^2 + az^2)
    //
    // --------------------------------------------------------

    uint64_t sum = (int64_t)ax * ax + (int64_t)ay * ay + (int64_t)az * az;
    uint32_t magnitude = (uint32_t)sqrt((double)sum);

    // --------------------------------------------------------
    // Remove gravity
    // --------------------------------------------------------

    uint32_t movement;

    if (magnitude > GRAVITY_MSS)
    {
        movement =
            magnitude - GRAVITY_MSS;
    }
    else
    {
        movement =
            GRAVITY_MSS - magnitude;
    }

    return movement;
}

// ============================================================
// LOW PASS FILTER
// ============================================================
//
//
// filtered += alpha * (input - filtered)
//
// alpha is represented as:
//
//     80 / 1000 = 0.08
//
// ============================================================

uint32_t filterAcceleration(
    uint32_t acceleration)
{
    int32_t error =
        (int32_t)acceleration -
        (int32_t)filteredAcceleration;

    // error * 0.08 = error * 80/1000
    // also known as: error * alpha / FilterScale
    int32_t correction =
        ((int32_t)FILTER_ALPHA * error) / (int32_t)FILTER_SCALE;

    int32_t result =
        (int32_t)filteredAcceleration +
        correction;

    if (result < 0)
    {
        result = 0;
    }

    filteredAcceleration =
        (uint32_t)result;

    return filteredAcceleration;
}

// ============================================================
// ACCELERATION -> COLOR
// ============================================================
//
// Blue  -> low acceleration
// Purple -> medium acceleration
// Red   -> high acceleration
//
// FastLED hue:
//     160 = blue
//     192 = purple
//     255 = red
//
// ============================================================

CRGB accelerationToColor(
    uint32_t acceleration)
{
    // --------------------------------------------------------
    // Deadzone
    // --------------------------------------------------------

    if (acceleration < DEADZONE_MSS)
    {
        acceleration = 0;
    }

    // --------------------------------------------------------
    // Update maximum
    // --------------------------------------------------------

    if (acceleration > MAX_ACCL_MSS)
    {
        MAX_ACCL_MSS = acceleration;
    }
    else if (MAX_ACCL_MSS > 18000)
    {
        // only decrease if we are above baseline
        MAX_ACCL_MSS -= 10;
    }

    // --------------------------------------------------------
    // Avoid division by zero
    // --------------------------------------------------------

    if (MAX_ACCL_MSS == 0)
    {
        return CRGB::Blue;
    }

    // --------------------------------------------------------
    // Normalize:
    //
    // 0 -> 1000
    //
    // 0 = minimum
    // 1000 = maximum
    // --------------------------------------------------------

    uint32_t normalized =
        ((uint64_t)acceleration * NORMALIZED_SCALE) / MAX_ACCL_MSS;

    if (normalized > 1000)
    {
        normalized = 1000;
    }

    // --------------------------------------------------------
    // Hue:
    //
    // 160 = blue
    // 255 = red
    //
    // 160 + 95 = 255
    // --------------------------------------------------------

    // uint8_t hue =
    //     160 - ((normalized * 160UL) / NORMALIZED_SCALE);

    HSV hsv = HSV();
    hsv.hueMapper(255, 160, normalized);

    return CHSV(
        hsv.hue,
        255,
        255);
}

// ============================================================
// UPDATE LEDS
// ============================================================

void updateLEDs(
    uint32_t acceleration)
{
    CRGB color =
        accelerationToColor(
            acceleration);

    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
        leds[i] = color;
    }

    FastLED.show();
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
    // --------------------------------------------------------
    // Serial
    // --------------------------------------------------------

    Serial.begin(115200);

    delay(100);

    Serial.println();
    Serial.println("==============================");
    Serial.println("ESP32-C3 POI");
    Serial.println("==============================");

    setCpuFrequencyMhz(80);

    // --------------------------------------------------------
    // Determine wake reason
    // --------------------------------------------------------

    esp_sleep_wakeup_cause_t wakeReason =
        esp_sleep_get_wakeup_cause();

    if (wakeReason == ESP_SLEEP_WAKEUP_GPIO)
    {
        Serial.println(
            "Wakeup: MOTION");
    }
    else
    {
        Serial.println(
            "Wakeup: POWER ON / RESET");
    }

    // --------------------------------------------------------
    // GPIO
    // --------------------------------------------------------

    pinMode(
        IMU_INT_PIN,
        INPUT_PULLDOWN);

    pinMode(
        STATUS_LED_PIN,
        OUTPUT);

    // Status LED on
    digitalWrite(
        STATUS_LED_PIN,
        LOW);

    // --------------------------------------------------------
    // MPU9250
    // --------------------------------------------------------
    Wire.begin(
        IMU_SDA_PIN,
        IMU_SCL_PIN);

    Wire.setClock(400000);

    Serial.println(
        "Initializing MPU9250...");

    imu.Config(
        &Wire,
        bfs::Mpu9250::I2C_ADDR_PRIM);

    while (!imu.Begin())
    {
        Serial.println(
            "MPU9250 initialization FAILED!");

        delay(500);
    }

    Serial.println(
        "MPU9250 initialized.");

    // --------------------------------------------------------
    // Sample rate
    // --------------------------------------------------------

    while (!imu.ConfigSrd(19))
    {
        Serial.println(
            "Error configuring SRD");

        delay(100);
    }

    // --------------------------------------------------------
    // BLE
    // --------------------------------------------------------

    Serial.println(
        "Starting BLE...");

    ble_driver =
        &getBLEDriverInstance();

    ble_driver->begin(
        BLE_DEVICE_NAME,
        BLE_SERVICE_UUID,
        BLE_CHAR_UUID);

    // --------------------------------------------------------
    // FastLED
    // --------------------------------------------------------

    FastLED.addLeds<
               WS2812B,
               DATA_PIN,
               GRB>(
               leds,
               NUM_LEDS)
        .setCorrection(
            TypicalLEDStrip);

    FastLED.setBrightness(
        LED_BRIGHTNESS);

    FastLED.clear();
    FastLED.show();

    // --------------------------------------------------------
    // Reset runtime timers
    // --------------------------------------------------------

    lastMotionTime =
        millis();

    lastLedUpdate =
        millis();

    Serial.println(
        "Setup complete.");

    Serial.println();
}

void loop()
{
    // --------------------------------------------------------
    // Status LED ON
    // --------------------------------------------------------

    digitalWrite(
        STATUS_LED_PIN,
        LOW);

    // --------------------------------------------------------
    // Read IMU
    // --------------------------------------------------------

    if (imu.Read())
    {
        // ----------------------------------------------------
        // Acceleration
        // ----------------------------------------------------

        uint32_t acceleration =
            getAbsoluteAcceleration();

        // ----------------------------------------------------
        // Filter
        // ----------------------------------------------------

        uint32_t filtered =
            filterAcceleration(
                acceleration);

        // ----------------------------------------------------
        // LED update
        // ----------------------------------------------------

        uint32_t now =
            millis();

        if (
            now - lastLedUpdate >= LED_UPDATE_MS)
        {
            lastLedUpdate =
                now;

            updateLEDs(
                filtered);
        }

        // ----------------------------------------------------
        // Motion detection
        // ----------------------------------------------------

        if (
            acceleration >=
            MINIMUM_ACCL_MSS)
        {
            lastMotionTime =
                millis();
        }

        // ----------------------------------------------------
        // Deep Sleep
        // ----------------------------------------------------

        if (
            millis() - lastMotionTime >= NO_MOTION_TIMEOUT_MS)
        {
            enterDeepSleep();
        }
    }

    // ========================================================
    // BLE
    // ========================================================

    if (
        ble_driver != nullptr &&
        ble_driver->connected())
    {
        // ----------------------------------------------------
        // MPU9250 temperature
        //
        // Send as normal float string, no computation done on value
        // ----------------------------------------------------

        float mpuTemp =
            imu.die_temp_c();

        char buff[16];

        snprintf(
            buff,
            sizeof(buff),
            "%.2f",
            mpuTemp);

        ble_driver->sendDataPacket(
            &buff,
            strlen(buff) + 1);

        // ----------------------------------------------------
        // Receive new filter setting
        // Expected values are: [0, 1000]
        // ----------------------------------------------------

        if (
            ble_driver->available())
        {
            String msg =
                ble_driver->get_received();

            uint32_t alpha =
                msg.toInt();

            // ------------------------------------------------
            // Limit to sensible range
            // ------------------------------------------------

            if (alpha < 1)
            {
                alpha = 1;
            }
            else if (alpha > 1000)
            {
                alpha = 1000;
            }

            FILTER_ALPHA =
                alpha;

            Serial.print(
                "New FILTER_ALPHA: ");

            Serial.println(
                FILTER_ALPHA);
        }
    }

    // ========================================================
    // ESP32-C3 TEMPERATURE
    // ========================================================

    static uint32_t lastTempPrint = 0;

    if (
        millis() - lastTempPrint >= 2000)
    {
        lastTempPrint = millis();

        float espTemp = temperatureRead();

        Serial.print("ESP32-C3 temp: ");
        Serial.print(espTemp, 2);
        Serial.println("°C");

        Serial.print("MPU9250 temp: ");
        Serial.print(imu.die_temp_c(), 2);
        Serial.println("°C");

        Serial.print("absAccl: ");
        Serial.print(getAbsoluteAcceleration());

        Serial.print("\tfilter: ");
        Serial.print(filteredAcceleration);

        Serial.print("\tmax: ");
        Serial.println(MAX_ACCL_MSS);
    }

    // --------------------------------------------------------
    // Small delay
    // --------------------------------------------------------

    delay(20);
}
