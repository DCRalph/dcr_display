#pragma once

// Compiled only when U8g2 is present.
#if __has_include(<U8g2lib.h>)

#include <U8g2lib.h>

#include "DisplayDriver.h"

/** Font per TextRole. Defaults suit 128x64; replace before `Display::begin` for other sizes. */
struct U8g2FontSet
{
  const uint8_t *small = u8g2_font_6x10_tf;
  const uint8_t *body = u8g2_font_helvR08_tf;
  const uint8_t *title = u8g2_font_helvB10_tf;
  const uint8_t *xlarge = u8g2_font_helvB14_tf;

  const uint8_t *forRole(TextRole role) const;
};

// Any monochrome U8g2 panel, through the common `U8G2` base. Needs a
// full-buffer (`_F_`) class; panels over 255px also need -D U8G2_16BIT.
class U8g2DisplayDriver : public DisplayDriver
{
public:
  explicit U8g2DisplayDriver(U8G2 &panel) : u8g2(panel) {}

  bool begin() override;
  uint16_t width() const override { return u8g2.getDisplayWidth(); }
  uint16_t height() const override { return u8g2.getDisplayHeight(); }
  uint8_t colorDepth() const override { return 1; }
  void push() override { u8g2.sendBuffer(); }

  void drawPixel(int32_t x, int32_t y, uint16_t color) override;
  void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) override;
  void fillScreen(uint16_t color) override;
  void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) override;
  void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) override;
  void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) override;
  void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) override;

  void setFont(TextRole role) override;
  void drawText(const char *text, int32_t x, int32_t y, uint16_t fg, int32_t bg) override;
  int16_t textWidth(const char *text) override;
  int16_t fontHeight() override;

  /** Black or white by luma. */
  uint16_t color565(uint8_t r, uint8_t g, uint8_t b) override;

  U8G2 &panel() { return u8g2; }
  U8g2FontSet fonts;

private:
  U8G2 &u8g2;
};

#endif
