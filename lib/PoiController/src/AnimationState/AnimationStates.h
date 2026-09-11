#pragma once 

enum class ANIMATION_STATE
{
    GYRO,
    ACCL,
    FLASH,
    CONST,
    RAINBOW,
    ERROR
};

const char* toString(ANIMATION_STATE state)
{
    switch (state)
    {
        
        case ANIMATION_STATE::GYRO:
            return "GYRO";

        case ANIMATION_STATE::ACCL:
            return "ACCL";

        case ANIMATION_STATE::FLASH:
            return "FLAS";

        case ANIMATION_STATE::CONST:
            return "CONS";

        case ANIMATION_STATE::RAINBOW:
            return "RAIN";

        case ANIMATION_STATE::ERROR:
        default:
            return "ERR";
        
    }
}

// should this function reside in AnimationState?
// how do I grant access to it from the outside?
// atm, poiController is the only one, who knows of
// AnimationState.h
ANIMATION_STATE animationStringToState(const std::string& letters)
{
    // ensure that we only grab the first 4 letters.
    if (letters == "ACCL")
    {
        return ANIMATION_STATE::ACCL;
    }
    else if (letters == "GYRO")
    {
        return ANIMATION_STATE::GYRO;
    }
    else if (letters == "CONS")
    {
        return ANIMATION_STATE::CONST;
    }
    else if (letters == "FLAS")
    {
        return ANIMATION_STATE::FLASH;
    }
    else if (letters == "RAIN")
    {
        return ANIMATION_STATE::RAINBOW;
    }

    return ANIMATION_STATE::CONST;
}
