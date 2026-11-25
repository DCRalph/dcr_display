#pragma once

#include "Color.h"

// How a menu lays out its rows.
enum class MenuLook : uint8_t
{
  Theme,   // use the theme's default
  Flat,    // rows straight on the background, the selected one highlighted
  Grouped, // rows share one raised card, hairline separated
};

// How a number item looks while it is being edited.
enum class NumberEdit : uint8_t
{
  Theme,
  Inline, // a raised - | value | + pill at the row's end
  Expand, // the row grows: large value between big -/+ keys, over a min-max bar
};

// How a select item looks while it is being edited.
enum class SelectEdit : uint8_t
{
  Theme,
  Inline, // a raised < | value | > pill at the row's end
  Sheet,  // a sheet of every option rises from the bottom, the current one marked
  Expand, // the row grows into a segmented control of every option
};

// Colours and component styles the library's own chrome uses: menus, top bar,
// popups, widgets. Copy a preset and change fields to make your own.
// `mono` suits 1-bit panels, `bactosure` and `midnight` give controls depth.
struct Theme
{
  uint16_t background = Color::rgb(10, 12, 16);
  uint16_t surface = Color::rgb(28, 32, 40); // cards, popups, sheets, idle keys
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

  uint16_t widget = Color::rgb(28, 32, 40); // idle button fill, -/+ keys
  uint16_t widgetText = Color::rgb(236, 240, 245);
  uint16_t focus = Color::rgb(56, 128, 255); // focused button fill
  uint16_t focusText = Color::White;

  uint16_t switchOn = Color::rgb(56, 128, 255); // switch track when on
  uint16_t switchKnob = Color::White;

  uint16_t success = Color::rgb(52, 199, 89);
  uint16_t warning = Color::rgb(255, 159, 10);
  uint16_t danger = Color::rgb(255, 69, 58);

  // Raised controls (buttons, keys, cards, popups) sit `depth` px above a shadow
  // of their own fill at `shadowPercent` brightness; pressing moves them onto it.
  // 0 draws everything flat. Ignored on 1-bit panels.
  uint8_t depth = 0;
  uint8_t shadowPercent = 55;

  bool fillTopBar = false; // fill the bar with `topBar` even where the profile leaves it bare
  bool oneBit = false;     // 1-bit: chrome uses outlines and inversion instead of fills

  // Defaults for items and menus that leave their own style at `Theme`.
  MenuLook menuLook = MenuLook::Grouped;
  NumberEdit numberEdit = NumberEdit::Expand;
  SelectEdit selectEdit = SelectEdit::Expand;

  /** Shadow under a raised surface filled with `fill`. */
  uint16_t shadowOf(uint16_t fill) const { return Color::shade(fill, shadowPercent); }

  static Theme dark() { return Theme{}; }

  static Theme light()
  {
    Theme t;
    t.background = Color::rgb(236, 239, 244);
    t.surface = Color::White;
    t.border = Color::rgb(214, 218, 225);
    t.text = Color::rgb(20, 22, 26);
    t.mutedText = Color::rgb(108, 115, 128);
    t.accent = t.highlight = t.focus = t.popupProgress = t.switchOn = Color::rgb(0, 106, 255);
    t.topBar = Color::rgb(250, 250, 252);
    t.topBarText = t.popupTitle = t.widgetText = Color::rgb(20, 22, 26);
    t.popupBackground = Color::White;
    t.widget = Color::rgb(228, 232, 238);
    t.popupText = Color::rgb(60, 66, 78);
    return t;
  }

  // Bactosure's bioreactor: dark-blue bar, pastel raised controls with dark-blue
  // labels, solid drop shadows (its AppUI / LightStyle).
  static Theme bactosure()
  {
    Theme t;
    const uint16_t navy = Color::rgb(41, 57, 99);    // COL_DARK_BLUE
    const uint16_t sky = Color::rgb(90, 222, 255);   // COL_LIGHT_BLUE
    const uint16_t lime = Color::rgb(173, 255, 66);  // COL_LIGHT_GREEN
    t.background = Color::rgb(232, 238, 246);
    t.surface = t.popupBackground = Color::White;
    t.border = Color::rgb(140, 158, 186);
    t.text = t.popupTitle = t.widgetText = navy;
    t.mutedText = t.popupText = Color::rgb(88, 102, 126);
    t.accent = t.popupProgress = navy;
    t.onAccent = Color::White;
    t.highlight = t.widget = t.focus = sky;
    t.highlightText = t.focusText = navy;
    t.topBar = navy;
    t.topBarText = Color::White;
    t.switchOn = lime;
    t.switchKnob = navy;
    t.success = Color::rgb(22, 130, 70);
    t.warning = Color::rgb(176, 106, 8);
    t.danger = Color::rgb(186, 32, 40);
    t.depth = 3;
    t.fillTopBar = true;
    return t;
  }

  // The bioreactor's engineering screens (its DarkStyle): black, slate raised controls.
  static Theme midnight()
  {
    Theme t;
    t.background = Color::Black;
    t.surface = t.popupBackground = Color::rgb(22, 25, 32);
    t.border = Color::rgb(56, 61, 74);
    t.text = t.popupTitle = t.widgetText = Color::White;
    t.mutedText = Color::rgb(148, 156, 172);
    t.popupText = Color::rgb(200, 206, 216);
    t.accent = t.highlight = t.focus = t.popupProgress = Color::rgb(88, 158, 240);
    t.widget = Color::rgb(56, 88, 152);
    t.topBar = Color::rgb(22, 25, 32);
    t.topBarText = Color::White;
    t.switchOn = Color::rgb(58, 198, 118);
    t.success = Color::rgb(58, 198, 118);
    t.warning = Color::rgb(240, 176, 48);
    t.danger = Color::rgb(232, 78, 78);
    t.depth = 3;
    t.shadowPercent = 45;
    t.fillTopBar = true;
    return t;
  }

  // Black and white with a yellow focus, for sunlight and small panels.
  static Theme contrast()
  {
    Theme t;
    const uint16_t yellow = Color::rgb(255, 214, 10);
    t.background = t.surface = t.popupBackground = t.topBar = Color::Black;
    t.border = t.text = t.popupTitle = t.popupText = t.topBarText = t.widgetText = Color::White;
    t.mutedText = Color::rgb(200, 200, 200);
    t.accent = t.highlight = t.focus = t.popupProgress = t.switchOn = yellow;
    t.onAccent = t.highlightText = t.focusText = t.switchKnob = Color::Black;
    t.widget = Color::rgb(40, 40, 40);
    t.success = Color::rgb(48, 209, 88);
    t.warning = yellow;
    t.danger = Color::rgb(255, 69, 58);
    return t;
  }

  static Theme mono()
  {
    Theme t;
    t.background = t.surface = t.topBar = t.popupBackground = t.widget = Color::Black;
    t.border = t.text = t.mutedText = t.accent = t.highlight = t.focus = t.switchOn = Color::White;
    t.topBarText = t.popupText = t.popupTitle = t.popupProgress = t.widgetText = Color::White;
    t.onAccent = t.highlightText = t.focusText = t.switchKnob = Color::Black;
    t.success = t.warning = t.danger = Color::White;
    t.oneBit = true;
    // Small 1-bit panels keep the compact list and in-row editing.
    t.menuLook = MenuLook::Flat;
    t.numberEdit = NumberEdit::Inline;
    t.selectEdit = SelectEdit::Inline;
    return t;
  }

  static Theme forColorDepth(uint8_t depth) { return depth == 1 ? mono() : dark(); }

  MenuLook resolve(MenuLook look) const { return look == MenuLook::Theme ? menuLook : look; }
  NumberEdit resolve(NumberEdit edit) const { return edit == NumberEdit::Theme ? numberEdit : edit; }
  SelectEdit resolve(SelectEdit edit) const { return edit == SelectEdit::Theme ? selectEdit : edit; }
};
