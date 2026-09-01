#include "BleServer.h"
#include <NimBLEDevice.h>

class ServerCallbacks : public NimBLEServerCallbacks {
    // Updated for NimBLE 2.x
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        Serial.printf("[BLE] Client connected from %s!\n", connInfo.getAddress().toString().c_str());
    }

    // Updated for NimBLE 2.x
    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        Serial.printf("[BLE] Client disconnected (Reason: %d). Restarting advertising...\n", reason);
        NimBLEDevice::startAdvertising();
    }
};

BleServer::BleServer(const char* serviceUUID) : _serviceUUID(serviceUUID) {}

void BleServer::begin(const char* deviceName, std::initializer_list<BleEndpointBase*> endpoints) {
    // 1. Store the endpoint pointers
    _endpoints = endpoints;

    // 2. Initialize NimBLE stack
    NimBLEDevice::init(deviceName);
    NimBLEDevice::setPower(ESP_PWR_LVL_N0);

    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService* pService = pServer->createService(_serviceUUID);

    // 3. SERVER CONSUMES ENDPOINTS:
    // Tell each endpoint to create its characteristic and attach its callbacks
    for (BleEndpointBase* ep : _endpoints) {
        ep->attachToService(pService);
    }

    // 4. Start service and advertising
    pService->start();

    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    pAdv->addServiceUUID(_serviceUUID);
    pAdv->enableScanResponse(true);
    pAdv->setMinInterval(160);
    pAdv->setMaxInterval(320);
    pAdv->start();
}

void BleServer::update() {
    // SERVER SYNCS ENDPOINTS:
    // Update every registered endpoint's thread-safe double-buffer
    for (BleEndpointBase* ep : _endpoints) {
        ep->update();
    }
}