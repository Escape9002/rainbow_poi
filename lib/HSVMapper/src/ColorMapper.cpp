#include "ColorMapper.h"

HSV HSVMapper::colorMapper(uint16_t min, uint16_t max, uint32_t value){
    // avoid dvision by zero
    if(value == 0){
        return HSV{min,0,0};
    }

    // limit to the allowed maximum value
    if (value > 1000) {
        value = 1000;
    }

    //TODO should we catch the min,max outofrange errors?

    /**
     * Hue: [0 - 360]
     * 
     * hue = ((max - min) * value) / 1000
     */

     uint16_t hue = ((max-min) * value) / 1000;

     return HSV{hue, 0,0};
}

HSV HSVMapper::saturationMapper(uint16_t min, uint16_t max, uint32_t value){
    
}