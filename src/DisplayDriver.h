#pragma once

#include <Arduino.h>

#include "Color.h"
#include "DisplayProfile.h"

// One panel backend. A driver implements the pure virtuals; everything else has a
// default built from them, so a new panel needs about six methods.
//
// Coordinates are panel pixels after rotation. Text is drawn top-left anchored;
// `Display` resolves datums before calling `drawText`.
class DisplayDriver
{
public:
  virtual ~DisplayDriver() = default;

  /** Bring up the panel. False leaves the display unusable and is logged by `Display`. */
  virtual bool begin() = 0;

  virtual uint16_t width() const = 0;
  virtual uint16_t height() const = 0;
  virtual uint8_t colorDepth() const { return 16; }

  /**
   * True when drawing lands in an off-screen frame that `push` sends to the panel.
   * Unbuffered drivers draw straight to glass, so `Display` redraws them only on change.
   */
  virtual bool buffered() const { return true; }
  virtual void push() {}
  /** Share of the frame the last push() sent, 0-100. Partial-push drivers report less. */
  virtual uint8_t lastPushPercent() const { return 100; }
  /** Make the next push() send the whole frame, e.g. after drawing to the panel directly. */
  virtual void resync() {}
  /**
   * Halves the brightness of what is already drawn in the rect, behind an overlay.
   * False where the driver has no frame to darken (the overlay then draws as is).
   */
  virtual bool dimRect(int32_t x, int32_t y, int32_t w, int32_t h) { return false; }

  virtual void drawPixel(int32_t x, int32_t y, uint16_t color) = 0;
  virtual void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) = 0;

  virtual void fillScreen(uint16_t color);
  virtual void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color);
  virtual void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
  virtual void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color);
  virtual void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color);
  // Angles in degrees, 0 at six o'clock, clockwise (TFT_eSPI convention).
  virtual void drawSmoothArc(int32_t x, int32_t y, int32_t r, int32_t ir, int32_t startAngle, int32_t endAngle,
                             uint16_t fgColor, uint16_t bgColor, bool roundEnds);

  virtual void setFont(TextRole role) = 0;
  /** Legacy integer text size. Scaling backends override; others pick the nearest role. */
  virtual void setTextScale(uint8_t size) { setFont(roleForTextSize(size)); }
  /** `bg` is a colour, or kThemeColor (-1) for a transparent background. */
  virtual void drawText(const char *text, int32_t x, int32_t y, uint16_t fg, int32_t bg) = 0;
  virtual int16_t textWidth(const char *text) = 0;
  virtual int16_t fontHeight() = 0;

  virtual uint16_t color565(uint8_t r, uint8_t g, uint8_t b) { return Color::rgb(r, g, b); }

protected:
  void drawCircleQuadrants(int32_t cx, int32_t cy, int32_t r, uint8_t quadrants, uint16_t color);
  void fillCircleHelper(int32_t x0, int32_t y0, int32_t r, uint8_t corners, int32_t delta, uint16_t color);
};
