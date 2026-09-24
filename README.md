# Rainbow Poi
Tiny LED's to be swung in circles at high speeds.
Yes, they change color when getting faster.
Yes, you can controll how they change color.
Yes, you can controll what is measured to change color.
Yes, there are more settings.

You can find the controller app here: [rainbow_app](https://github.com/Escape9002/rainbow_app).

## Firmware
### Animation States
Theres an Animation State machine, currently supported are:
- Acceleration
- Gyroscope
- Flashing
- Rainbow
- Constant

Some of these states can be manipulated via the BLE(Bluetooth Low Energy) interface.
You can find the implementation here:
``./lib/PoiController/src/AnimationState``

### Hardware States
The Poi will go to sleep after a configured amount of time (usually 5 minutes), if no motion is detected.
You can wake it with a HARD shake (bonking it on the ground will do it, or against a second poi).
If the Poi starts flashing red, the battery is low. Please recharge asap.

The Hardware State Machine supports these states:
- Idle (waiting for sleep)
- On
- Sleep (shutting down Wifi, BLE, waking at ShakeDetection of IMU)
- LowBattery (if <10% charge remaining, flash red)

### OverTheAir Updates
The Firmware supports OTA updates, I dont yet know how to do this via the rainbow_app, but latery maybe...


## Case
You can find my Case-Design here:
``SimpleCase.FCStd``

Please come up with a better one. 

## Circuit
```text
       [ Li-Ion Battery 3.7V ]
         | +          | -
         |            |
  +------|------------|----------------------+
  |     5V           GND                     |
  |                                          |
  |     A3 (Bat Lvl) <-----[ V-Divider ]-----+
  |                                          |
  |         [ESP32-C3 SuperMini]             |
  |                                          |
  |  GPIO2 (LED Data) -----------------------> [ WS2812B LED Strip (15x) ]
  |  GPIO8 (Status LED)                      |
  |                                          |
  |  GPIO4 (IMU_INT) <-----------+           |
  |  GPIO5 (SCL) <------------+  |           |
  |  GPIO6 (SDA) <---------+  |  |           |
  |                        |  |  |           |
  +------------------------|--|--|-----------+
                           |  |  |
                        +--|--|--|-+
                        | SDA    | |
                        | SCL  INT |
                        | [MPU9250]|
                        |  9-DOF   |
                        +----------+
```

- **LED-Strip**:
    powered directly from the battery
- **Battery**:
    Choosing Lithium-ion since current drain rate is high enough for MCU and LEDs, while not being a handgranate in high impact scenarios (looking at you, LiPo). I got these ones: [Jesspow CR123A](https://jesspow.com/products/cr123a-rechargeable-li-ion-battery-4-pack)
- **Voltage Divider**: 
    consists of 2x10k Ohm resistors, to ensure that the analog pin of the ESP can measure the up to 4V of the battery.

Since my battery is mounted on a different PCB, I "insulated" the backside of the main PCB with some tape.

# Pictures

> You can find all the pics in the ``img`` folder

![img](/img/InAction.webp)
![img](/img/PCB_Assembled.webp)
![img](/img/InCase.webp)