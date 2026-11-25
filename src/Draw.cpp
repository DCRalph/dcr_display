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

  // On a highlighted row whose fill is the switch colour, the track turns to the row's ink.
  const bool clash = onFill && t.switchOn == t.highlight;
  const uint16_t onTrack = clash ? t.highlightText : t.switchOn;
  const uint16_t track = Color::blend(t.border, onTrack, static_cast<uint8_t>(on * 255));
  d.fillRoundRect(x, y, w, h, r, track);
  d.fillRoundRect(knobX, y + inset, knob, knob, knob / 2, clash && on > 0.5f ? t.highlight : t.switchKnob);
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

void Draw::raised(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t fill, bool pressed)
{
  Display &d = Display::active();
  const Theme &t = d.theme();
  const int16_t depth = t.oneBit ? 0 : t.depth;
  if (depth == 0)
  {
    d.fillRoundRect(x, y, w, h, r, fill);
    return;
  }
  if (pressed)
  {
    d.fillRoundRect(x, y + depth, w, h, r, fill);
    return;
  }
  d.fillRoundRect(x, y + depth, w, h, r, t.shadowOf(fill));
  d.fillRoundRect(x, y, w, h, r, fill);
}

void Draw::sign(int16_t cx, int16_t cy, int16_t size, bool plus, uint16_t color)
{
  Display &d = Display::active();
  const int16_t bar = max<int16_t>(2, size / 6);
  d.fillRect(cx - size / 2, cy - bar / 2, size, bar, color);
  if (plus)
    d.fillRect(cx - bar / 2, cy - size / 2, bar, size, color);
}

void Draw::radio(int16_t cx, int16_t cy, int16_t r, bool on, uint16_t ring, uint16_t dot, uint16_t background)
{
  Display &d = Display::active();
  d.fillRoundRect(cx - r, cy - r, 2 * r, 2 * r, r, ring);
  const int16_t inner = r - 2;
  d.fillRoundRect(cx - inner, cy - inner, 2 * inner, 2 * inner, inner, background);
  if (on)
  {
    const int16_t dr = max<int16_t>(2, r / 2);
    d.fillRoundRect(cx - dr, cy - dr, 2 * dr, 2 * dr, dr, dot);
  }
}
