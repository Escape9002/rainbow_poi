#ifndef BLE_NRF52_H
#define BLE_NRF52_H

#include <bluefruit.h>

// It's good practice to have an interface if other drivers might exist.
// If not, you can remove ": public BLEDriver" and the "override" keywords.
#include "BLEInterface.h" // Assuming BLEDriver.h from the previous example exists.

class BLEnRF52 : public BLEInterface
{
public:
    // --- Singleton Access ---
    static BLEnRF52 &getInstance();
    BLEnRF52(const BLEnRF52 &) = delete;
    void operator=(const BLEnRF52 &) = delete;

    // --- Public API ---
    virtual bool begin(String device_name, String service_uuid, String char_uuid) override;
    virtual bool connected() override;
    virtual bool sendDataPacket(const void *data, unsigned int len) override;
    virtual bool sendWhenReady(const void *data, unsigned int len) override;
    virtual bool available() override;
    virtual String get_received() override;

    BLEnRF52();

private:
    // --- Private Constructor for Singleton ---

    // --- Constants ---
    // Using clearer names and proper C++ style
    // const char *VEIIO_SERVICE_UUID;
    // const char *VEIIO_DATAPACKET_CHAR_UUID;

    // --- Bluefruit Objects ---
    BLEDis bledis;
    BLEUart bleuart;
    // BLEService service;
    // BLECharacteristic DataPacketChar;

    // --- Member Variables ---
    String received_msg; // Buffer for incoming UART data
     volatile bool new_message_available_;

    // --- Private Helper Methods ---
    void startAdv();
    void setup_DataPacketChar();
    void configDeviceInfo();
    void setupBLEUart();

    // --- Real Callback Handlers (Member Functions) ---
    void onConnect(uint16_t conn_handle);
    void onDisconnect(uint16_t conn_handle, uint8_t reason);
    void onBleUartRx(uint16_t conn_hdl);
    void onBleUartNotify(uint16_t conn_hdl, bool enabled);

    // --- Static Trampoline Functions (for C-style callbacks) ---
    static void static_connect_callback(uint16_t conn_handle);
    static void static_disconnect_callback(uint16_t conn_handle, uint8_t reason);
    static void static_bleuart_rx_callback(uint16_t conn_hdl);
    static void static_bleuart_notify_callback(uint16_t conn_hdl, bool enabled);

    // --- Singleton Instance Pointer ---
    static BLEnRF52 *_instance;
};

#endif // BLE_NRF52_H