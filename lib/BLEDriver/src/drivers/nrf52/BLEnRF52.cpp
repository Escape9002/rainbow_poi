#if defined(USE_NRF52_DRIVER)

#include "BLEnRF52.h"

/**
 * The Bluefruit library expects functions with this header:
 *      typedef void (*rx_callback_t) (uint16_t conn_hdl);
 * Since functions originating have a different header
 *      (BLEnRF::*) (uint16_t conn_hdl)
 * its required to mark the functions passed to bluefruit as
 * static. Since this may break function when there is more
 * then one of these driver objects, we have to ensure that
 * only one will ever exist.
 *
 * This is ensured via the getInstance functions.
 * Since the static functions have no access to local variables
 * of the object, a trampolin pattern is implemented to send
 * the call "into" the object with local variables.
 */

// --- Static Member Initialization ---
BLEnRF52 *BLEnRF52::_instance = nullptr;

// --- Singleton Accessor ---
BLEnRF52 &BLEnRF52::getInstance()
{
    if (_instance == nullptr)
    {
        _instance = new BLEnRF52();
    }
    return *_instance;
}

// --- Constructor (CORRECT way to initialize members) ---
BLEnRF52::BLEnRF52() : bledis(),
                       bleuart()
{
    // The constructor is now responsible for initializing the objects.
    // The `begin()` method is for starting them.
}

// --- Public Method Implementations ---

bool BLEnRF52::begin(String device_name, String service_uuid, String char_uuid)
{
    _instance = this; // Set the instance pointer for static callbacks

    // VEIIO_SERVICE_UUID = service_uuid.c_str();
    // VEIIO_DATAPACKET_CHAR_UUID = char_uuid.c_str();

    // service = BLEService(VEIIO_SERVICE_UUID);
    // DataPacketChar = BLECharacteristic(VEIIO_DATAPACKET_CHAR_UUID);

    // Setup basic BLE settings
    Bluefruit.autoConnLed(true);
    Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
    Bluefruit.begin();
    Bluefruit.setTxPower(4);
    Bluefruit.Periph.setConnInterval(6, 12); // 7.5 - 15 ms
    Bluefruit.setName(device_name.c_str());

    // Set the callbacks to our static "trampoline" functions
    Bluefruit.Periph.setConnectCallback(static_connect_callback);
    Bluefruit.Periph.setDisconnectCallback(static_disconnect_callback);

    // Setup services and characteristics in the correct order
    configDeviceInfo();
    setupBLEUart();
    // setup_DataPacketChar(); // This will now also begin the service

    // Finally, start advertising
    startAdv();

    DEBUG_PRINTLN("BLEnRF52 Driver Started");
    return true;
}

bool BLEnRF52::connected()
{
    return Bluefruit.connected();
}

// CORRECTED: Must take data and length
bool BLEnRF52::sendDataPacket(const void *data, unsigned int len)
{
    // DEBUG_PRINTLN("Received call to send Packet");
    if (connected())
    {
        return bleuart.write((const uint8_t *)data, len);
    }
    else
    {
        DEBUG_PRINTLN("We are not connected, cant send");
    }
    return false;
}

bool BLEnRF52::sendWhenReady(const void *data, unsigned int len)
{
    // For now, it's just a pass-through. A real implementation
    // would buffer the data if not connected.
    return sendDataPacket(data, len);
}

// CORRECTED: Checks our internal buffer, not the hardware buffer
bool BLEnRF52::available()
{

    return new_message_available_;
}

// CORRECTED: Returns and clears our internal buffer
String BLEnRF52::get_received()
{
    if (available())
    {
        String msg = received_msg;
        received_msg = "";              // Clear buffer after reading
        new_message_available_ = false; // Reset availability flag
        return msg;
    }
    return "";
}

// --- Private Helper Implementations ---

void BLEnRF52::configDeviceInfo()
{
    bledis.setManufacturer("Veiio");
    bledis.setModel("Body Hub");
    bledis.setFirmwareRev("0.0.1"); // Best to use semver
    bledis.begin();
}

void BLEnRF52::setupBLEUart()
{
    bleuart.bufferTXD(true); // Enable TX buffering
    bleuart.begin();
    // Use the static trampoline functions for callbacks
    bleuart.setRxCallback(static_bleuart_rx_callback);
    bleuart.setNotifyCallback(static_bleuart_notify_callback); // Corrected typo
}

// void BLEnRF52::setup_DataPacketChar()
// {
//     DataPacketChar.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
//     DataPacketChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
//     // DataPacketChar.setFixedLen(sizeof(DataPacket)); // You can set this if your packets are always the same size
//     DataPacketChar.begin();
//     DataPacketChar.write32(0); // Set initial value

//     // The service must be started *after* its characteristics are configured
//     service.begin();
// }

void BLEnRF52::startAdv()
{
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    // Bluefruit.Advertising.addService(service);
    Bluefruit.Advertising.addService(bleuart); // Also advertise UART service
    Bluefruit.ScanResponse.addName();
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);
    Bluefruit.Advertising.setFastTimeout(30);
    Bluefruit.Advertising.start(0);
}

// --- Static Trampoline Implementations ---

void BLEnRF52::static_connect_callback(uint16_t conn_handle)
{
    if (_instance)
        _instance->onConnect(conn_handle);
}

void BLEnRF52::static_disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
    if (_instance)
        _instance->onDisconnect(conn_handle, reason);
}

void BLEnRF52::static_bleuart_rx_callback(uint16_t conn_hdl)
{
    if (_instance)
        _instance->onBleUartRx(conn_hdl);
}

void BLEnRF52::static_bleuart_notify_callback(uint16_t conn_hdl, bool enabled)
{
    if (_instance)
        _instance->onBleUartNotify(conn_hdl, enabled);
}

// --- Real Callback Handler Implementations ---

void BLEnRF52::onConnect(uint16_t conn_handle)
{
    DEBUG_PRINTLN("Connected");
    BLEConnection *conn = Bluefruit.Connection(conn_handle);

    /// @note enabling special MTU/PHY/LE Data Length features seems to cause issues with some apps
    // conn->requestPHY();
    // conn->requestDataLengthUpdate();
    // conn->requestConnectionParameter(6, 0, 400); // 7.5ms, no latency, 4s supervision timeout
    // conn->requestMtuExchange(50); // activating this, has the app glitching out
#if DEBUG_BLEDRIVER
    String settings = conn->getPHY() == BLE_GAP_PHY_2MBPS ? "2M" : (conn->getPHY() == BLE_GAP_PHY_1MBPS ? "1M" : "Coded");
    DEBUG_PRINTLN("Connection MTU: " + String(conn->getMtu()) + ", PHY: " + settings + ", Interval: " + String(conn->getConnectionInterval()) + " units");
#endif
    delay(100); // Small delay for requests to process
}

void BLEnRF52::onDisconnect(uint16_t conn_handle, uint8_t reason)
{
    (void)conn_handle;
    DEBUG_PRINT("Disconnected, reason = " + String(reason, HEX));
}

// CORRECTED: Buffer data here instead of in get_received()
void BLEnRF52::onBleUartRx(uint16_t conn_hdl)
{
    // (void)conn_hdl;
    // while (bleuart.available())
    // {
    //     received_msg += (char)bleuart.read();

    // }
    while (bleuart.available())
    {
        char c = (char)bleuart.read();
        received_msg += c;
        // Optionally, you can add a check for end-of-line or message termination here
        if (c == '\n')
        { // Example: end of message on newline
            DEBUG_PRINTLN(String(millis()) + "\tReceived data: " + received_msg);
            new_message_available_ = true;
        }
    }
}

void BLEnRF52::onBleUartNotify(uint16_t conn_hdl, bool enabled)
{
    (void)conn_hdl;
    if (enabled)
    {
        DEBUG_PRINTLN("BLE UART 'Notify' enabled by client.");
    }
    else
    {
        DEBUG_PRINTLN("BLE UART 'Notify' disabled by client.");
    }
}

#endif