#pragma once

#include <Arduino.h>

struct Rect;

// Shared pieces of the library's look, drawn on Display::active() with its theme.
namespace Draw
{
  /** Corner radius for a control `h` tall: pill-ish on colour, tight on 1-bit. */
  int16_t radius(int16_t h);
  /** Switch `w` x `h` at x, y. `on` is 0-1 so the knob can slide. `onFill` marks a row already in the accent. */
  void switchGlyph(int16_t x, int16_t y, int16_t w, int16_t h, float on, bool onFill);
  /** Chevron `size` px tall centred on cy, pointing right or left, its tip at x. */
  void chevron(int16_t x, int16_t cy, int16_t size, bool right, uint16_t color);
  /** Battery outline with a fill for `percent`, nub on the right, in w x h. */
  void battery(int16_t x, int16_t y, int16_t w, int16_t h, int percent, uint16_t color);
  /** A ring 2 px outside `r`, for keyboard and button focus. */
  void focusRing(const Rect &r, int16_t radius);
}
