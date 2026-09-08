#include <HAL.h>
#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <vector>
#include "mpu9250.h"
#include "esp_sleep.h"
#include <driver/gpio.h>
#include "FastLED.h"

class ESP32C3SuperMini : public HAL
{

private:
    bfs::Mpu9250 *imu;
    const uint8_t IMU_INT_PIN;

public:
    ESP32C3SuperMini(bfs::Mpu9250 *imu,
                     uint8_t imu_int_pin)
        : imu(imu),
          IMU_INT_PIN(imu_int_pin)
    {
    }

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

    void enterDeepSleep() override
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
        imu->EnableWom(1020, bfs::Mpu9250::WOM_RATE_15_63HZ);

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
        esp_err_t err_gpio =
            esp_deep_sleep_enable_gpio_wakeup(
                1ULL << IMU_INT_PIN,
                ESP_GPIO_WAKEUP_GPIO_HIGH);

        if (err_gpio != ESP_OK)
        {
            Serial.print("Failed to configure GPIO wakeup: ");
            Serial.println(err_gpio);
            return;
        }

        // --------------------------------------------------------
        // Configure ESP32-C3 TIMER wakeup
        // --------------------------------------------------------

        // 1 day = 24 hours * 60 mins * 60 secs * 1,000,000 microseconds
        const uint64_t SLEEP_TIME_MS = 86400ULL * 1000000ULL;

        esp_err_t err_timer = esp_sleep_enable_timer_wakeup(SLEEP_TIME_MS);

        if (err_timer != ESP_OK)
        {
            Serial.print("Failed to configure timer wakeup: ");
            Serial.println(err_timer);
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
};