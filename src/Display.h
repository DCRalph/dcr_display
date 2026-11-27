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

// Frame timing over the last full second, from Display::stats(). Times in microseconds.
struct FrameStats
{
  uint16_t fps = 0;         // frames drawn and pushed
  uint16_t renders = 0;     // render() calls, drawn or not
  uint32_t avgDrawUs = 0;   // building a frame: screen, top bar, popup (drawn frames)
  uint32_t avgPushUs = 0;   // sending it to the panel (drawn frames)
  uint8_t avgPushedPercent = 0; // share of each frame actually sent; partial pushes send less
  uint32_t maxFrameUs = 0;  // worst draw + push
  uint32_t avgUpdateUs = 0; // input and screen update (every render)
  uint8_t loadPercent = 0;  // share of the second spent inside render()
};

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
  /** Panel trait: every theme set from now (and the current one) gets Theme::withContrast(percent). */
  void setContrastBoost(uint8_t percent);
  const DisplayProfile &profile() const { return displayProfile; }
  void setProfile(const DisplayProfile &profile);

  /**
   * Fills with `color`, or the theme background when kThemeColor. A screen whose
   * draw() starts with this skips the library's own pre-clear on later frames.
   */
  void clearScreen(int32_t color = kThemeColor);
  /** Darkens what is drawn in the rect, behind a sheet or popup; no-op on unbuffered and 1-bit panels. */
  bool dimRect(int32_t x, int32_t y, int32_t w, int32_t h);
  /** Drawing outside the rect is dropped until clearClip(), where the driver can clip. */
  void setClip(int32_t x, int32_t y, int32_t w, int32_t h);
  void clearClip();
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
  /** Dim the top bar too, once drawn this frame: for an overlay the screen draws, like a sheet. */
  void dimTopBar();
  void drawTopBar();
  void setTopBarRenderer(TopBarRenderer renderer) { topBarRenderer = renderer; }
  /** nullptr hides the battery readout. */
  void setBatteryProvider(BatteryProvider provider)
  {
    batteryProvider = provider;
    batteryReadMs = 0;
  }

  /** Redraw on the next `render`. */
  void invalidate() { dirty = true; }

  /** Tweens animate when on (the default) and the panel is buffered; off, they jump. */
  void setAnimations(bool on) { animations = on; }
  bool animationsEnabled() const { return animations && panel && panel->buffered(); }
  /** Redraw every frame for the next `ms`; Tween::start calls this. */
  void animateFor(uint32_t ms);
  /**
   * Default for screens with `redrawMs == -1`. 0 (the default on buffered panels)
   * redraws every frame; unbuffered panels default to 1000.
   */
  void setIdleRedrawMs(uint32_t ms) { idleRedrawMs = ms; }

  /** One frame: input, screen update, and a redraw when due. True if it drew. */
  bool render();

  /** Timing for the last full second of render() calls. */
  const FrameStats &stats() const { return frameStats; }

private:
  int drawWrapped(const String &text, int x, int y, int width, int height, int gap, uint16_t color, int textAlignment);
  int drawWrappedLong(const String &text, int x, int y, int width, int height, int gap, uint16_t color, int textAlignment);

  DisplayDriver *panel = nullptr;
  ScreenManager *screenManager = nullptr;
  bool ready = false;

  Theme currentTheme;
  Theme baseTheme; // as passed to setTheme, before the contrast boost
  uint8_t contrastBoost = 0;
  DisplayProfile displayProfile;
  MenuStyle::Metrics metrics;
  InputSet inputSet;
  TopBarRenderer topBarRenderer = nullptr;
  BatteryProvider batteryProvider = nullptr;
  char batteryText[8] = {0};
  int batteryPercent = 0;
  uint32_t batteryReadMs = 0; // the provider is read once a second, not every frame

  // Pre-clear skipping: a screen that filled the frame last time is not cleared first.
  const Screen2 *lastDrawnScreen = nullptr;
  bool screenFillsFrame = false;
  bool drawingScreen = false;
  bool sawScreenFill = false;

  uint8_t datum = Datum::TopLeft;
  uint16_t textFg = Color::White;
  int32_t textBg = kThemeColor;

  bool showTopBar = true;
  bool dimBar = false;
  bool dirty = true;
  bool animations = true;
  bool animationPending = false; // a tween is running, or its final frame is not drawn yet
  uint32_t animateUntilMs = 0;
  uint32_t idleRedrawMs = 0;
  uint32_t lastDrawMs = 0;

  struct
  {
    uint32_t windowStartMs = 0;
    uint16_t frames = 0, renders = 0;
    uint64_t drawUs = 0, pushUs = 0, updateUs = 0, renderUs = 0;
    uint32_t pushedPercent = 0;
    uint32_t maxFrameUs = 0;
  } frameAccum;
  FrameStats frameStats;
  void accumulateFrame(bool drew, uint32_t drawUs, uint32_t pushUs, uint32_t updateUs, uint32_t renderUs);
};

extern Display display;
