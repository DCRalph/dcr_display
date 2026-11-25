#pragma once

#include <stdint.h>

// RGB565 colours. Values match TFT_eSPI's TFT_* constants, so either spelling works.
namespace Color
{
  constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b)
  {
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
  }

  // Rec. 601 luma of an RGB565 colour, 0-255.
  constexpr uint8_t luma(uint16_t c)
  {
    return static_cast<uint8_t>((((c >> 11) & 0x1F) * 255 / 31 * 299 +
                                 ((c >> 5) & 0x3F) * 255 / 63 * 587 +
                                 (c & 0x1F) * 255 / 31 * 114) /
                                1000);
  }

  // Mix of a and b; t = 0 gives a, 255 gives b.
  constexpr uint16_t blend(uint16_t a, uint16_t b, uint8_t t)
  {
    return static_cast<uint16_t>(((((a >> 11) & 0x1F) * (255 - t) + ((b >> 11) & 0x1F) * t) / 255) << 11 |
                                 ((((a >> 5) & 0x3F) * (255 - t) + ((b >> 5) & 0x3F) * t) / 255) << 5 |
                                 (((a & 0x1F) * (255 - t) + (b & 0x1F) * t) / 255));
  }

  // The colour at `percent` of its brightness, for shadows and pressed states.
  constexpr uint16_t shade(uint16_t c, uint8_t percent)
  {
    return static_cast<uint16_t>(((((c >> 11) & 0x1F) * percent / 100) << 11) |
                                 ((((c >> 5) & 0x3F) * percent / 100) << 5) |
                                 ((c & 0x1F) * percent / 100));
  }

  // `c` pushed `percent` further from `from`, channel by channel: more contrast
  // against `from` with the hue roughly kept.
  constexpr uint8_t awayChannel(int c, int from, uint8_t percent)
  {
    const int v = c + (c - from) * percent / 100;
    return static_cast<uint8_t>(v < 0 ? 0 : v > 255 ? 255 : v);
  }
  constexpr uint16_t away(uint16_t c, uint16_t from, uint8_t percent)
  {
    return rgb(awayChannel(((c >> 11) & 0x1F) * 255 / 31, ((from >> 11) & 0x1F) * 255 / 31, percent),
               awayChannel(((c >> 5) & 0x3F) * 255 / 63, ((from >> 5) & 0x3F) * 255 / 63, percent),
               awayChannel((c & 0x1F) * 255 / 31, (from & 0x1F) * 255 / 31, percent));
  }

  constexpr uint16_t Black = 0x0000;
  constexpr uint16_t White = 0xFFFF;
  constexpr uint16_t Red = 0xF800;
  constexpr uint16_t Green = 0x07E0;
  constexpr uint16_t Blue = 0x001F;
  constexpr uint16_t Cyan = 0x07FF;
  constexpr uint16_t Magenta = 0xF81F;
  constexpr uint16_t Yellow = 0xFFE0;
  constexpr uint16_t Orange = 0xFDA0;
  constexpr uint16_t Purple = 0x780F;
  constexpr uint16_t Navy = 0x000F;
  constexpr uint16_t DarkGrey = 0x7BEF;
  constexpr uint16_t LightGrey = 0xD69A;
}

// Text anchor points. Values match TFT_eSPI's *_DATUM constants.
namespace Datum
{
  constexpr uint8_t TopLeft = 0;
  constexpr uint8_t TopCenter = 1;
  constexpr uint8_t TopRight = 2;
  constexpr uint8_t MiddleLeft = 3;
  constexpr uint8_t MiddleCenter = 4;
  constexpr uint8_t MiddleRight = 5;
  constexpr uint8_t BottomLeft = 6;
  constexpr uint8_t BottomCenter = 7;
  constexpr uint8_t BottomRight = 8;
}

// Colour slot that defers to the active Theme.
constexpr int32_t kThemeColor = -1;
