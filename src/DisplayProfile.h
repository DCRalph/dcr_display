// Panel-derived geometry for the library's own chrome. `Display::begin` applies
// `DisplayProfile::forPanel`; override with `setProfile` after begin().
#pragma once

#include <Arduino.h>

/**
 * Logical text sizes. Drivers map these to whatever their backend does natively:
 * TFT_eSPI scales one bitmap font by an integer, U8g2 selects a discrete font.
 * Library chrome always asks for a role, never a raw size, so it renders
 * sensibly on both.
 */
enum class TextRole : uint8_t
{
  Small = 0, // dense lists, footnotes
  Body,      // default running text
  Title,     // headings, popup titles
  XLarge,    // large readouts

  RoleCount
};

/** Integer scale a scaling driver (TFT_eSPI, Adafruit_GFX) uses for each role. */
constexpr uint8_t defaultTextScaleFor(TextRole role) noexcept
{
  switch (role)
  {
  case TextRole::Small:
    return 1;
  case TextRole::Body:
    return 2;
  case TextRole::Title:
    return 3;
  case TextRole::XLarge:
    return 4;
  default:
    return 2;
  }
}

/** Nearest role for a legacy `setTextSize` value, for drivers with discrete fonts. */
constexpr TextRole roleForTextSize(uint8_t size) noexcept
{
  return size <= 1   ? TextRole::Small
         : size == 2 ? TextRole::Body
         : size == 3 ? TextRole::Title
                     : TextRole::XLarge;
}

/** Geometry for the built-in top bar and popups. All values are pixels. */
struct DisplayProfile
{
  // --- Top bar ---
  uint16_t topBarHeight = 20;
  /** Fill the bar with the screen's top bar colour. Mono panels underline instead. */
  bool topBarFilled = true;
  /** Drawn along the bottom edge of the bar when `topBarFilled` is false. */
  bool topBarUnderline = false;
  uint16_t topBarPaddingX = 10;
  TextRole topBarTextRole = TextRole::Body;

  // --- Popups ---
  uint16_t popupMarginX = 30;
  uint16_t popupMarginY = 20;
  uint16_t popupPadding = 10;
  uint8_t popupRadius = 10;
  uint16_t popupLineGap = 3;
  TextRole popupTitleRole = TextRole::Title;
  TextRole popupMessageRole = TextRole::Body;
  /** Height of the AutoClosePopup countdown bar; 0 hides it. */
  uint16_t popupProgressHeight = 10;

  /**
   * Defaults for a panel of this size. Three bands: small mono-style panels
   * (<=80px tall), mid-size colour panels, and everything larger, which keeps
   * the geometry the library shipped with.
   */
  static DisplayProfile forPanel(uint16_t width, uint16_t height, uint8_t colorDepth) noexcept;
};
