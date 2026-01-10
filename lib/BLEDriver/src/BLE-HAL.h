#ifndef BLE_DRIVER_H
#define BLE_DRIVER_H

#include "BLE-HAL.h" // The common interface

// Conditionally include the header for the specific driver
#if defined(USE_NRF52_DRIVER)
#include "drivers/nrf52/BLEnRF52.h"
#elif defined(USE_ESP32_DRIVER)
#include "drivers/esp32/BLE_ESP32.h"
#elif defined(USE_HM10_DRIVER)
#include "drivers/HM10/HM10.h"
#else
#error "No BLE driver specified! Please define USE_NRF52_DRIVER or USE_ESP32_DRIVER in your platformio.ini build_flags."
#endif

// Expose BLEDriver as a name instead of BLEInterface for clearer semantics
using BLEDriver = BLEInterface;

/**
 * @brief Factory function to get the correct singleton instance of the BLE driver.
 *
 * @return A reference to the BLEDriver interface.
 */
inline BLEInterface &getBLEDriverInstance()
{
#if defined(USE_NRF52_DRIVER)
    return BLEnRF52::getInstance();
#elif defined(USE_ESP32_DRIVER)
    return NimBLEWrapper::getInstance();
#elif defined(USE_HM10_DRIVER)
    return HM10::getInstance();
#endif
}

#endif // BLE_DRIVER_H