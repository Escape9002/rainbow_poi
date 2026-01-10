#include <Arduino.h>
#include "MPU9250.h"
#include "esp_sleep.h"
#include <driver/gpio.h>
#include "BLE-HAL.h"
#include <BLEInterface.h>



// Seeed XIAO ESP32-C3 Hardware Mapping
#define IMU_INT_PIN D2             // D2 is GPIO 4
#define NO_MOTION_TIMEOUT_MS 10000 // 10 seconds (gives you time to see Serial)

unsigned long lastMotionTime = 0;
MPU9250 IMU(Wire, 0x68);

#define PIN_R D10 // D0
#define PIN_G D7  // D1
#define PIN_B D8  // D3 (D2/GPIO4 is used by your IMU)
bool commonAnode = false;

RTC_DATA_ATTR float MAX_ACCL_MSS = 18;

#define BLE_DEVICE_NAME "POI"
#define BLE_SERVICE_UUID "c5b6fc84-1450-4f82-83c7-ef4dc0e948de"
#define BLE_CHAR_UUID "309d5cfd-4ad1-45f6-81c8-fd6f512ae200"
BLEInterface * ble_driver = nullptr;

// Function to acknowledge/clear the MPU9250 interrupt hardware pin
void clearIMUInterrupt()
{
  Wire.beginTransmission(0x68);
  Wire.write(0x3A); // Read INT_STATUS register
  Wire.endTransmission();
  Wire.requestFrom(0x68, 1);
  while (Wire.available())
    Wire.read();
}

void blinkLED(int times, int duration)
{
  for (int i = 0; i < times; i++)
  {
    digitalWrite(PIN_G, HIGH);
    delay(duration);
    digitalWrite(PIN_G, LOW);
    delay(duration);
  }
}

void setRGB(int r, int g, int b)
{
  if (commonAnode)
  {
    analogWrite(PIN_R, 255 - r);
    analogWrite(PIN_G, 255 - g);
    analogWrite(PIN_B, 255 - b);
  }
  else
  {
    analogWrite(PIN_R, r);
    analogWrite(PIN_G, g);
    analogWrite(PIN_B, b);
  }
}

// Function to convert HSV to RGB
// h: 0-360, s: 0-100, v: 0-100
void setHSV(float h, float s, float v)
{
  float r, g, b;

  s /= 100.0;
  v /= 100.0;

  if (s == 0)
  {
    r = g = b = v;
  }
  else
  {
    float f, p, q, t;
    int i;

    h /= 60.0;
    i = floor(h);
    f = h - i;
    p = v * (1.0 - s);
    q = v * (1.0 - (s * f));
    t = v * (1.0 - (s * (1.0 - f)));

    switch (i)
    {
    case 0:
      r = v;
      g = t;
      b = p;
      break;
    case 1:
      r = q;
      g = v;
      b = p;
      break;
    case 2:
      r = p;
      g = v;
      b = t;
      break;
    case 3:
      r = p;
      g = q;
      b = v;
      break;
    case 4:
      r = t;
      g = p;
      b = v;
      break;
    default:
      r = v;
      g = p;
      b = q;
      break;
    }
  }

  setRGB((int)(r * 255), (int)(g * 255), (int)(b * 255));
}

void enterDeepSleep()
{
  Serial.println("Preparing to sleep...");
  Serial.println("Configuring WOM and sleeping...");
  setRGB(0, 0, 0);

  // Switch IMU to Low Power WOM Mode
  IMU.enableWakeOnMotion(400, MPU9250::LP_ACCEL_ODR_15_63HZ);
  // Latch interrupt and clear on any read
  Wire.beginTransmission(0x68);
  Wire.write(0x37);
  Wire.write(0x30);
  Wire.endTransmission();

  // 1. Clear any existing interrupt before sleeping
  clearIMUInterrupt();
  delay(50);

  // 2. Configure Wakeup
  // Level-triggered: if the pin is HIGH, the chip wakes up.
  esp_deep_sleep_enable_gpio_wakeup(1ULL << IMU_INT_PIN, ESP_GPIO_WAKEUP_GPIO_HIGH);

  Serial.println("Entering Deep Sleep now. LED is OFF.");
  Serial.flush();

  // Ensure LED is OFF before sleeping (HIGH = OFF on XIAO C3)

  esp_deep_sleep_start();
}

void setup()
{
  // --- LED STARTUP INDICATION ---

  
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(IMU_INT_PIN, INPUT_PULLDOWN);
  
  setRGB(0,255,0); // green to show booting

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

  Wire.begin();
  if (IMU.begin() < 0)
  {
    Serial.println("IMU Initialization FAILED!");
    blinkLED(PIN_R, 100);
  }

  lastMotionTime = millis();
}

void loop()
{
  IMU.readSensor();

  // 2. Calculate Magnitude: sqrt(x^2 + y^2 + z^2)
  float ax = IMU.getAccelX_mss();
  float ay = IMU.getAccelY_mss();
  float az = IMU.getAccelZ_mss();

  float magnitude = sqrt(ax * ax + ay * ay + az * az);
  float motionForce = abs(magnitude - 9.81);
  float sensitivity_gain = 360/MAX_ACCL_MSS;
  float hue = map(constrain(motionForce*sensitivity_gain, 0, 360), 0, 360, 240, 0);

  unsigned long timeSinceMotion = millis() - lastMotionTime;
  float brightness = map(constrain(timeSinceMotion, 0, NO_MOTION_TIMEOUT_MS), 0, NO_MOTION_TIMEOUT_MS, 100, 0);

  setHSV(hue, 100, brightness);

  if (timeSinceMotion < NO_MOTION_TIMEOUT_MS)
  {
    setHSV(hue, 100, brightness);
  }
  else
  {
    enterDeepSleep();
  }

  if (motionForce > 0.5f || digitalRead(IMU_INT_PIN) == HIGH)
  {
    lastMotionTime = millis();
    clearIMUInterrupt();
  }

  if(ble_driver->connected()){
        float temp = IMU.getTemperature_C();
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