# BLE-HAL
## Using a driver
Here is a base example:

main.cpp
```cpp
#include "BLE-HAL.h"
BLEInterface * ble_driver = nullptr;

void setup(){
    ble_driver = &getBLEDriverInstance();

    ble_driver->begin();
}

void loop(){
    if(ble_driver->connected()){
        uint32_t test_payload = 0xDEADBEEF;
        ble_driver->sendDataPacket(&test_payload, sizeof(test_payload));
    }
}
```

Pay attention to endianes when using any BLE-Connectivity.

## Creating a new driver
Please note that your driver should only be compiled when used.
Usage is specified over compiler flags. Thus, please wrap your code in 
```cpp
#if defined(USE_YOURDIRVER_DRIVER)
...
#endif
```
blocks. Thank you!

### .h and .cpp files
To add new BLE-Devices to this library please follow the examples of the HM-10 or nRF52840.  
Essentially it goes like this:

Extend this BLE-Interface with you implementation like this:

myBLE.h
```cpp
public myBLE : public BLEInterface {
    /** 
    * Add the declared functions of the Interface here and
    * override them!
    */
}
```

Since some BLE-Drivers are wierd, we need a trampoline + singleton pattern here. More can be found here: ```drivers/nRF52.cpp```

I dont know yet how to add this to the interface, so here are the rest of the required fields which go into the ```.h``` file:
- static reference function to retrieve an instance of this object. In the code its labeled under Singleton Access
```cpp
    // --- Singleton Access ---
    static HM10 &getInstance();
    HM10(const HM10 &) = delete;
    void operator=(const HM10 &) = delete;
```
- statis instance pointer
``` cpp
   // --- Singleton Instance Pointer ---
    static HM10 *_instance;
```

Please provide an implementation for these functions. Examples can be found within the drivers.

### Factory
You have to add your driver to the ```BLE-HAL.h``` file, simply follow the already existing ones. Take care that you defined the compiler flags mentioned above!