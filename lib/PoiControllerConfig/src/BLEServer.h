#pragma once
#include <Arduino.h>
#include <vector>
#include <initializer_list>
#include "BleEndpoint.h"

class BleServer {
private:
    std::string _serviceUUID;
    std::vector<BleEndpointBase*> _endpoints;

public:
    explicit BleServer(const char* serviceUUID);

    // Consumes a list of custom endpoint pointers
    void begin(const char* deviceName, std::initializer_list<BleEndpointBase*> endpoints);

    // Synchronizes all registered endpoints across tasks
    void update();
};