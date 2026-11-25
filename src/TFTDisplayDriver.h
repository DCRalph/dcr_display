#pragma once

// Compiled only when TFT_eSPI is present.
#if __has_include(<TFT_eSPI.h>)

#include <TFT_eSPI.h>

#include "DisplayDriver.h"

// TFT_eSPI panel with a full-frame sprite. If the sprite cannot be allocated
// (a large panel without PSRAM) it falls back to drawing straight to the panel.
//
// Subclass and override `initPanel`/`pushFrame` for panels TFT_eSPI cannot
// drive itself but whose frame a sprite can hold (e.g. an RM67162 AMOLED).
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

  /** Text scale (built-in font) per role. Defaults 1/2/3/4. */
  void setRoleScale(TextRole role, uint8_t scale);

  TFT_eSPI &tft() { return panel; }
  TFT_eSprite &sprite() { return frame; }
  /** Where drawing lands: the sprite, or the panel in fallback mode. */
  TFT_eSPI &canvas() { return spriteReady ? static_cast<TFT_eSPI &>(frame) : panel; }

protected:
  virtual void initPanel();
  virtual void pushFrame();
  /** False makes `begin` fail instead of falling back to direct drawing. */
  virtual bool allowDirectFallback() const { return true; }

  TFT_eSPI panel;
  TFT_eSprite frame;
  uint16_t w, h;
  uint8_t rotation;
  bool spriteReady = false;
  uint8_t roleScale[static_cast<uint8_t>(TextRole::RoleCount)] = {1, 2, 3, 4};
};

#endif
