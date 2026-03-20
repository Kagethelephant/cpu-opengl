#include "data.hpp"


vec4 hexColorToFloat(const Color color) {
   vec4 floatColor; // (0-1)
   unsigned int hexValue = static_cast<unsigned int>(color);
   // Extract the R, G, B, and A bytes using bit shifting and masking
   floatColor[0] = ((hexValue >> 24) & 0xFF)/256.0f; // Extract the AA byte
   floatColor[1] = ((hexValue >> 16) & 0xFF)/256.0f; // Extract the RR byte
   floatColor[2] = ((hexValue >> 8) & 0xFF)/256.0f;  // Extract the GG byte
   floatColor[3] = ((hexValue) & 0xFF)/256.0f;       // Extract the BB byte
   return floatColor;
}


vec4 hexColorToRGB(const Color color) {
   vec4 rgbColor; // (0-256)
   unsigned int hexValue = static_cast<unsigned int>(color);
   // Extract the R, G, B, and A bytes using bit shifting and masking
   rgbColor[0] = ((hexValue >> 24) & 0xFF); // Extract the AA byte
   rgbColor[1] = ((hexValue >> 16) & 0xFF); // Extract the RR byte
   rgbColor[2] = ((hexValue >> 8) & 0xFF);  // Extract the GG byte
   rgbColor[3] = ((hexValue) & 0xFF);       // Extract the BB byte
   return rgbColor;
}


