#pragma once

// Compiled only when Adafruit_ILI9341 is present.
#if __has_include(<Adafruit_ILI9341.h>)

#include <Adafruit_ILI9341.h>
#include <SPI.h>

#include "DisplayDriver.h"

// SPI ILI9341 through Adafruit_GFX. With `buffered`, frames go to a 16-bit canvas
// (150 KB at 320x240, PSRAM) pushed in one burst; without, or if that allocation
// fails, it draws straight to the panel and `Display` redraws only on change.
// Owns the SPI bus; a touch controller (e.g. XPT2046) can share it via `spi()`.
class ILI9341DisplayDriver : public DisplayDriver
{
public:
  struct Pins
  {
    int8_t cs, dc, rst, sclk, miso, mosi;
  };

  ILI9341DisplayDriver(uint8_t spiBus, Pins pins, uint8_t rotation = 1, uint32_t spiFrequency = 27000000,
                       bool buffered = false);
  ~ILI9341DisplayDriver() override;

  bool begin() override;
  uint16_t width() const override;
  uint16_t height() const override;
  bool buffered() const override { return canvas != nullptr; }
  void push() override;

  void drawPixel(int32_t x, int32_t y, uint16_t color) override;
  void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) override;
  void fillScreen(uint16_t color) override;
  void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) override;
  void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) override;
  void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) override;
  void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) override;

  void setFont(TextRole role) override;
  void setTextScale(uint8_t size) override;
  void drawText(const char *text, int32_t x, int32_t y, uint16_t fg, int32_t bg) override;
  int16_t textWidth(const char *text) override;
  int16_t fontHeight() override;

  SPIClass &spi() { return bus; }
  Adafruit_ILI9341 &tft() { return panel; }
  /** Where drawing lands: the canvas when buffered, else the panel. */
  Adafruit_GFX &gfx() { return canvas ? static_cast<Adafruit_GFX &>(*canvas) : panel; }

private:
  SPIClass bus;
  Adafruit_ILI9341 panel;
  GFXcanvas16 *canvas = nullptr;
  bool wantBuffer;
  Pins pins;
  uint8_t rotation;
  uint32_t spiFrequency;
  uint8_t textScale = 2;
};

#endif
