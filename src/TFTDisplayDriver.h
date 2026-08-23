// TFT_eSPI sprite-backed display driver callbacks.
#pragma once

// Compiled only when the backing panel library is present, so projects that use a
// different driver (e.g. U8g2) need not depend on TFT_eSPI.
#if __has_include(<TFT_eSPI.h>)

#include <TFT_eSPI.h>

#include "Display.h"

struct TFTDisplayDriverContext
{
  TFT_eSPI tft;
  TFT_eSprite sprite;

  TFTDisplayDriverContext() : tft(), sprite(&tft) {}
};

namespace TFTDisplayDriver
{
  DisplayDriverOps makeOps();
  DisplayConfig makeConfig(TFTDisplayDriverContext *context, uint16_t width, uint16_t height, uint8_t rotation, uint8_t colorDepth = 16);
}

#endif // __has_include(<TFT_eSPI.h>)
