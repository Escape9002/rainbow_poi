#pragma once
#include <vector>
#include <initializer_list>
#include "BleEndpoint.h"

class BleServer
{
private:
    std::string _serviceUUID;
    std::vector<BleEndpointBase *> _endpoints;
    std::vector<BleEndpointBase *> _updatedEndpoints;

public:
    static constexpr uint16_t MIN_INTERVAL = 80;
    static constexpr uint16_t MAX_INTERVAL = 80;

    static constexpr uint16_t LATENCY = 0;
    static constexpr uint16_t TIMEOUT = 400;

    // OPTIMIZATION 1: Advertising Interval.
    // 800 units * 0.625ms = 500ms intervals.
    static constexpr uint16_t ADV_INTERVAL = 800;

    explicit BleServer(const char *serviceUUID);

    // Consumes a list of custom endpoint pointers
    void begin(const char *deviceName, std::initializer_list<BleEndpointBase *> endpoints);

    // Synchronizes all registered endpoints across tasks
    const std::vector<BleEndpointBase *> &update();
};