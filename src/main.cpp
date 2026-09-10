#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <vector>
#include "mpu9250.h"
#include "esp_sleep.h"
#include <driver/gpio.h>

#include <FastLED.h>

#include <HSV.h>
#include <LowPass.h>
#include <ESP32C3SuperMini.h>

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
#define LED_BRIGHTNESS 255

// Status LED
#define STATUS_LED_PIN 8

#define VOLTAGE_DIVIDER_FACTOR 2
#define BATTERY_MEASUREMENT_PIN 3
const uint16_t CHARGE_CUTOFF_V = 4200;    // mV
const uint16_t DISCHARGE_CUTOFF_V = 3230; // mV
uint16_t chargePercentage = 100;

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
#define CONTROLLER_MODE_UUID \
    "bbe6c883-f669-4fa8-b110-808feb345e75"
// 1. Create the server instance
BleServer bleServer(BLE_SERVICE_UUID);
// 2. Create our custom typed endpoint (initial value: 80)
BleEndpoint<uint32_t> endpointAlpha(ALPHA_UUID, "filter_alpha", 80, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
BleEndpoint<uint16_t> endpointHueMin(HUE_MIN_UUID, "hueMin", 260, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
BleEndpoint<uint16_t> endpointHueMax(HUE_MAX_UUID, "hueMax", 359, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
BleEndpoint<std::string> endpointCntrlMde(CONTROLLER_MODE_UUID, "CntrlMde", "GYRO", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);

// Typ: uint8_t | Startwert: 100% | Rechte: Lesen & Benachrichtigen (Kein Schreiben vom Handy!)
BleEndpoint<uint8_t> endpointBattery(
    BATTERY_CHAR_UUID,
    "BatteryLvl",
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

uint32_t MAX_GYRO_RADS = 200;

// ============================================================
// LED UPDATE
// ============================================================

#define LED_UPDATE_MS 5UL

uint32_t lastLedUpdate = 0;

FastLEDEffects realEffectEngine;

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

uint32_t getAbsoluteRadS()
{
    int32_t ax =
        (int32_t)(imu.gyro_x_radps() * FP_SCALE);

    int32_t ay =
        (int32_t)(imu.gyro_y_radps() * FP_SCALE);

    int32_t az =
        (int32_t)(imu.gyro_z_radps() * FP_SCALE);

    // --------------------------------------------------------
    // Calculate:
    //
    // sqrt(ax^2 + ay^2 + az^2)
    //
    // --------------------------------------------------------

    uint64_t sum = (int64_t)ax * ax + (int64_t)ay * ay + (int64_t)az * az;
    uint32_t magnitude = (uint32_t)sqrt((double)sum);

    return magnitude;
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
// GET BATTTERY PERCENTAGE
// ============================================================

uint8_t getBatteryPercentage()
{

    uint16_t volt = analogReadMilliVolts(BATTERY_MEASUREMENT_PIN) * VOLTAGE_DIVIDER_FACTOR;

    // 1. Clamp the voltage to our known bounds to prevent math errors
    if (volt > CHARGE_CUTOFF_V)
        volt = CHARGE_CUTOFF_V;
    if (volt < DISCHARGE_CUTOFF_V)
        volt = DISCHARGE_CUTOFF_V;

    return static_cast<uint8_t>(((volt - DISCHARGE_CUTOFF_V) * 100) / (CHARGE_CUTOFF_V - DISCHARGE_CUTOFF_V));
}

// ============================================================
// POI_CONTROLLER
// ============================================================
#include <PoiController.h>
ESP32C3SuperMini esp32_c3_superMini = ESP32C3SuperMini(&imu, IMU_INT_PIN);
PoiController poi_controller = PoiController(
    realEffectEngine,
    esp32_c3_superMini);

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
    // Check battery levels and report to controller
    // --------------------------------------------------------
    // the controller must do a tick to update its hardware states!
    // otherwise the default values persist!
    poi_controller.setBatteryLevel(getBatteryPercentage());
    poi_controller.hardwareTick(0, 0);

    // --------------------------------------------------------
    // Determine wake reason
    // --------------------------------------------------------

    switch (esp_sleep_get_wakeup_cause())
    {
    case ESP_SLEEP_WAKEUP_GPIO:
        Serial.println("Wakeup: Motion");
        break;

    case ESP_SLEEP_WAKEUP_TIMER:

        // we should check the battery and return to sleep if the charge is high enough.
        // otherwise we should start flashing red.
        // this should be handled by the battery check at the start of the setup function

        break;

    default:
        Serial.println("Wakeup: POWER ON / RESET | DEFAULT");

        break;
    }

    // --------------------------------------------------------
    // BLE
    // --------------------------------------------------------
#if BLE
    if (
        poi_controller.getHardwareState() != HARDWARE_STATE::LOW_BATTERY && poi_controller.getHardwareState() != HARDWARE_STATE::SLEEP)
    {

        Serial.println("Starting BLE...");

        bleServer.begin(BLE_DEVICE_NAME, {&endpointAlpha,
                                          &endpointHueMin,
                                          &endpointHueMax,
                                          &endpointBattery,
                                          &endpointCntrlMde});

        // digital capacitor :3
        delay(250);
    }

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

    const uint8_t SRD = (LED_UPDATE_MS)-1;

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

    lastLedUpdate = millis();

    Serial.println("Setup complete.");
}

void loop()
{
    digitalWrite(STATUS_LED_PIN, LOW);

    static uint32_t latest_accl = 0;
    static uint32_t latest_gyro = 0;

    // --------------------------------------------------------
    // Read IMU
    // --------------------------------------------------------
    if (imu.Read())
    {
        latest_accl = getAbsoluteAcceleration();
        latest_gyro = getAbsoluteRadS();
    }

    // ----------------------------------------------------
    // LED update
    // ----------------------------------------------------

    uint32_t now = millis();
    if (
        now - lastLedUpdate >= LED_UPDATE_MS)
    {

        uint32_t dt_ms = now - lastLedUpdate;
        lastLedUpdate = now;

        // Figure out which sensor value the controller cares about right now
        // (If the mode is LOW_BATTERY or CONSTANT, the controller ignores this value anyway)
        uint32_t sensor_value = (poi_controller.getAnimationState() == ANIMATION_STATE::GYRO)
                                    ? latest_gyro
                                    : latest_accl;

        HSV hsv = poi_controller.tick(sensor_value, dt_ms);
        updateLEDs(hsv);
    }

    // ========================================================
    // BATTERY POWER
    // ========================================================

    static uint32_t lastUpdate = 0;
    if (millis() - lastUpdate > 5000)
    {
        lastUpdate = millis();

        poi_controller.setBatteryLevel(getBatteryPercentage());
    }

    // ========================================================
    // BLE
    // ========================================================

#if BLE
    if (poi_controller.getHardwareState() != HARDWARE_STATE::LOW_BATTERY && poi_controller.getHardwareState() != HARDWARE_STATE::SLEEP)
    {
        // Pusht den neuen Wert per Notify direkt auf das Handy!
        if (chargePercentage != endpointBattery.getValue())
        {
            endpointBattery.setValue(chargePercentage);
        }

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

            uint16_t newHueMin = endpointHueMin.getValue();
            if (newHueMin != poi_controller.getHueMin())
            {
                poi_controller.setColorRange(newHueMin, poi_controller.getHueMax());
            }

            uint16_t newHueMax = endpointHueMax.getValue();
            if (newHueMax != poi_controller.getHueMax())
            {
                poi_controller.setColorRange(poi_controller.getHueMin(), newHueMax);
            }

            poi_controller.setAnimationState(
                animationStringToState(
                    endpointCntrlMde.getValue().c_str()));

            endpointCntrlMde.setValue(toString(poi_controller.getAnimationState()));
        }
    }

#endif
}
