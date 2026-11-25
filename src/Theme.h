#pragma once

#include "Color.h"

// Colours the library's own chrome uses: menus, top bar, popups, widgets.
struct Theme
{
  uint16_t background = Color::Black;
  uint16_t text = Color::White;
  uint16_t mutedText = Color::LightGrey;
  uint16_t accent = Color::Blue;

  uint16_t highlight = Color::White; // active menu row and focused widget fill
  uint16_t highlightText = Color::Black;

  uint16_t topBar = Color::Blue;
  uint16_t topBarText = Color::Black;

  uint16_t popupBackground = Color::White;
  uint16_t popupText = Color::Black;
  uint16_t popupTitle = Color::Blue;
  uint16_t popupProgress = Color::Red;

  uint16_t widget = Color::DarkGrey; // unfocused button fill
  uint16_t widgetText = Color::White;
  uint16_t focus = Color::Blue; // focused button fill
  uint16_t focusText = Color::White;

  static Theme dark() { return Theme{}; }

  // 1-bit panels: every colour is black or white, so accents become ink.
  static Theme mono()
  {
    Theme t;
    t.mutedText = Color::White;
    t.accent = Color::White;
    t.topBar = Color::White;
    t.popupTitle = Color::Black;
    t.popupProgress = Color::Black;
    t.widget = Color::Black;
    t.focus = Color::White;
    t.focusText = Color::Black;
    return t;
  }

  static Theme forColorDepth(uint8_t depth) { return depth == 1 ? mono() : dark(); }
};
