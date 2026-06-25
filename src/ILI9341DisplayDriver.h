// Adafruit_ILI9341 (SPI) adapter for the dcr_display `Display` facade.
//
// Mirrors TFTDisplayDriver but targets an SPI ILI9341 panel through Adafruit_GFX,
// so a second (or alternative) Display can be driven with the exact same drawing
// API as the built-in TFT_eSPI panel.
//
// Differences from the TFT_eSPI driver:
//   * Renders directly to the panel (no sprite/double-buffer), so push() is a
//     no-op and callers should draw on-demand rather than re-rendering each frame.
//   * Adafruit_GFX has no text-datum concept, so datums are emulated with
//     getTextBounds().
//   * The SPI bus is owned here and exposed via spi(), letting a touch controller
//     (e.g. XPT2046) share it.
#pragma once

#include <SPI.h>
#include <Adafruit_ILI9341.h>

#include "Display.h"

struct ILI9341DisplayDriverContext
{
  SPIClass spi;
  Adafruit_ILI9341 tft;

  int8_t pinCS, pinDC, pinRST, pinSCLK, pinMISO, pinMOSI;
  uint32_t spiFrequency;

  // Emulated text state (Adafruit_GFX tracks size/colour itself, but not datum).
  uint8_t datum;    // TFT_eSPI-style datum (TL_DATUM, MC_DATUM, ...)
  uint8_t textSize; // tracked so fontHeight() can report a height

  ILI9341DisplayDriverContext(uint8_t spiBus, int8_t cs, int8_t dc, int8_t rst,
                              int8_t sclk, int8_t miso, int8_t mosi,
                              uint32_t freq = 27000000)
      : spi(spiBus), tft(&spi, dc, cs, rst),
        pinCS(cs), pinDC(dc), pinRST(rst),
        pinSCLK(sclk), pinMISO(miso), pinMOSI(mosi),
        spiFrequency(freq), datum(0), textSize(1) {}
};

namespace ILI9341DisplayDriver
{
  DisplayDriverOps makeOps();
  DisplayConfig makeConfig(ILI9341DisplayDriverContext *context, uint16_t width,
                           uint16_t height, uint8_t rotation, uint8_t colorDepth = 16);

  // The driver-owned SPI bus, so a touch controller can share the same wiring.
  SPIClass &spi(ILI9341DisplayDriverContext *context);
}
