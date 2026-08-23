#include "Popup.h"
#include "MenuInput.h"
#include "ScreenManager.h"

namespace
{
  /**
   * Popup box in panel coordinates. Derived from the display profile rather than
   * hardcoded, so the box stays inside the panel and the message stays inside
   * the box on a 128x64 OLED as much as on a 320x240 TFT.
   */
  struct PopupBox
  {
    int32_t x, y, w, h;
    int32_t textX, textY, textW, textH;
  };

  PopupBox popupBox()
  {
    const DisplayProfile &p = display.profile();

    PopupBox b;
    b.x = p.popupMarginX;
    b.y = p.popupMarginY;
    b.w = static_cast<int32_t>(display.width()) - 2 * p.popupMarginX;
    b.h = static_cast<int32_t>(display.height()) - 2 * p.popupMarginY;

    if (b.w < 1)
      b.w = 1;
    if (b.h < 1)
      b.h = 1;

    b.textX = b.x + p.popupPadding;
    b.textY = b.y + p.popupPadding;
    b.textW = b.w - 2 * static_cast<int32_t>(p.popupPadding);
    b.textH = b.h - 2 * static_cast<int32_t>(p.popupPadding);

    if (b.textW < 1)
      b.textW = 1;
    if (b.textH < 1)
      b.textH = 1;

    return b;
  }

  /** Title across the top of the box, message wrapped in the space left below. */
  void drawPopupContents(const PopupBox &b, const String &title, const String &message, uint16_t titleColor)
  {
    const DisplayProfile &p = display.profile();

    // The box is filled white. On a 1-bit panel any non-black ink maps to white,
    // so an accent title colour would render invisible against it.
    if (display.colorDepth() == 1)
      titleColor = TFT_BLACK;

    display.setTextRole(p.popupTitleRole);
    display.setTextColor(titleColor, TFT_WHITE);
    display.setTextDatum(TL_DATUM);
    display.drawString(title, b.textX, b.textY);

    const int32_t titleHeight = display.fontHeight() + p.popupLineGap;
    const int32_t messageY = b.textY + titleHeight;
    const int32_t messageH = b.textH - titleHeight;
    if (messageH <= 0)
      return;

    display.wrapTextInBounds(message, b.textX, messageY, b.textW, messageH,
                             p.popupMessageRole, p.popupLineGap, TFT_BLACK, TL_DATUM);
  }
}

Popup::Popup(String _title, String _message) : title(_title), message(_message)
{
}

void Popup::draw()
{
  const PopupBox b = popupBox();

  display.fillRoundRect(b.x, b.y, b.w, b.h, display.profile().popupRadius, TFT_WHITE);
  drawPopupContents(b, title, message, TFT_BLACK);
}

void Popup::update()
{
  MenuInput::update();
  if (MenuInput::getSelectClicks() != 0 || MenuInput::getBackClicks() != 0)
  {
    screenManager.closePopup();
  }
}

AutoClosePopup::AutoClosePopup(String _title, String _message, unsigned long _duration)
    : Popup(_title, _message), duration(_duration)
{
  startTime = millis();
}

void AutoClosePopup::draw()
{
  const DisplayProfile &p = display.profile();
  const PopupBox b = popupBox();

  display.fillRoundRect(b.x, b.y, b.w, b.h, p.popupRadius, TFT_WHITE);

  // Countdown bar inset along the bottom edge, inside the box, so it cannot
  // collide with the top bar on a short panel.
  if (p.popupProgressHeight > 0 && b.h > p.popupProgressHeight + 2)
  {
    const int32_t trackX = b.x + 2;
    const int32_t trackW = b.w - 4;
    const int32_t trackY = b.y + b.h - p.popupProgressHeight - 2;

    const int32_t fillW = trackW > 0 ? (trackW * progressPercent) / 100 : 0;
    display.fillRect(trackX, trackY, trackW, p.popupProgressHeight, TFT_BLACK);
    if (fillW > 0)
      display.fillRect(trackX, trackY, fillW, p.popupProgressHeight, TFT_RED);
  }

  PopupBox text = b;
  if (p.popupProgressHeight > 0)
    text.textH -= p.popupProgressHeight + 2;

  drawPopupContents(text, title, message, TFT_BLUE);
}

void AutoClosePopup::update()
{
  const unsigned long elapsed = millis() - startTime;

  if (elapsed > duration)
  {
    screenManager.closePopup();
    return;
  }

  progressPercent = duration > 0
                        ? static_cast<uint8_t>(100 - (elapsed * 100) / duration)
                        : 0;
}
