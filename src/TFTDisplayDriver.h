#pragma once

// Compiled only when TFT_eSPI is present.
#if __has_include(<TFT_eSPI.h>)

#include <TFT_eSPI.h>

#include "DisplayDriver.h"
#include "FrameDiff.h"

// TFT_eSPI panel with a full-frame sprite, pushing only what changed (FrameDiff).
// If the sprite cannot be allocated (a large panel without PSRAM) it falls back
// to drawing straight to the panel.
//
// Subclass and override `initPanel`, `pushFrame` and `pushRegion` for panels
// TFT_eSPI cannot drive itself but whose frame a sprite can hold (RM67162 AMOLED).
class TFTDisplayDriver : public DisplayDriver
{
public:
  /** `width` and `height` are after `rotation`. */
  TFTDisplayDriver(uint16_t width, uint16_t height, uint8_t rotation = 0);

  bool begin() override;
  uint16_t width() const override { return w; }
  uint16_t height() const override { return h; }
  bool buffered() const override { return spriteReady; }
  void push() override;
  uint8_t lastPushPercent() const override { return pushedPercent; }
  /** Call before begin(). On by default; off always pushes the whole frame. */
  void setPartialPush(bool on) { partialPush = on; }

  void drawPixel(int32_t x, int32_t y, uint16_t color) override;
  void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) override;
  void fillScreen(uint16_t color) override;
  void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) override;
  void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) override;
  void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) override;
  void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color) override;
  void drawSmoothArc(int32_t x, int32_t y, int32_t r, int32_t ir, int32_t startAngle, int32_t endAngle,
                     uint16_t fgColor, uint16_t bgColor, bool roundEnds) override;

  void setFont(TextRole role) override;
  void setTextScale(uint8_t size) override;
  void drawText(const char *text, int32_t x, int32_t y, uint16_t fg, int32_t bg) override;
  int16_t textWidth(const char *text) override;
  int16_t fontHeight() override;

#ifdef LOAD_GFXFF
  /**
   * Font per role. Defaults: built-in font for Small, FreeSans 9 / Bold 12 /
   * Bold 18 for Body, Title and XLarge. nullptr gives the built-in font at `scale`.
   */
  void setRoleFont(TextRole role, const GFXfont *font, uint8_t scale = 1);
#else
  /** Without LOAD_GFXFF every role is the built-in font at an integer scale. */
  void setRoleScale(TextRole role, uint8_t scale);
#endif

  /** Direct access for app drawing. Taking one forgets the applied font, so the next role is reapplied. */
  TFT_eSPI &tft()
  {
    appTouched();
    return panel;
  }
  TFT_eSprite &sprite()
  {
    appTouched();
    return frame;
  }
  /** Where drawing lands: the sprite, or the panel in fallback mode. */
  TFT_eSPI &canvas()
  {
    appTouched();
    return target();
  }
  void resync() override { diff.invalidate(); }
  bool dimRect(int32_t x, int32_t y, int32_t w, int32_t h) override;

protected:
  TFT_eSPI &target() { return spriteReady ? static_cast<TFT_eSPI &>(frame) : panel; }
  // App code may change the font or set a TFT_eSPI viewport through the accessors:
  // reapply the font next time, and from then on fill through TFT_eSPI, which clips.
  void appTouched()
  {
    appliedFont = nullptr;
    rawFills = false;
  }
  virtual void initPanel();
  virtual void pushFrame();
  /** Send one changed region of the sprite to the same place on the panel. */
  virtual void pushRegion(int16_t x, int16_t y, int16_t w, int16_t h);
  /** False makes `begin` fail instead of falling back to direct drawing. */
  virtual bool allowDirectFallback() const { return true; }

  TFT_eSPI panel;
  TFT_eSprite frame;
  uint16_t w, h;
  uint8_t rotation;
  bool spriteReady = false;
  bool partialPush = true;
  uint8_t pushedPercent = 100;
  FrameDiff diff;
  struct RoleFont
  {
    const void *font; // const GFXfont * when LOAD_GFXFF
    uint8_t scale;
    int8_t ascent, descent; // glyph extent above/below the baseline, from the font
  };
  RoleFont roleFonts[static_cast<uint8_t>(TextRole::RoleCount)];
  const RoleFont *currentFont = nullptr; // role in use (metrics); nullptr after setTextScale
  const RoleFont *appliedFont = nullptr; // role last installed in TFT_eSPI, to skip rescans
  bool rawFills = true;                  // fills write the frame directly (no app viewport possible)
};

#endif
