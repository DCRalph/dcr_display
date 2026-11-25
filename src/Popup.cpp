#include "Popup.h"

#include "Display.h"
#include "Draw.h"
#include "Input.h"
#include "ScreenManager.h"

void Popup::close()
{
  Display::active().screens().closePopup();
}

void Popup::update()
{
  for (const InputEvent &e : Inputs::events())
  {
    if (e.kind == InputKind::Select || e.kind == InputKind::Back || e.kind == InputKind::Tap ||
        e.kind == InputKind::Swipe)
    {
      close();
      return;
    }
  }
}

Popup::Box Popup::box() const
{
  const DisplayProfile &p = Display::active().profile();
  Box b;
  b.x = p.popupMarginX;
  // Colour panels keep the top bar clear; 64 px panels need the height.
  b.y = Display::active().theme().oneBit ? p.popupMarginY : max<int32_t>(p.popupMarginY, p.topBarHeight + 4);
  b.w = max<int32_t>(1, Display::active().width() - 2 * p.popupMarginX);
  b.h = max<int32_t>(1, Display::active().height() - b.y - p.popupMarginY);
  b.textX = b.x + p.popupPadding;
  b.textY = b.y + p.popupPadding;
  b.textW = max<int32_t>(1, b.w - 2 * p.popupPadding);
  b.textH = max<int32_t>(1, b.h - 2 * p.popupPadding);

  // Slides up into place the first time it is shown.
  if (!entered)
  {
    entered = true;
    enter.start(static_cast<float>(b.h / 4), 0.0f, 180, Ease::outCubic);
  }
  const int32_t dy = static_cast<int32_t>(enter.value());
  b.y += dy;
  b.textY += dy;
  return b;
}

void Popup::drawFrame(const Box &b)
{
  Display &d = Display::active();
  const Theme &t = d.theme();
  const int16_t r = d.profile().popupRadius;
  // Whatever is behind recedes, so the popup reads as on top.
  d.dimRect(0, 0, d.width(), d.height());
  // A solid drop shadow on colour panels: the theme's depth, or a dark band when flat.
  if (!t.oneBit)
  {
    const int16_t drop = t.depth > 0 ? t.depth + 1 : 4;
    const uint16_t shadow = t.depth > 0 ? t.shadowOf(t.popupBackground) : Color::blend(t.background, Color::Black, 160);
    d.fillRoundRect(b.x + (t.depth > 0 ? 0 : 2), b.y + drop, b.w, b.h, r, shadow);
  }
  d.fillRoundRect(b.x, b.y, b.w, b.h, r, t.popupBackground);
  if (t.oneBit || t.depth == 0)
    d.drawRoundRect(b.x, b.y, b.w, b.h, r, t.border);
}

void Popup::drawText(const Box &b, uint16_t titleColor)
{
  const DisplayProfile &p = Display::active().profile();
  const Theme &t = Display::active().theme();

  Display::active().setTextRole(p.popupTitleRole);
  Display::active().setTextColor(titleColor);
  Display::active().setTextDatum(Datum::TopLeft);
  Display::active().drawString(title, b.textX, b.textY);

  const int32_t titleHeight = Display::active().fontHeight() + p.popupLineGap;
  const int32_t messageH = b.textH - titleHeight;
  if (messageH > 0)
    Display::active().wrapTextInBounds(message, b.textX, b.textY + titleHeight, b.textW, messageH, p.popupMessageRole,
                             p.popupLineGap, t.popupText, Datum::TopLeft);
}

void Popup::draw()
{
  const Box b = box();
  drawFrame(b);
  drawText(b, Display::active().theme().popupTitle);
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
  const DisplayProfile &p = Display::active().profile();
  const Theme &t = Display::active().theme();
  const Box b = box();
  drawFrame(b);

  Box text = b;
  if (p.popupProgressHeight > 0 && b.h > p.popupProgressHeight + 4)
  {
    const int32_t trackX = b.x + 3;
    const int32_t trackW = b.w - 6;
    const int32_t trackY = b.y + b.h - p.popupProgressHeight - 3;
    const int32_t fillW = trackW * remainingPercent / 100;
    Display::active().drawRect(trackX, trackY, trackW, p.popupProgressHeight, t.popupText);
    if (fillW > 0)
      Display::active().fillRect(trackX, trackY, fillW, p.popupProgressHeight, t.popupProgress);
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
    // The last choice is the affirmative one, as on phones.
    buttons.emplace_back(choices[i], [this, i]()
                         { chosen = static_cast<int>(i); },
                         i + 1 == choices.size() ? Button::Style::Primary : Button::Style::Secondary);
  }
  for (Button &button : buttons)
    group.add(button);
}

void ChoicePopup::layout(const Box &b)
{
  if (buttons.empty())
    return;
  const int32_t gap = Display::active().profile().popupPadding;
  const int32_t h = max<int32_t>(Display::active().height() / 6, Inputs::hasPointer() ? 32 : 10);
  const int32_t w = (b.textW - gap * (static_cast<int32_t>(buttons.size()) - 1)) / static_cast<int32_t>(buttons.size());
  const int32_t y = b.y + b.h - Display::active().profile().popupPadding - h;
  for (size_t i = 0; i < buttons.size(); i++)
    buttons[i].bounds = Rect{static_cast<int16_t>(b.textX + i * (w + gap)), static_cast<int16_t>(y),
                             static_cast<int16_t>(w), static_cast<int16_t>(h)};
}

void ChoicePopup::update()
{
  layout(box());
  for (const InputEvent &e : Inputs::events())
  {
    if (e.kind == InputKind::Back || e.kind == InputKind::Swipe)
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
    b.textH = buttons[0].bounds.y - b.textY - Display::active().profile().popupLineGap;
  drawText(b, Display::active().theme().popupTitle);
  group.draw();
}
