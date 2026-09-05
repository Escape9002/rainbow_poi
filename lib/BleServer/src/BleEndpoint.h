#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <string>
#include <cstring>

// --- C++11 Template Specialization for String Parsing ---
// 1. Declare the generic template
template <typename T>
T parseBLEString(const std::string &s);

// TODO none of these parsers check parameter lengths, are therefore prone to wrapping issues etc.

// 2. Specialize for int32_t
template <>
inline int32_t parseBLEString<int32_t>(const std::string &s)
{

    return static_cast<int32_t>(std::strtol(s.c_str(), nullptr, 10));
}

template <>
inline std::string parseBLEString<std::string>(const std::string &s)
{

    return s;
}

template <>
inline uint16_t parseBLEString<uint16_t>(const std::string &s)
{

    return static_cast<uint16_t>(std::strtol(s.c_str(), nullptr, 10));
}

template <>
inline uint32_t parseBLEString<uint32_t>(const std::string &s)
{

    return static_cast<uint32_t>(std::strtoul(s.c_str(), nullptr, 10));
}

template <>
inline uint8_t parseBLEString<uint8_t>(const std::string &s)
{

    return static_cast<uint8_t>(std::strtoul(s.c_str(), nullptr, 10));
}

// 3. Specialize for float
template <>
inline float parseBLEString<float>(const std::string &s)
{
    return std::strtof(s.c_str(), nullptr);
}

class BleEndpointBase
{
public:
    virtual ~BleEndpointBase() = default;
    virtual void attachToService(NimBLEService *pService) = 0;
    virtual bool update() = 0;
};

template <typename T>
class BleEndpoint : public BleEndpointBase, public NimBLECharacteristicCallbacks
{
private:
    std::string _uuid;
    std::string _description;
    uint32_t _properties;
    T _bg_value;
    T _fg_value;
    bool _has_updates;
    SemaphoreHandle_t _mutex;
    NimBLECharacteristic *_pChar;

public:
    // ---------------------------------------------------------
    // C++11 STANDARD: Delete copy constructor & assignment operator
    // Prevents accidental copying which would cause Mutex double-deletion crashes.
    // ---------------------------------------------------------
    BleEndpoint(const BleEndpoint &) = delete;
    BleEndpoint &operator=(const BleEndpoint &) = delete;

    // C++11 STANDARD: Use const reference for initialValue
    BleEndpoint(
        const char *uuid,
        const char *description,
        const T &initialValue,
        uint32_t properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE)
        : _uuid(uuid),
          _description(description),
          _properties(properties),
          _bg_value(initialValue),
          _fg_value(initialValue),
          _has_updates(false), _pChar(nullptr)
    {
        _mutex = xSemaphoreCreateMutex();
    }

    ~BleEndpoint() override
    {
        if (_mutex != nullptr)
        {
            vSemaphoreDelete(_mutex);
        }
    }

    void attachToService(NimBLEService *pService) override
    {
        _pChar = pService->createCharacteristic(_uuid, _properties);
        _pChar->setCallbacks(this);

        // NIMBLE 2.x STANDARD: NimBLE natively supports templates for setValue!
        _pChar->setValue(_fg_value);

        // Human-readable characteristic description
        NimBLEDescriptor *description =
            _pChar->createDescriptor(
                "2901",
                NIMBLE_PROPERTY::READ);

        description->setValue(_description);
    }

    void onWrite(NimBLECharacteristic *pChar, NimBLEConnInfo &connInfo) override
    {
        std::string rxData = pChar->getValue();

        if (!rxData.empty())
        { // C++11 prefers .empty() over .length() > 0

            // Call our clean template specialization
            T parsedValue = parseBLEString<T>(rxData);

            if (xSemaphoreTake(_mutex, portMAX_DELAY))
            {
                _bg_value = parsedValue;
                _has_updates = true;
                xSemaphoreGive(_mutex);
            }
        }
    }

    bool update() override
    {
        bool updated = false;
        if (xSemaphoreTake(_mutex, portMAX_DELAY))
        {
            if (_has_updates)
            {
                _fg_value = _bg_value;
                _has_updates = false;
                updated = true;
            }
            xSemaphoreGive(_mutex);
        }
        return updated;
    }

    T getValue() const
    {
        return _fg_value;
    }

    // C++11 STANDARD: Pass newValue by const reference
    void setValue(const T &newValue)
    {
        _fg_value = newValue;

        if (_pChar != nullptr)
        {
            // NIMBLE 2.x STANDARD: Let NimBLE handle the byte casting
            _pChar->setValue(newValue);

            // NIMBLE 2.x STANDARD: Support both Notify and Indicate
            if ((_properties & NIMBLE_PROPERTY::NOTIFY) || (_properties & NIMBLE_PROPERTY::INDICATE))
            {
                _pChar->notify();
            }
        }
    }
};