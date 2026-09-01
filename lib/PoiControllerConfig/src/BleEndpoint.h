#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <string>
#include <cstdlib> // C++11 Standard instead of stdlib.h

// --- C++11 Template Specialization for String Parsing ---
// 1. Declare the generic template
template <typename T> 
T parseBLEString(const std::string& s);

// 2. Specialize for int32_t
template <> 
inline int32_t parseBLEString<int32_t>(const std::string& s) {
    // std::strtol is safe without exceptions. static_cast is C++11 standard.
    return static_cast<int32_t>(std::strtol(s.c_str(), nullptr, 10));
}

// 3. Specialize for float
template <> 
inline float parseBLEString<float>(const std::string& s) {
    return std::strtof(s.c_str(), nullptr);
}

// --- Abstract Base Class ---
class BleEndpointBase {
public:
    // C++11 standard requires virtual destructors for polymorphic base classes
    virtual ~BleEndpointBase() = default;
    virtual void attachToService(NimBLEService* pService) = 0;
    virtual bool update() = 0;
};

// --- Template Class ---
template <typename T>
class BleEndpoint : public BleEndpointBase, public NimBLECharacteristicCallbacks {
private:
    std::string _uuid;
    T _bg_value;
    T _fg_value;
    bool _has_updates;
    SemaphoreHandle_t _mutex;

public:
    BleEndpoint(const char* uuid, T initialValue) 
        : _uuid(uuid), _bg_value(initialValue), _fg_value(initialValue), _has_updates(false) {
        _mutex = xSemaphoreCreateMutex();
    }

    // Rule of Zero/Three/Five: If we create a Mutex, we must destroy it.
    ~BleEndpoint() override {
        if (_mutex != nullptr) {
            vSemaphoreDelete(_mutex);
        }
    }

    void attachToService(NimBLEService* pService) override {
        NimBLECharacteristic* pChar = pService->createCharacteristic(
            _uuid, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
        );
        pChar->setCallbacks(this);
        
        // NimBLE prefers std::string for setting text values natively
        pChar->setValue(std::to_string(_fg_value));
    }

    void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
        std::string rxData = pChar->getValue();
        if (rxData.length() == sizeof(T)) {
            T incomingValue;
            memcpy(&incomingValue, rxData.data(), sizeof(T));

            if (xSemaphoreTake(_mutex, portMAX_DELAY)) {
                _bg_value = incomingValue;
                _has_updates = true;
                xSemaphoreGive(_mutex);
            }
        }
    }

    bool update() override {
        bool updated = false;
        if (xSemaphoreTake(_mutex, portMAX_DELAY)) {
            if (_has_updates) {
                _fg_value = _bg_value;
                _has_updates = false;
                updated = true;
            }
            xSemaphoreGive(_mutex);
        }
        return updated;
    }

    T getValue() const {
        return _fg_value;
    }
};