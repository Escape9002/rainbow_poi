#if defined(USE_ESP32_DRIVER)

#include "BLE_ESP32.h"

// #define BLE_DEBUG 1
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

// // --- Global getBLEDriverInstance implementation ---
// // This now returns a reference to our NimBLEWrapper singleton.
// BLEInterface& getBLEDriverInstance() {
//     return NimBLEWrapper::getInstance();
// }

// --- Singleton Implementation ---
NimBLEWrapper &NimBLEWrapper::getInstance()
{
    DEBUG_PRINTLN("get instance");
    static NimBLEWrapper instance;
    return instance;
}

// --- Constructor ---
// Initialize members, especially the callback handlers, in the initializer list.
NimBLEWrapper::NimBLEWrapper()
    : pServer(nullptr),
      pVeiioService(nullptr),
      pDataPacketChar(nullptr),
      new_message_available_(false)
//   serverCallbacks_(this), // Pass 'this' to the callback handlers
//   charCallbacks_(this)
{
    DEBUG_PRINTLN("create a new instance");
    serverCallbacks = new ServerCallbacks(this);
    charCallbacks = new CharacteristicCallbacks(this);
}

bool NimBLEWrapper::begin(String device_name, String service_uuid, String char_uuid)
{

    VEIIO_SERVICE_UUID = service_uuid.c_str();
    VEIIO_DATAPACKET_CHAR_UUID = char_uuid.c_str();

    //////////////////////////////////////////////////////////////////////////////////

    NUS_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
    NUS_CHAR_UUID_RX = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
    NUS_CHAR_UUID_TX = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";



    // 1. Initialize NimBLE
    NimBLEDevice::init(device_name.c_str());
    // Set power, security, etc. if needed
    // NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    DEBUG_PRINTLN("running init");

    // 2. Create the Server
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(serverCallbacks);

    //////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////
    // // 3. Create our custom Veiio Service
    // pVeiioService = pServer->createService(VEIIO_SERVICE_UUID);

    // // 4. Create the Data Packet Characteristic
    // pDataPacketChar = pVeiioService->createCharacteristic(
    //     VEIIO_DATAPACKET_CHAR_UUID,
    //     NIMBLE_PROPERTY::WRITE |
    //         NIMBLE_PROPERTY::WRITE_NR |
    //         NIMBLE_PROPERTY::NOTIFY |
    //         NIMBLE_PROPERTY::READ);
    // pDataPacketChar->createDescriptor("2902");
    // pDataPacketChar->setCallbacks(charCallbacks);

    // // 5. Start the service
    // pVeiioService->start();

    //////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////

 // --- 1. Create the UART Service with the STANDARD UUID ---
    pUartService = pServer->createService(NUS_SERVICE_UUID);

    // --- 2. Create the TX Characteristic (MCU -> Phone) ---
    // This is what we use to send data (notify).
    pTXChar = pUartService->createCharacteristic(
        NUS_CHAR_UUID_TX,
        NIMBLE_PROPERTY::NOTIFY
    );
    // Add the standard 2902 descriptor to allow subscriptions
    pTXChar->createDescriptor("2902");

    // --- 3. Create the RX Characteristic (Phone -> MCU) ---
    // This is what the phone WRITES to. This needs the onWrite callback.
    pRXChar = pUartService->createCharacteristic(
        NUS_CHAR_UUID_RX,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pRXChar->setCallbacks(charCallbacks); // <-- The callback goes HERE

    // --- 4. Start the service ---
    pUartService->start();

    //////////////////////////////////////////////////////////////////////////////////

    // 6. Setup and start advertising
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setName(device_name.c_str());
    // pAdvertising->addServiceUUID(pVeiioService->getUUID());
    pAdvertising->addServiceUUID(pUartService->getUUID());
    pAdvertising->enableScanResponse(true);
    pAdvertising->start();

    
    
    Serial.println(F("NimBLE Wrapper Initialized. Advertising started."));
    return true;
}

bool NimBLEWrapper::connected()
{
    // DEBUG_PRINTLN("check connection");
    return pServer->getConnectedCount() > 0;
}

bool NimBLEWrapper::available()
{
    // DEBUG_PRINTLN("check msgs");
    // This is the flag set by the ISR-context callback. It's safe to read.
    return new_message_available_;
}

String NimBLEWrapper::get_received()
{
    // DEBUG_PRINTLN("get what received");
    // This is called from the main loop, so it's safe.

    noInterrupts();
    String msg = received_msg;
    new_message_available_ = false; // Clear the flag
    interrupts();
    return msg;
}

bool NimBLEWrapper::sendDataPacket(const void *data, unsigned int len)
{
    // DEBUG_PRINTLN("send something");
    if (connected())
    {
        // DEBUG_PRINTLN("connected, sending");
        // pDataPacketChar->setValue((uint8_t*)data, len);
        // pDataPacketChar->notify();
        pTXChar->setValue((uint8_t*)data, len);
        pTXChar->notify();
        return true;
    }
    return false;
}

bool NimBLEWrapper::sendWhenReady(const void *data, unsigned int len)
{
    // currently not implemented
    return sendDataPacket(data, len);
}

// --- Internal Callback Handler Implementations ---

void NimBLEWrapper::onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo)
{
    // You now have access to the connection info!
    Serial.print(F("Client Connected: "));
    Serial.println(connInfo.getAddress().toString().c_str());
    uint8_t txPhy, rxPhy;
    pServer->getPhy(connInfo.getConnHandle(), &txPhy, &rxPhy);
    DEBUG_PRINTLN("Client MTU: " + String(connInfo.getMTU()) + ", Interval: " + String(connInfo.getConnInterval()) + " units"+ ", TX PHY: " + String(txPhy) + ", RX PHY: " + String(rxPhy));
}

void NimBLEWrapper::onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason)
{
    Serial.println(F("Client Disconnected"));
}

void NimBLEWrapper::onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo)
{
    // THIS IS THE ISR-CONTEXT PART
    // DEBUG_PRINTLN("onWrite callback triggered!");
    received_msg = pCharacteristic->getValue().c_str();
    new_message_available_ = true;
    DEBUG_PRINTLN("Received data: " + received_msg);
}

#endif