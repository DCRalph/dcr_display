// U8g2-backed driver, for monochrome OLED/LCD panels.
//
// Takes a `U8G2 &`, the common base every U8g2 panel class derives from, so one
// driver covers SH1106, SSD1306, SSD1309, ST7565, e-paper and the rest without
// the library knowing which is in use.
//
// Requirements:
//   * A full-buffer device (the `_F_` variants, e.g.
//     U8G2_SH1106_128X64_NONAME_F_HW_I2C). Paged variants (`_1_`, `_2_`) need a
//     firstPage/nextPage loop, which does not fit a draw-then-push facade;
//     `makeConfig` logs an error if it is handed one.
//   * Panels wider or taller than 255px additionally need U8G2_16BIT, since
//     u8g2_uint_t is otherwise 8 bits.
#pragma once

#if __has_include(<U8g2lib.h>)

#include <U8g2lib.h>

#include "Display.h"

/**
 * Fonts backing each TextRole. Defaults suit a 128x64 panel; assign your own
 * before `makeConfig` for a different size or a non-Latin character set.
 */
struct U8g2FontSet
{
  const uint8_t *small = u8g2_font_6x10_tf;
  const uint8_t *body = u8g2_font_profont12_tf;
  const uint8_t *title = u8g2_font_profont22_tf;
  const uint8_t *xlarge = u8g2_font_logisoso24_tr;

  const uint8_t *forRole(TextRole role) const;
};

struct U8g2DisplayDriverContext
{
  explicit U8g2DisplayDriverContext(U8G2 &panel) : u8g2(panel) {}

  U8G2 &u8g2;
  U8g2FontSet fonts;

  // Text state u8g2 does not track the way the facade expects.
  uint8_t datum = TL_DATUM;
  bool textOn = true; // false when text is drawn in the background colour
};

namespace U8g2DisplayDriver
{
  DisplayDriverOps makeOps();

  /**
   * Build a config for `Display::begin`. Width and height default to the
   * panel's own reported size, which is almost always what you want.
   */
  DisplayConfig makeConfig(U8g2DisplayDriverContext *context,
                           uint16_t width = 0,
                           uint16_t height = 0,
                           uint8_t rotation = 0);
}

#endif // __has_include(<U8g2lib.h>)
