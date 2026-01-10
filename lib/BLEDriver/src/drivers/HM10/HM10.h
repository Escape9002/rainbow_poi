#ifndef HM_10_H
#define HM_10_H

#include "BLEInterface.h"
#include "SoftwareSerial.h"

class HM10 : public BLEInterface
{
public:
    // --- Singleton Access ---
    static HM10 &getInstance();
    HM10(const HM10 &) = delete;
    void operator=(const HM10 &) = delete;

    HM10();

    virtual bool begin(String device_name, String service_uuid, String char_uuid) override;
    virtual bool connected() override;
    virtual bool sendDataPacket(const void *data, uint16_t len) override;
    virtual bool sendWhenReady(const void *data, uint16_t len) override;
    virtual bool available() override;
    virtual String get_received() override;

private:
    // --- Singleton Instance Pointer ---
    static HM10 *_instance;

    bool sendATCommand(const char *cmd);
};

#endif