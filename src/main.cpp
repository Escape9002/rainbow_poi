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

#define VOLTAGE_DIVIDER_FACTOR 2
#define BATTERY_MEASUREMENT_PIN 3
const uint16_t CHARGE_CUTOFF_V = 4200;    // mV
const uint16_t DISCHARGE_CUTOFF_V = 3000; // mV

// ============================================================
// MPU9250
// ============================================================
/* Mpu9250 object, I2C bus,  0x68 address */
bfs::Mpu9250 imu(&Wire, bfs::Mpu9250::I2C_ADDR_PRIM);

// ============================================================
// BLE
// ============================================================
#define BLE 1
#if BLE
#include <BLEServer.h>
#include <BleEndpoint.h>
#include "FastLEDEffects.h"

#define BLE_DEVICE_NAME "POI"

#define BLE_SERVICE_UUID \
    "c5b6fc84-1450-4f82-83c7-ef4dc0e948de"

#define ALPHA_UUID \
    "309d5cfd-4ad1-45f6-81c8-fd6f512ae200"
#define BATTERY_CHAR_UUID "2A19" // Standard BLE Battery Level UUID
#define HUE_MIN_UUID \
    "f79431f4-255a-400e-a9d4-63c0d5be5e2a"
#define HUE_MAX_UUID \
    "f789580d-1fd5-4579-bf3f-18db5adc6b3e"

// 1. Create the server instance
BleServer bleServer(BLE_SERVICE_UUID);
// 2. Create our custom typed endpoint (initial value: 80)
BleEndpoint<uint32_t> endpointAlpha(ALPHA_UUID, 80, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
BleEndpoint<uint16_t> endpointHueMin(HUE_MIN_UUID, 260, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
BleEndpoint<uint16_t> endpointHueMax(HUE_MAX_UUID, 359, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);

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

#define LED_UPDATE_MS 10UL

uint32_t lastLedUpdate = 0;

FastLEDEffects realEffectEngine;

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
PoiController poi_controller = PoiController(MAX_ACCL_MSS, FP_SCALE, 80, 240, 359, true, &realEffectEngine);

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
    // GPIO
    // --------------------------------------------------------

    pinMode(IMU_INT_PIN, INPUT_PULLDOWN);

    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(A3, INPUT);

    // Status LED on
    digitalWrite(STATUS_LED_PIN, LOW);

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

    bleServer.begin(BLE_DEVICE_NAME, {&endpointAlpha, &endpointHueMin, &endpointHueMax, &endpointBattery});

    // digital capacitor :3
    delay(250);

    // ble_driver->begin(
    //     BLE_DEVICE_NAME,
    //     BLE_SERVICE_UUID,
    //     BLE_CHAR_UUID);

#endif

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
    // srd should be conform with update_led_ms.

    // MPU9250:
    //     rate [Hz] = 1000 / (SRD + 1)
    //
    // Desired:
    //     sample period [ms] = LED_UPDATE_MS
    //
    // Therefore:
    //     rate [Hz] = 1000 / LED_UPDATE_MS
    //     SRD       = 1000 / LED_UPDATE_MS - 1

    static_assert(LED_UPDATE_MS > 0, "LED_UPDATE_MS must not be 0");

    const uint8_t SRD = (1000 / LED_UPDATE_MS) - 1;

    while (!imu.ConfigSrd(SRD))
    {
        Serial.println("Error configuring SRD");
        delay(100);
    }

    while (!imu.ConfigAccelRange(bfs::Mpu9250::ACCEL_RANGE_16G))
    {
        Serial.println("Error configuring ACCL_Range");
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

    // lessen LED-Flicker (https://github.com/FastLED/FastLED/wiki/FastLED-Temporal-Dithering)
    FastLED.setDither(DISABLE_DITHER);

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
            uint32_t dt_ms = now - lastLedUpdate;
            lastLedUpdate = now;

            HSV hsv = poi_controller.tick(acceleration, dt_ms);

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
            // enterDeepSleep();
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

        uint16_t newHueMin =  endpointHueMin.getValue();
        if (newHueMin != poi_controller.getHueMin())
        {
            poi_controller.setColorRange(newHueMin, poi_controller.getHueMax());
        }

        uint16_t newHueMax =  endpointHueMax.getValue();
        if (newHueMax != poi_controller.getHueMax())
        {
            poi_controller.setColorRange(poi_controller.getHueMin(), newHueMax);
        }
    }

    static uint32_t lastUpdate = 0;
    if (millis() - lastUpdate > 5000)
    {
        lastUpdate = millis();

        uint16_t volt = analogReadMilliVolts(BATTERY_MEASUREMENT_PIN) * VOLTAGE_DIVIDER_FACTOR;

        // 1. Clamp the voltage to our known bounds to prevent math errors
        if (volt > CHARGE_CUTOFF_V)
            volt = CHARGE_CUTOFF_V;
        if (volt < DISCHARGE_CUTOFF_V)
            volt = DISCHARGE_CUTOFF_V;

        uint16_t chargePercentage = ((volt - DISCHARGE_CUTOFF_V) * 100) / (CHARGE_CUTOFF_V - DISCHARGE_CUTOFF_V);

        Serial.print(volt);
        Serial.print("\t");
        Serial.println(chargePercentage);

        // Pusht den neuen Wert per Notify direkt auf das Handy!
        endpointBattery.setValue(chargePercentage);

        poi_controller.setBatteryLevel(chargePercentage);
    }
#endif
}
