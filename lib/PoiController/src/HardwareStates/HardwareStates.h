enum class HARDWARE_STATE
{
    SLEEP,
    IDLE,
    ON,
    LOW_BATTERY,
    ERROR

};

const char* toString(HARDWARE_STATE state)
{
    switch (state)
    {
    case HARDWARE_STATE::SLEEP:
        return "SLEEP";

    case HARDWARE_STATE::IDLE:
        return "IDLE";

    case HARDWARE_STATE::ON:
        return "ON";

    case HARDWARE_STATE::LOW_BATTERY:
        return "LOW_BATTERY";

    default:
        return "ERR";
    }
}

HARDWARE_STATE HardwareStateFromString(const char* str)
{
    if (str == "SLEEP")
        return HARDWARE_STATE::SLEEP;
    if (str == "IDLE")
        return HARDWARE_STATE::IDLE;
    if (str == "ON")
        return HARDWARE_STATE::ON;
    if (str == "LOW_BATTERY")
        return HARDWARE_STATE::LOW_BATTERY;

    return HARDWARE_STATE::ERROR;
}