#ifndef BLE_ESP32_H
#define BLE_ESP32_H

#include "BLEInterface.h"
#include <NimBLEDevice.h>

#define BLE_DEBUG 1
#if BLE_DEBUG
#define DEBUG_PRINTLN(x)    \
    Serial.print("[BLE] "); \
    Serial.println(x);      \
    Serial.flush();
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_BEGIN(x) Serial.begin(x)
#define DEBUG_SERIAL Serial
#else
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT(x)
#define DEBUG_BEGIN(x)
#define DEBUG_SERIAL
#endif

class NimBLEWrapper : public BLEInterface
{
public:
    // --- Singleton Access ---
    static NimBLEWrapper &getInstance();
    NimBLEWrapper(const NimBLEWrapper &) = delete;
    void operator=(const NimBLEWrapper &) = delete;
    virtual bool begin(String device_name, String service_uuid, String char_uuid) override;
    virtual bool connected() override;
    virtual bool sendDataPacket(const void *data, unsigned int len) override;
    virtual bool sendWhenReady(const void *data, unsigned int len) override;
    virtual bool available() override;
    virtual String get_received() override;

    NimBLEWrapper();

private:
    // --- Constants ---
    // Using clearer names and proper C++ style
    const char *VEIIO_SERVICE_UUID;
    const char *VEIIO_DATAPACKET_CHAR_UUID;

    /// @brief Nordic UART Service (NUS) UUIDs
    const char *NUS_SERVICE_UUID;
    const char *NUS_CHAR_UUID_RX; // RX is for data FROM the phone TO the MCU (WRITE)
    const char *NUS_CHAR_UUID_TX; // TX is for data FROM the MCU TO the phone (NOTIFY)

    NimBLEService *pUartService;
    NimBLECharacteristic *pRXChar;
    NimBLECharacteristic *pTXChar;



    NimBLEServer *pServer;
    NimBLEService *pVeiioService;
    NimBLECharacteristic *pDataPacketChar;

    // --- Member Variables ---
    String received_msg; // Buffer for incoming UART data
    volatile bool new_message_available_;

    // --- Internal Callback Handlers ---
    // These are the real, non-static methods that handle events.
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo);                // Add connInfo
    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason); // Add connInfo and reason
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo);  // Add connInfo

    // --- Trampoline Classes for NimBLE Callbacks ---
    // These static-like classes forward the C-style callbacks to our object instance.
    class ServerCallbacks : public NimBLEServerCallbacks
    {
        NimBLEWrapper *pWrapper;

    public:
        ServerCallbacks(NimBLEWrapper *wrapper) : pWrapper(wrapper) {}
        void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override
        {
            DEBUG_PRINTLN("callback connect");
            pWrapper->onConnect(pServer, connInfo);
        }
        void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override
        {

            DEBUG_PRINTLN("callback disconnect");
            DEBUG_PRINT("\treason: ");
            DEBUG_PRINT(reason);

            pWrapper->onDisconnect(pServer, connInfo, reason);
        }
    };

    class CharacteristicCallbacks : public NimBLECharacteristicCallbacks
    {
        NimBLEWrapper *pWrapper;

    public:
        CharacteristicCallbacks(NimBLEWrapper *wrapper) : pWrapper(wrapper) {}
        void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
        {
            // DEBUG_PRINTLN("callback onwrite");
            pWrapper->onWrite(pCharacteristic, connInfo);
        }
    };

    // Instantiate the callback handler objects
    ServerCallbacks *serverCallbacks;
    CharacteristicCallbacks *charCallbacks;
};

#endif