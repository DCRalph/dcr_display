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
// push() sends only the 8-pixel pages that changed: a full 128x64 frame is
// about 25 ms of I2C at 400 kHz, a static screen sends nothing.
class U8g2DisplayDriver : public DisplayDriver
{
public:
  explicit U8g2DisplayDriver(U8G2 &panel) : u8g2(panel) {}
  U8g2DisplayDriver(const U8g2DisplayDriver &) = delete;
  U8g2DisplayDriver &operator=(const U8g2DisplayDriver &) = delete;
  ~U8g2DisplayDriver() override;

  bool begin() override;
  uint16_t width() const override { return u8g2.getDisplayWidth(); }
  uint16_t height() const override { return u8g2.getDisplayHeight(); }
  uint8_t colorDepth() const override { return 1; }
  void push() override;
  uint8_t lastPushPercent() const override { return pushedPercent; }
  /** Call before begin(). On by default; off always sends the whole buffer. */
  void setPartialPush(bool on) { partialPush = on; }
  /** Call after clearDisplay(), sleep/wake or anything else that changes the panel outside push(). */
  void resync() override { panelInSync = false; }

  void setClip(int32_t x, int32_t y, int32_t w, int32_t h) override;
  void clearClip() override;
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
  uint8_t *shown = nullptr; // copy of the buffer the panel shows, for page diffs
  bool partialPush = true;
  bool panelInSync = false;
  uint8_t pushedPercent = 100;
};

#endif
