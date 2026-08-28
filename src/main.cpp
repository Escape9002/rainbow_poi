#include <Arduino.h>
#include "mpu9250.h"
#include "esp_sleep.h"
#include <driver/gpio.h>
#include "BLE-HAL.h"
#include <BLEInterface.h>


#include <FastLED.h>

// Add this build flag for the real colorimetric solver:
//   -DFASTLED_RGBW_COLORIMETRIC=1
// Without it, kRGBWColorimetric compiles and falls back to kRGBWExactColors.

// How many leds in your strip?
#define NUM_LEDS 15

// For led chips like WS2812, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
// Clock pin only needed for SPI based chipsets when not using hardware SPI
#define DATA_PIN 2

// Define the array of leds
CRGB leds[NUM_LEDS];

// Seeed XIAO ESP32-C3 Hardware Mapping
#define imu_INT_PIN 4
            // D2 is GPIO 4
#define NO_MOTION_TIMEOUT_MS 10000 // 10 seconds (gives you time to see Serial)

unsigned long lastMotionTime = 0;

/* Mpu9250 object, I2C bus,  0x68 address */
bfs::Mpu9250 imu(&Wire, bfs::Mpu9250::I2C_ADDR_PRIM);


RTC_DATA_ATTR float MAX_ACCL_MSS = 18;

#define BLE_DEVICE_NAME "POI"
#define BLE_SERVICE_UUID "c5b6fc84-1450-4f82-83c7-ef4dc0e948de"
#define BLE_CHAR_UUID "309d5cfd-4ad1-45f6-81c8-fd6f512ae200"
BLEInterface * ble_driver = nullptr;

// Function to acknowledge/clear the MPU9250 interrupt hardware pin
void clearimuInterrupt()
{
  Wire.beginTransmission(0x68);
  Wire.write(0x3A); // Read INT_STATUS register
  Wire.endTransmission();
  Wire.requestFrom(0x68, 1);
  while (Wire.available())
    Wire.read();
}

void enterDeepSleep()
{
  Serial.println("Preparing to sleep...");
  Serial.println("Configuring WOM and sleeping...");

  // Switch imu to Low Power WOM Mode
  imu.EnableWom(40, bfs::Mpu9250::WOM_RATE_250HZ);
  // Latch interrupt and clear on any read
  Wire.beginTransmission(0x68);
  Wire.write(0x37);
  Wire.write(0x30);
  Wire.endTransmission();


  // 1. Clear any existing interrupt before sleeping
  clearimuInterrupt();
  delay(50);

  // 2. Configure Wakeup
  // Level-triggered: if the pin is HIGH, the chip wakes up.
  esp_deep_sleep_enable_gpio_wakeup(1ULL << imu_INT_PIN, ESP_GPIO_WAKEUP_GPIO_HIGH);

  Serial.println("Entering Deep Sleep now. LED is OFF.");
  Serial.flush();

  // Ensure LED is OFF before sleeping (HIGH = OFF on XIAO C3)

  esp_deep_sleep_start();
}

void setup()
{
  // --- LED STARTUP INDICATION ---

  pinMode(imu_INT_PIN, INPUT_PULLDOWN);
  
  ble_driver = &getBLEDriverInstance();

  ble_driver->begin(BLE_DEVICE_NAME, BLE_SERVICE_UUID, BLE_CHAR_UUID);

  Serial.begin(115200);

  Serial.println("\n*************************");
  Serial.println("XIAO ESP32-C3 IS AWAKE");

  esp_sleep_wakeup_cause_t wakeReason = esp_sleep_get_wakeup_cause();
  if (wakeReason == ESP_SLEEP_WAKEUP_GPIO)
  {
    Serial.println("Wakeup Source: Motion Detected!");
  }
  else
  {
    Serial.println("Wakeup Source: Power On / Reset");
  }

  Wire.begin(6,5);
  Wire.setClock(400000);
  imu.Config(&Wire, bfs::Mpu9250::I2C_ADDR_PRIM);
  while (!imu.Begin())
  {
    Serial.println("imu Initialization FAILED!");
    delay(100);
  }

  /* Set the sample rate divider */
  while (!imu.ConfigSrd(19)) {
    Serial.println("Error configured SRD");
    delay(100);
  }
  
  lastMotionTime = millis();

  pinMode(8,OUTPUT);

   FastLED.addLeds<WS2811, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(  50 );
    
}

void loop()
{
  digitalWrite(8, LOW);

  for(int i = 0; i <= NUM_LEDS; i++) {
    leds[i] = CRGB::LightBlue;
  }
  FastLED.show();

  if(imu.Read()){

     // 2. Calculate Magnitude: sqrt(x^2 + y^2 + z^2)
  float ax = imu.accel_x_mps2();
  float ay = imu.accel_y_mps2();
  float az = imu.accel_z_mps2();

  float magnitude = sqrt(ax * ax + ay * ay + az * az);
  float motionForce = abs(magnitude - 9.81);
  float sensitivity_gain = 360/MAX_ACCL_MSS;
  float hue = map(constrain(motionForce*sensitivity_gain, 0, 360), 0, 360, 240, 0);

  unsigned long timeSinceMotion = millis() - lastMotionTime;
  float brightness = map(constrain(timeSinceMotion, 0, NO_MOTION_TIMEOUT_MS), 0, NO_MOTION_TIMEOUT_MS, 100, 0);

  if (timeSinceMotion < NO_MOTION_TIMEOUT_MS)
  {
    }
  else
  {
    enterDeepSleep();
  }

  if (motionForce > 0.5f || digitalRead(imu_INT_PIN) == HIGH)
  {
    lastMotionTime = millis();
    clearimuInterrupt();
    
      float temperature = temperatureRead();
    Serial.print(millis());
    Serial.print("\t");
    Serial.print("ESP32 temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
  }

  if(ble_driver->connected()){
        float temp = imu.die_temp_c();
        char buff[5];
        snprintf (buff, sizeof(buff), "%f", temp);
        ble_driver->sendDataPacket(&buff, sizeof(buff));
        if(ble_driver->available()){
          String msg = ble_driver->get_received();
          MAX_ACCL_MSS = msg.toFloat();
        }
    }

  delay(20);
  }



 
}