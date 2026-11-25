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

  /**
   * A control face at x, y on a shadow of its own fill, theme().depth px lower;
   * keep that much free below. Pressed, the face sits on the shadow. Flat at depth 0
   * and on 1-bit panels.
   */
  void raised(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t fill, bool pressed = false);
  /** A minus or plus `size` px across centred on cx, cy, in bars that stay legible at any size. */
  void sign(int16_t cx, int16_t cy, int16_t size, bool plus, uint16_t color);
  /** A radio mark of radius r: a ring, filled with a dot when on. */
  void radio(int16_t cx, int16_t cy, int16_t r, bool on, uint16_t ring, uint16_t dot, uint16_t background);
}
