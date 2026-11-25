#pragma once

#include <Arduino.h>

#include "Color.h"
#include "DisplayDriver.h"
#include "DisplayProfile.h"
#include "Input.h"
#include "MenuMetrics.h"
#include "Theme.h"

class ScreenManager;
class Display;
struct Screen2;

/** Replaces the built-in top bar. `screen` is never null. */
using TopBarRenderer = void (*)(Display &display, const Screen2 *screen);
/** Battery percentage for the top bar, 0-100. */
using BatteryProvider = int (*)();

// Drawing facade over one DisplayDriver, plus the frame loop. App code and the
// library's own chrome draw through this, never through the driver directly.
//
// Several displays can run side by side, each with its own ScreenManager, menu
// metrics and input sources. Render them all from one task.
class Display
{
public:
  /**
   * The display whose frame is running; the global `display` otherwise. Menus,
   * popups and widgets draw on and navigate this one.
   */
  static Display &active();

  /** Starts the driver and seeds theme, profile and menu metrics from the panel. */
  bool begin(DisplayDriver &driver, ScreenManager *screenManager = nullptr);
  void setScreenManager(ScreenManager *screenManager);
  ScreenManager &screens() const;

  /** Input for this display only, e.g. a touch panel on a second screen. */
  void addInput(InputSource &source);
  void removeInput(InputSource &source);
  InputSet &inputs() { return inputSet; }

  MenuStyle::Metrics &menuMetrics() { return metrics; }
  void setMenuMetrics(const MenuStyle::Metrics &m);

  bool isReady() const { return ready; }
  DisplayDriver *driver() const { return panel; }
  uint16_t width() const;
  uint16_t height() const;
  uint8_t colorDepth() const;

  const Theme &theme() const { return currentTheme; }
  void setTheme(const Theme &theme);
  const DisplayProfile &profile() const { return displayProfile; }
  void setProfile(const DisplayProfile &profile);

  /** Fills with `color`, or the theme background when kThemeColor. */
  void clearScreen(int32_t color = kThemeColor);
  void fillScreen(int32_t color = kThemeColor);
  void push();

  void setTextSize(uint8_t size);
  /** Prefer roles: a driver with discrete fonts (U8g2) cannot honour arbitrary sizes. */
  void setTextRole(TextRole role);
  void setTextDatum(uint8_t datum);
  void setTextColor(uint16_t color);
  void setTextColor(uint16_t fg, uint16_t bg);

  void drawString(const String &text, int32_t x, int32_t y);
  void drawString(const char *text, int32_t x, int32_t y);
  void drawPixel(int32_t x, int32_t y, uint16_t color);
  void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color);
  void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
  void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
  void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color);
  void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color);
  void drawSmoothArc(int32_t x, int32_t y, int32_t r, int32_t ir, int32_t startAngle, int32_t endAngle,
                     uint16_t fgColor, uint16_t bgColor, bool roundEnds);

  int16_t textWidth(const String &text);
  int16_t textWidth(const char *text);
  int16_t fontHeight();
  uint16_t color565(uint8_t r, uint8_t g, uint8_t b);

  /**
   * Word-wraps `text` into the box, honouring '\n'. The datum's column (left,
   * centre, right) aligns each line against `x`. Returns the lines drawn.
   */
  int wrapTextInBounds(const String &text, int x, int y, int width, int height, TextRole role, int gap,
                       uint16_t color, int textAlignment = Datum::TopCenter);
  int wrapTextInBounds(const String &text, int x, int y, int width, int height, int textSize, int gap,
                       uint16_t color, int textAlignment = Datum::TopCenter);

  /** Hide the top bar for the frame being drawn. Call from a screen's draw(). */
  void noTopBar();
  void drawTopBar();
  void setTopBarRenderer(TopBarRenderer renderer) { topBarRenderer = renderer; }
  /** nullptr hides the battery readout. */
  void setBatteryProvider(BatteryProvider provider) { batteryProvider = provider; }

  /** Redraw on the next `render`. */
  void invalidate() { dirty = true; }
  /**
   * Default for screens with `redrawMs == -1`. 0 (the default on buffered panels)
   * redraws every frame; unbuffered panels default to 1000.
   */
  void setIdleRedrawMs(uint32_t ms) { idleRedrawMs = ms; }

  /** One frame: input, screen update, and a redraw when due. True if it drew. */
  bool render();

private:
  int drawWrapped(const String &text, int x, int y, int width, int height, int gap, uint16_t color, int textAlignment);

  DisplayDriver *panel = nullptr;
  ScreenManager *screenManager = nullptr;
  bool ready = false;

  Theme currentTheme;
  DisplayProfile displayProfile;
  MenuStyle::Metrics metrics;
  InputSet inputSet;
  TopBarRenderer topBarRenderer = nullptr;
  BatteryProvider batteryProvider = nullptr;

  uint8_t datum = Datum::TopLeft;
  uint16_t textFg = Color::White;
  int32_t textBg = kThemeColor;

  bool showTopBar = true;
  bool dirty = true;
  uint32_t idleRedrawMs = 0;
  uint32_t lastDrawMs = 0;
};

extern Display display;
