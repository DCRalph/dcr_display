#pragma once

#include <stdint.h>

// Solid fills straight into a 16-bit frame buffer, in aligned 32-bit words.
// Sprite and canvas fillRect write one pixel at a time (and TFT_eSprite then
// copies rows PSRAM to PSRAM); this is several times faster on large areas.
namespace FastFill
{
  /**
   * Fill w x h at x, y of a frame `stride` pixels wide with `value` exactly as the
   * buffer stores it (byte-swapped for TFT_eSprite). The caller clips to the frame.
   */
  inline void rect(uint16_t *frame, int32_t stride, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t value)
  {
    const uint32_t word = static_cast<uint32_t>(value) << 16 | value;
    for (int32_t row = 0; row < h; row++)
    {
      uint16_t *p = frame + static_cast<int32_t>(y + row) * stride + x;
      int32_t n = w;
      if (n > 0 && (reinterpret_cast<uintptr_t>(p) & 2))
      {
        *p++ = value;
        n--;
      }
      uint32_t *q = reinterpret_cast<uint32_t *>(p);
      for (int32_t i = n / 2; i > 0; i--)
        *q++ = word;
      if (n & 1)
        *reinterpret_cast<uint16_t *>(q) = value;
    }
  }

  /** Clip a rect to a w x h frame; false if nothing is left. */
  inline bool clip(int32_t &x, int32_t &y, int32_t &w, int32_t &h, int32_t frameW, int32_t frameH)
  {
    if (x < 0)
    {
      w += x;
      x = 0;
    }
    if (y < 0)
    {
      h += y;
      y = 0;
    }
    if (x + w > frameW)
      w = frameW - x;
    if (y + h > frameH)
      h = frameH - y;
    return w > 0 && h > 0;
  }
}
