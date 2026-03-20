#pragma once

#include "matrix.hpp"


enum class Color : unsigned int {
   Red   = 0xc93e34ff,  //0xc93e34
   Green = 0x288561ff,  //0x288561
   Blue  = 0x2b4da1ff,  //0x2b4da1
   White = 0xe0ded7ff,  //0xe0ded7
   Black = 0x0f1217ff,  //0x0f1217
   Yellow = 0xdeb245ff, //0xdeb245
   Purple = 0x6742c7ff, //0x6742c7
   Orange = 0xd68e3cff, //0xd68e3c
   Transperant = 0xffffff00,
};

/// @brief: Converts hexidecimal color to vec4 with float values from 0-1
vec4 hexColorToFloat(const Color hexValue);

/// @brief: Converts hexidecimal color to vec4 with values from 0-256
vec4 hexColorToRGB(const Color color);



