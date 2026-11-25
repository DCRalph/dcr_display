#pragma once

#include <Arduino.h>

#include "Color.h"

struct Screen2;
using ScreenBackgroundStep = void (*)(const Screen2 *screen);

// A worker the ScreenManager runs while its screen is current, with a watchdog
// that stops it if one step overruns.
struct ScreenBackgroundTaskConfig
{
  ScreenBackgroundStep step = nullptr;
  const char *name = nullptr;
  uint32_t stackDepth = 4096;
  uint32_t loopDelayMs = 1000;
  uint32_t watchdogTimeoutMs = 15000;
};

/** Screen descriptor. Field order is fixed so designated initialisers keep compiling. */
struct Screen2
{
  const char *name;
  void (*draw)();
  void (*update)();
  void (*onEnter)();
  void (*onExit)();
  int32_t topBarColor = kThemeColor;     // RGB565, or kThemeColor
  int32_t topBarTextColor = kThemeColor; // RGB565, or kThemeColor
  ScreenBackgroundTaskConfig backgroundTask{};
  // Max ms between redraws when nothing changed. -1 uses Display::setIdleRedrawMs,
  // 0 redraws every frame. Input, popups and screen changes always redraw.
  int32_t redrawMs = -1;
};
