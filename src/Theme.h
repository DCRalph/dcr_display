#pragma once

#include "Color.h"

// Colours the library's own chrome uses: menus, top bar, popups, widgets.
// Flat surfaces on a dark background with one accent, in the style of current
// phone UIs. `mono` suits 1-bit panels, `light` a bright room.
struct Theme
{
  uint16_t background = Color::rgb(10, 12, 16);
  uint16_t surface = Color::rgb(28, 32, 40); // cards, popups, idle buttons and keys
  uint16_t border = Color::rgb(52, 58, 70);  // hairlines, outlines, switch tracks
  uint16_t text = Color::rgb(236, 240, 245);
  uint16_t mutedText = Color::rgb(140, 148, 162);
  uint16_t accent = Color::rgb(56, 128, 255);
  uint16_t onAccent = Color::White; // text and glyphs drawn on accent

  uint16_t highlight = Color::rgb(56, 128, 255); // active menu row
  uint16_t highlightText = Color::White;

  uint16_t topBar = Color::rgb(18, 21, 27);
  uint16_t topBarText = Color::rgb(236, 240, 245);

  uint16_t popupBackground = Color::rgb(28, 32, 40);
  uint16_t popupText = Color::rgb(200, 206, 216);
  uint16_t popupTitle = Color::rgb(236, 240, 245);
  uint16_t popupProgress = Color::rgb(56, 128, 255);

  uint16_t widget = Color::rgb(28, 32, 40); // idle button fill
  uint16_t widgetText = Color::rgb(236, 240, 245);
  uint16_t focus = Color::rgb(56, 128, 255); // focused button fill
  uint16_t focusText = Color::White;

  uint16_t success = Color::rgb(52, 199, 89);
  uint16_t warning = Color::rgb(255, 159, 10);
  uint16_t danger = Color::rgb(255, 69, 58);

  bool oneBit = false; // 1-bit: chrome uses outlines and inversion instead of fills

  static Theme dark() { return Theme{}; }

  static Theme light()
  {
    Theme t;
    t.background = Color::rgb(242, 243, 246);
    t.surface = Color::White;
    t.border = Color::rgb(214, 218, 225);
    t.text = Color::rgb(20, 22, 26);
    t.mutedText = Color::rgb(108, 115, 128);
    t.accent = t.highlight = t.focus = t.popupProgress = Color::rgb(0, 106, 255);
    t.topBar = Color::rgb(250, 250, 252);
    t.topBarText = t.popupTitle = t.widgetText = Color::rgb(20, 22, 26);
    t.popupBackground = t.widget = Color::White;
    t.popupText = Color::rgb(60, 66, 78);
    return t;
  }

  static Theme mono()
  {
    Theme t;
    t.background = t.surface = t.topBar = t.popupBackground = t.widget = Color::Black;
    t.border = t.text = t.mutedText = t.accent = t.highlight = t.focus = Color::White;
    t.topBarText = t.popupText = t.popupTitle = t.popupProgress = t.widgetText = Color::White;
    t.onAccent = t.highlightText = t.focusText = Color::Black;
    t.success = t.warning = t.danger = Color::White;
    t.oneBit = true;
    return t;
  }

  static Theme forColorDepth(uint8_t depth) { return depth == 1 ? mono() : dark(); }
};
