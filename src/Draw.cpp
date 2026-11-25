#include "Draw.h"

#include "Display.h"
#include "Widgets.h"

int16_t Draw::radius(int16_t h)
{
  return Display::active().theme().oneBit ? min<int16_t>(3, h / 2) : min<int16_t>(10, h / 2);
}

void Draw::switchGlyph(int16_t x, int16_t y, int16_t w, int16_t h, float on, bool onFill)
{
  Display &d = Display::active();
  const Theme &t = d.theme();
  const int16_t r = h / 2;
  const int16_t inset = t.oneBit ? 1 : 2;
  const int16_t knob = h - 2 * inset;
  const int16_t knobX = x + inset + static_cast<int16_t>((w - 2 * inset - knob) * on);

  if (t.oneBit)
  {
    // Outline track; filled when on, knob in the opposite ink.
    if (on > 0.5f)
      d.fillRoundRect(x, y, w, h, r, onFill ? Color::Black : Color::White);
    else
      d.drawRoundRect(x, y, w, h, r, onFill ? Color::Black : Color::White);
    const uint16_t knobInk = (on > 0.5f) != onFill ? Color::Black : Color::White;
    d.fillRoundRect(knobX, y + inset, knob, knob, knob / 2, knobInk);
    return;
  }

  const uint16_t onTrack = onFill ? t.onAccent : t.accent;
  const uint16_t track = Color::blend(t.border, onTrack, static_cast<uint8_t>(on * 255));
  d.fillRoundRect(x, y, w, h, r, track);
  d.fillRoundRect(knobX, y + inset, knob, knob, knob / 2, onFill && on > 0.5f ? t.accent : Color::White);
}

void Draw::chevron(int16_t x, int16_t cy, int16_t size, bool right, uint16_t color)
{
  Display &d = Display::active();
  const int16_t half = size / 2;
  const int16_t dx = right ? -half : half;
  for (int16_t i = 0; i < 2; i++) // two px thick
  {
    d.drawLine(x + dx - (right ? i : -i), cy - half, x - (right ? i : -i), cy, color);
    d.drawLine(x - (right ? i : -i), cy, x + dx - (right ? i : -i), cy + half, color);
  }
}

void Draw::battery(int16_t x, int16_t y, int16_t w, int16_t h, int percent, uint16_t color)
{
  Display &d = Display::active();
  const int16_t body = w - 2;
  d.drawRoundRect(x, y, body, h, min<int16_t>(2, h / 3), color);
  d.fillRect(x + body, y + h / 3, 2, h - 2 * (h / 3), color);
  const int16_t fill = (body - 4) * constrain(percent, 0, 100) / 100;
  if (fill > 0)
  {
    const Theme &t = d.theme();
    const uint16_t c = t.oneBit || percent > 20 ? color : t.danger;
    d.fillRect(x + 2, y + 2, fill, h - 4, c);
  }
}

void Draw::focusRing(const Rect &r, int16_t radius)
{
  Display &d = Display::active();
  const Theme &t = d.theme();
  d.drawRoundRect(r.x - 2, r.y - 2, r.w + 4, r.h + 4, radius + 2,
                  t.oneBit ? t.text : Color::blend(t.accent, Color::White, 110));
}
