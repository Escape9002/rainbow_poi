#if defined(USE_HM10_DRIVER)
// Debug macros are best placed here or in the header, outside the class
// #define DEBUG 0
#if DEBUG
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINT(x) Serial.print(x)
#else
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT(x)
#endif

#include "HM10.h"
#include <Arduino.h>
#include "SoftwareSerial.h"

////////////////////////////////////// MOVE TO .h if possible
#define BT_RX 7
#define BT_TX 8
SoftwareSerial btSerial(BT_RX, BT_TX); // SoftwareSerial for Bluetooth
//////////////////////////////////////

// --- Static Member Initialization ---
HM10 *HM10::_instance = nullptr;

// --- Singleton Accessor ---
HM10 &HM10::getInstance()
{
    if (_instance == nullptr)
    {
        _instance = new HM10();
    }
    return *_instance;
}

HM10::HM10()
{
    // nothing to do here
}

bool HM10::begin(String device_name, String service_uuid, String char_uuid)
{
    _instance = this; // Set the instance pointer for static callbacks

    DEBUG_PRINTLN("Configuring Bluetooth...");
    btSerial.begin(19200);
    delay(250); // Allow HM-10 boot time

    String name_command = "AT+NAME" + device_name;

    if (!sendATCommand("AT"))
        return false; // Test communication
    if (!sendATCommand("AT+RESET"))
        return false; // Soft reset
    if (!sendATCommand(name_command.c_str()))
        return false; // Set device name
    if (!sendATCommand("AT+BAUD1"))
        return false; // Set baud rate to 9600
    if (!sendATCommand("AT+ROLE0"))
        return false; // Set to slave role

    return true;
}

bool HM10::sendATCommand(const char *cmd)
{
    btSerial.print(cmd); // No println! Only raw command
    delay(20);           // Give time for HM-10 to reply
    while (btSerial.available())
    {
        String response = btSerial.readStringUntil('\n');
        DEBUG_PRINTLN("HM-10 Response: " + response);

        if (response.startsWith("OK"))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

bool HM10::connected()
{
    return btSerial.available();
}

bool HM10::sendDataPacket(const void *data, uint16_t len)
{
    // Cast the void pointer to an unsigned char array
    const unsigned char *buffer = reinterpret_cast<const unsigned char *>(data);

    // Write the data packet to the serial interface (btSerial.write())
    btSerial.write(buffer, len);

    return true; // Assuming successful write operation
}

bool HM10::sendWhenReady(const void *data, uint16_t len)
{
    // TODO implement batched sending
    return false;
}

bool HM10::available()
{
    return btSerial.available();
}

String HM10::get_received()
{
    String inputBuffer = "";

    while (btSerial.available())
    {
        char c = btSerial.read();
        inputBuffer += c;
    }
    inputBuffer.trim();

    return inputBuffer;
}

#endif