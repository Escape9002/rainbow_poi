#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <vector>
#include "mpu9250.h"
#include "esp_sleep.h"
#include <driver/gpio.h>

// #include "BLE-HAL.h"
// #include <BLEInterface.h>

#include <FastLED.h>

#include <HSV.h>
#include <LowPass.h>

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
#define LED_BRIGHTNESS 100

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
#define BLE 0
#if BLE
#include <BLEServer.h>
#include <BleEndpoint.h>

#define BLE_DEVICE_NAME "POI"

#define BLE_SERVICE_UUID \
    "c5b6fc84-1450-4f82-83c7-ef4dc0e948de"

#define BLE_CHAR_UUID \
    "309d5cfd-4ad1-45f6-81c8-fd6f512ae200"
#define BATTERY_CHAR_UUID "2A19" // Standard BLE Battery Level UUID

// 1. Create the server instance
BleServer bleServer(BLE_SERVICE_UUID);
// 2. Create our custom typed endpoint (initial value: 80)
BleEndpoint<uint32_t> endpointAlpha(BLE_CHAR_UUID, 80, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);

// Typ: uint8_t | Startwert: 100% | Rechte: Lesen & Benachrichtigen (Kein Schreiben vom Handy!)
BleEndpoint<uint8_t> endpointBattery(
    BATTERY_CHAR_UUID,
    100,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
#endif
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
// UPDATE LEDS
// ============================================================

void updateLEDs(
    HSV hsv)
{

    CRGB color = CHSV(
        hsv.hue,
        hsv.saturation,
        hsv.brightness);

    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
        leds[i] = color;
    }

    FastLED.show();
}

// ============================================================
// POI_CONTROLLER
// ============================================================
#include <PoiController.h>
PoiController poi_controller = PoiController(18000, 1000, 80, 160, 359, true);

// ============================================================
// SETUP
// ============================================================

void setup()
{
    // --------------------------------------------------------
    // Serial
    // --------------------------------------------------------

    Serial.begin(115200);

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
        Serial.println("Wakeup: MOTION");
    }
    else
    {
        Serial.println("Wakeup: POWER ON / RESET");
    }

    // --------------------------------------------------------
    // BLE
    // --------------------------------------------------------
#if BLE
    Serial.println("Starting BLE...");

    bleServer.begin(BLE_DEVICE_NAME, {&endpointAlpha, &endpointBattery});

    // ble_driver->begin(
    //     BLE_DEVICE_NAME,
    //     BLE_SERVICE_UUID,
    //     BLE_CHAR_UUID);

#endif
    // --------------------------------------------------------
    // GPIO
    // --------------------------------------------------------

    pinMode(IMU_INT_PIN, INPUT_PULLDOWN);

    pinMode(STATUS_LED_PIN, OUTPUT);

    // Status LED on
    digitalWrite(STATUS_LED_PIN, LOW);

    // --------------------------------------------------------
    // MPU9250
    // --------------------------------------------------------
    Wire.begin(
        IMU_SDA_PIN,
        IMU_SCL_PIN);

    Wire.setClock(400000);

    Serial.println("Initializing MPU9250...");

    imu.Config(
        &Wire,
        bfs::Mpu9250::I2C_ADDR_PRIM);

    while (!imu.Begin())
    {
        Serial.println("MPU9250 initialization FAILED!");

        delay(500);
    }

    Serial.println("MPU9250 initialized.");

    // --------------------------------------------------------
    // Sample rate
    // --------------------------------------------------------

    while (!imu.ConfigSrd(19))
    {
        Serial.println("Error configuring SRD");
        delay(100);
    }

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
    // Init runtime timers
    // --------------------------------------------------------

    lastMotionTime = millis();
    lastLedUpdate = millis();

    Serial.println("Setup complete.");
}

void loop()
{

    digitalWrite(STATUS_LED_PIN, LOW);

    // --------------------------------------------------------
    // Read IMU
    // --------------------------------------------------------

    if (imu.Read())
    {
        uint32_t acceleration = getAbsoluteAcceleration();

        // ----------------------------------------------------
        // LED update
        // ----------------------------------------------------

        uint32_t now = millis();

        if (
            now - lastLedUpdate >= LED_UPDATE_MS)
        {
            lastLedUpdate = now;

            HSV hsv = poi_controller.tick(acceleration);

            updateLEDs(hsv);
        }

        // ----------------------------------------------------
        // Motion detection
        // ----------------------------------------------------

        if (
            acceleration >= MINIMUM_ACCL_MSS)
        {
            lastMotionTime = millis();
        }

        // ----------------------------------------------------
        // Deep Sleep
        // ----------------------------------------------------

        if (millis() - lastMotionTime >= NO_MOTION_TIMEOUT_MS)
        {
            enterDeepSleep();
        }
    }

    // ========================================================
    // BLE
    // ========================================================
#if BLE
    if (!bleServer.update().empty())
    {
        // TODO changed endpoint receiver
        // 5. Read the value directly and safely
        uint32_t newAlpha = endpointAlpha.getValue();
        Serial.printf("[MAIN] Received Alpha: %d\n", newAlpha);

        if (newAlpha != poi_controller.getAlpha())
        {
            poi_controller.setAlpha(newAlpha);
            Serial.printf("[MAIN] Alpha updated to: %d\n", poi_controller.getAlpha());
        }
    }

    // Simuliere einen sinkenden Batteriestand alle 5 Sekunden
    static uint32_t lastUpdate = 0;
    if (millis() - lastUpdate > 5000)
    {
        lastUpdate = millis();

        uint8_t currentBattery = endpointBattery.getValue();
        if (currentBattery > 0)
        {
            currentBattery -= 1; // Akku verliert 1%

            // Pusht den neuen Wert per Notify direkt auf das Handy!
            endpointBattery.setValue(currentBattery);

            // Serial.printf("Batterie auf %d%% gesunken und gesendet!\n", currentBattery);
        }
    }
#endif
}
