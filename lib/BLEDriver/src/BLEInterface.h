#ifndef BLE_INTERFACE_H
#define BLE_INTERFACE_H

#include "Arduino.h"

// Debug macros are best placed here or in the header, outside the class
#define DEBUG_BLEDRIVER 1
#if DEBUG_BLEDRIVER 
#define DEBUG_PRINTLN(x)           \
    Serial.print("\t[BLEDriver] "); \
    Serial.println(x);             \
    Serial.flush();
#define DEBUG_PRINT(x) Serial.print(x)
#else
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT(x)
#endif

class BLEInterface
{
public:
    virtual ~BLEInterface() {} // Virtual destructor is important for interfaces

    virtual bool begin(String device_name, String service_uuid, String char_uuid) = 0;
    virtual bool connected() = 0;
    virtual bool sendDataPacket(const void *data, unsigned int  len) = 0;
    virtual bool sendWhenReady(const void *data, unsigned int len) = 0;
    virtual bool available() = 0;
    virtual String get_received() = 0;
};

#endif // BLE_INTERFACE_H