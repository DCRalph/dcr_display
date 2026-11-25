#pragma once

#include <Arduino.h>

#include "DisplayProfile.h"

namespace MenuStyle
{
  struct DrawStyle
  {
    TextRole role;
    int lineHeight;
    uint8_t radius;
  };

  /** Row geometry per density preset. `Display::begin` installs `forPanel`; override after. */
  struct Metrics
  {
    uint16_t contentTopY = 22; // first row's y, i.e. the top bar height
    // Panel height assumed before `Display::begin` (static menus are built first).
    uint16_t fallbackDisplayHeight = 240;

    DrawStyle small = {TextRole::Small, 12, 2};
    DrawStyle medium = {TextRole::Body, 20, 3};
    DrawStyle large = {TextRole::Title, 28, 4};

    // Rows grow to at least this while a pointer source is attached.
    uint16_t minTouchRowHeight = 32;

    static Metrics forPanel(uint16_t width, uint16_t height, uint8_t colorDepth) noexcept;
  };
}
