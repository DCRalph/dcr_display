#include "Popup.h"

#include "Display.h"
#include "Input.h"
#include "ScreenManager.h"

void Popup::close()
{
  screenManager.closePopup();
}

void Popup::update()
{
  for (const InputEvent &e : Input::events())
  {
    if (e.kind == InputKind::Select || e.kind == InputKind::Back || e.kind == InputKind::Tap)
    {
      close();
      return;
    }
  }
}

Popup::Box Popup::box() const
{
  const DisplayProfile &p = display.profile();
  Box b;
  b.x = p.popupMarginX;
  b.y = p.popupMarginY;
  b.w = max<int32_t>(1, display.width() - 2 * p.popupMarginX);
  b.h = max<int32_t>(1, display.height() - 2 * p.popupMarginY);
  b.textX = b.x + p.popupPadding;
  b.textY = b.y + p.popupPadding;
  b.textW = max<int32_t>(1, b.w - 2 * p.popupPadding);
  b.textH = max<int32_t>(1, b.h - 2 * p.popupPadding);
  return b;
}

void Popup::drawFrame(const Box &b)
{
  const Theme &t = display.theme();
  display.fillRoundRect(b.x, b.y, b.w, b.h, display.profile().popupRadius, t.popupBackground);
  if (display.colorDepth() == 1)
    display.drawRoundRect(b.x, b.y, b.w, b.h, display.profile().popupRadius, t.text);
}

void Popup::drawText(const Box &b, uint16_t titleColor)
{
  const DisplayProfile &p = display.profile();
  const Theme &t = display.theme();

  display.setTextRole(p.popupTitleRole);
  display.setTextColor(titleColor);
  display.setTextDatum(Datum::TopLeft);
  display.drawString(title, b.textX, b.textY);

  const int32_t titleHeight = display.fontHeight() + p.popupLineGap;
  const int32_t messageH = b.textH - titleHeight;
  if (messageH > 0)
    display.wrapTextInBounds(message, b.textX, b.textY + titleHeight, b.textW, messageH, p.popupMessageRole,
                             p.popupLineGap, t.popupText, Datum::TopLeft);
}

void Popup::draw()
{
  const Box b = box();
  drawFrame(b);
  drawText(b, display.theme().popupText);
}

AutoClosePopup::AutoClosePopup(String title, String message, unsigned long durationMs)
    : Popup(std::move(title), std::move(message)), startMs(millis()), durationMs(durationMs) {}

void AutoClosePopup::update()
{
  const unsigned long elapsed = millis() - startMs;
  if (elapsed >= durationMs)
  {
    close();
    return;
  }
  remainingPercent = durationMs > 0 ? static_cast<uint8_t>(100 - elapsed * 100 / durationMs) : 0;
  Popup::update();
}

void AutoClosePopup::draw()
{
  const DisplayProfile &p = display.profile();
  const Theme &t = display.theme();
  const Box b = box();
  drawFrame(b);

  Box text = b;
  if (p.popupProgressHeight > 0 && b.h > p.popupProgressHeight + 4)
  {
    const int32_t trackX = b.x + 3;
    const int32_t trackW = b.w - 6;
    const int32_t trackY = b.y + b.h - p.popupProgressHeight - 3;
    const int32_t fillW = trackW * remainingPercent / 100;
    display.drawRect(trackX, trackY, trackW, p.popupProgressHeight, t.popupText);
    if (fillW > 0)
      display.fillRect(trackX, trackY, fillW, p.popupProgressHeight, t.popupProgress);
    text.textH -= p.popupProgressHeight + 3;
  }

  drawText(text, t.popupTitle);
}

ChoicePopup::ChoicePopup(String title, String message, std::vector<String> choices, std::function<void(int)> onChoose)
    : Popup(std::move(title), std::move(message)), onChoose(std::move(onChoose))
{
  buttons.reserve(choices.size());
  for (size_t i = 0; i < choices.size(); i++)
  {
    buttons.emplace_back(choices[i], [this, i]()
                         { chosen = static_cast<int>(i); });
  }
  for (Button &button : buttons)
    group.add(button);
}

void ChoicePopup::layout(const Box &b)
{
  if (buttons.empty())
    return;
  const int32_t gap = display.profile().popupPadding;
  const int32_t h = max<int32_t>(display.height() / 6, Input::hasPointer() ? 32 : 10);
  const int32_t w = (b.textW - gap * (static_cast<int32_t>(buttons.size()) - 1)) / static_cast<int32_t>(buttons.size());
  const int32_t y = b.y + b.h - display.profile().popupPadding - h;
  for (size_t i = 0; i < buttons.size(); i++)
    buttons[i].bounds = Rect{static_cast<int16_t>(b.textX + i * (w + gap)), static_cast<int16_t>(y),
                             static_cast<int16_t>(w), static_cast<int16_t>(h)};
}

void ChoicePopup::update()
{
  layout(box());
  for (const InputEvent &e : Input::events())
  {
    if (e.kind == InputKind::Back)
      chosen = -1;
  }
  if (chosen == -2)
    group.update();

  if (chosen != -2)
  {
    const int result = chosen;
    close();
    if (onChoose)
      onChoose(result);
  }
}

void ChoicePopup::draw()
{
  Box b = box();
  layout(b);
  drawFrame(b);
  if (!buttons.empty())
    b.textH = buttons[0].bounds.y - b.textY - display.profile().popupLineGap;
  drawText(b, display.theme().popupTitle);
  group.draw();
}
