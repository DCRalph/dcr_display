#include "DisplayProfile.h"

DisplayProfile DisplayProfile::forPanel(uint16_t width, uint16_t height, uint8_t colorDepth) noexcept
{
  DisplayProfile p;

  if (height <= 80)
  {
    // Small mono OLED territory (128x64 and friends). Every pixel counts, and a
    // filled bar on a 1-bit panel is a solid white slab, so underline instead.
    p.topBarHeight = 12;
    p.topBarFilled = false;
    p.topBarUnderline = true;
    p.topBarPaddingX = 0;
    p.topBarTextRole = TextRole::Small;

    p.popupMarginX = 6;
    p.popupMarginY = 6;
    p.popupPadding = 4;
    p.popupRadius = 3;
    p.popupLineGap = 1;
    p.popupTitleRole = TextRole::Small;
    p.popupMessageRole = TextRole::Small;
    p.popupProgressHeight = 3;
  }
  else if (height <= 160)
  {
    p.topBarHeight = 16;
    p.topBarPaddingX = 6;
    p.topBarTextRole = TextRole::Small;

    p.popupMarginX = 16;
    p.popupMarginY = 12;
    p.popupPadding = 6;
    p.popupRadius = 6;
    p.popupTitleRole = TextRole::Body;
    p.popupMessageRole = TextRole::Small;
    p.popupProgressHeight = 6;
  }

  // A 1-bit panel has no colour to fill with whatever the screen requested.
  if (colorDepth == 1)
  {
    p.topBarFilled = false;
    p.topBarUnderline = true;
  }

  // Never let the chrome swallow a narrow panel.
  const uint16_t maxMarginX = width / 4;
  if (p.popupMarginX > maxMarginX)
    p.popupMarginX = maxMarginX;

  const uint16_t maxMarginY = height / 4;
  if (p.popupMarginY > maxMarginY)
    p.popupMarginY = maxMarginY;

  return p;
}
