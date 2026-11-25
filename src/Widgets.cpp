#include "Widgets.h"

#include <algorithm>

#include "Display.h"
#include "Draw.h"

void Button::draw(bool focused, bool pressed) const
{
  Display &d = Display::active();
  const Theme &t = d.theme();
  const int16_t r = Draw::radius(bounds.h);
  uint16_t fill = t.surface, ink = t.widgetText, outline = t.border;

  if (t.oneBit)
  {
    // 1-bit: only focus fills; primary gets a double border instead.
    fill = focused ? Color::White : Color::Black;
    ink = focused ? Color::Black : Color::White;
    outline = Color::White;
  }
  else if (style == Style::Primary)
  {
    fill = outline = t.accent;
    ink = t.onAccent;
  }
  else if (style == Style::Ghost)
  {
    fill = outline = t.background;
    ink = t.accent;
  }
  if (focused && !t.oneBit && style != Style::Primary)
    outline = t.accent;
  if (!enabled)
    ink = t.mutedText;
  if (pressed && !t.oneBit)
    fill = Color::blend(fill, Color::Black, 70);

  d.fillRoundRect(bounds.x, bounds.y, bounds.w, bounds.h, r, fill);
  d.drawRoundRect(bounds.x, bounds.y, bounds.w, bounds.h, r, outline);
  if (t.oneBit && style == Style::Primary && !focused)
    d.drawRoundRect(bounds.x + 1, bounds.y + 1, bounds.w - 2, bounds.h - 2, max<int16_t>(0, r - 1), outline);
  if (focused && !t.oneBit)
    Draw::focusRing(bounds, r);

  d.setTextRole(bounds.h >= 28 ? TextRole::Body : TextRole::Small);
  d.setTextColor(ink);
  d.setTextDatum(Datum::MiddleCenter);
  d.drawString(label, bounds.x + bounds.w / 2, bounds.y + bounds.h / 2);
}

void Button::press() const
{
  if (enabled && onPress)
    onPress();
}

void Switch::draw(bool focused, bool pressed) const
{
  Display &d = Display::active();
  const Theme &t = d.theme();
  if (!shown || *value != lastValue)
  {
    if (shown)
      knob.start(*value ? 1.0f : 0.0f, 160);
    else
      knob.snap(*value ? 1.0f : 0.0f);
    shown = true;
    lastValue = *value;
  }

  const int16_t r = Draw::radius(bounds.h);
  if (focused || pressed)
    d.fillRoundRect(bounds.x, bounds.y, bounds.w, bounds.h, r, t.oneBit ? Color::White : t.surface);
  if (focused && !t.oneBit)
    d.drawRoundRect(bounds.x, bounds.y, bounds.w, bounds.h, r, t.accent);

  const int16_t sh = t.oneBit ? std::max<int16_t>(6, bounds.h - 4) : std::min<int16_t>(bounds.h - 6, 22);
  const int16_t sw = sh * 9 / 5;
  Draw::switchGlyph(bounds.x + bounds.w - sw - 6, bounds.y + (bounds.h - sh) / 2, sw, sh, knob.value(),
                    focused && t.oneBit);

  d.setTextRole(bounds.h >= 28 ? TextRole::Body : TextRole::Small);
  d.setTextColor(focused && t.oneBit ? Color::Black : (enabled ? t.text : t.mutedText));
  d.setTextDatum(Datum::MiddleLeft);
  d.drawString(label, bounds.x + 8, bounds.y + bounds.h / 2);
}

void Switch::activate()
{
  if (!enabled)
    return;
  *value = !*value;
  if (onChange)
    onChange(*value);
}

Rect Slider::track() const
{
  const int16_t knob = std::min<int16_t>(14, bounds.h / 2);
  return Rect{static_cast<int16_t>(bounds.x + knob / 2 + 4), static_cast<int16_t>(bounds.y + bounds.h - knob / 2 - 4),
              static_cast<int16_t>(bounds.w - knob - 8), 4};
}

void Slider::draw(bool focused, bool pressed) const
{
  Display &d = Display::active();
  const Theme &t = d.theme();
  const int16_t r = Draw::radius(bounds.h);
  if (editing && !t.oneBit)
    d.fillRoundRect(bounds.x, bounds.y, bounds.w, bounds.h, r, t.surface);
  if (focused)
    d.drawRoundRect(bounds.x, bounds.y, bounds.w, bounds.h, r, t.oneBit ? Color::White : t.accent);

  const TextRole role = bounds.h >= 40 ? TextRole::Body : TextRole::Small;
  d.setTextRole(role);
  d.setTextColor(enabled ? t.text : t.mutedText);
  d.setTextDatum(Datum::TopLeft);
  d.drawString(label, bounds.x + 8, bounds.y + 4);
  d.setTextColor(editing ? t.accent : t.mutedText);
  d.setTextDatum(Datum::TopRight);
  d.drawString(String(*value) + suffix, bounds.x + bounds.w - 8, bounds.y + 4);

  const Rect tr = track();
  const int16_t fill = maxValue != minValue ? static_cast<int16_t>((int32_t)tr.w * (*value - minValue) / (maxValue - minValue)) : 0;
  d.fillRoundRect(tr.x, tr.y, tr.w, tr.h, 2, t.border);
  d.fillRoundRect(tr.x, tr.y, fill, tr.h, 2, t.accent);
  const int16_t knob = std::min<int16_t>(t.oneBit ? 9 : 14, bounds.h / 2) + (pressed ? 2 : 0);
  // 1-bit: a hollow knob, so it stands out from the filled track.
  d.fillRoundRect(tr.x + fill - knob / 2, tr.y + tr.h / 2 - knob / 2, knob, knob, knob / 2,
                  t.oneBit ? Color::Black : Color::White);
  d.drawRoundRect(tr.x + fill - knob / 2, tr.y + tr.h / 2 - knob / 2, knob, knob, knob / 2,
                  t.oneBit ? Color::White : t.accent);
}

int Slider::stepSize() const
{
  return step > 0 ? step : 1;
}

void Slider::set(int v)
{
  v = constrain(v, minValue, maxValue);
  if (v == *value)
    return;
  *value = v;
  if (onChange)
    onChange(v);
}

void Slider::adjust(int steps)
{
  set(*value + steps * stepSize());
}

void Slider::drag(int16_t x, int16_t)
{
  const Rect tr = track();
  if (tr.w <= 0)
    return;
  // The ends map exactly to min and max; between them the value snaps to steps.
  if (x <= tr.x)
    return set(minValue);
  if (x >= tr.x + tr.w)
    return set(maxValue);
  const int s = stepSize();
  const int32_t raw = (int32_t)(x - tr.x) * (maxValue - minValue) / tr.w;
  set(static_cast<int>(minValue + (raw + s / 2) / s * s));
}

void FocusGroup::clear()
{
  widgets.clear();
  focused = 0;
  pressed = -1;
}

void FocusGroup::setFocus(size_t index)
{
  if (index < widgets.size())
    focused = index;
}

void FocusGroup::move(int steps)
{
  if (widgets.empty())
    return;
  const int n = static_cast<int>(widgets.size());
  int next = static_cast<int>(focused) + steps;
  next = ((next % n) + n) % n;
  focused = static_cast<size_t>(next);
}

void FocusGroup::moveVertical(int steps)
{
  if (widgets.empty())
    return;
  // Column-major order: down the first column, then the next.
  std::vector<size_t> order;
  order.reserve(widgets.size());
  for (size_t col = 0; col < columns; col++)
    for (size_t i = col; i < widgets.size(); i += columns)
      order.push_back(i);

  size_t pos = 0;
  for (size_t i = 0; i < order.size(); i++)
    if (order[i] == focused)
      pos = i;
  const int n = static_cast<int>(order.size());
  focused = order[((static_cast<int>(pos) + steps) % n + n) % n];
}

bool FocusGroup::update()
{
  if (widgets.empty())
    return false;

  bool consumed = false;
  for (const InputEvent &e : Inputs::events())
  {
    Widget *w = current();
    if (!w)
      break; // a callback emptied the group
    if (e.kind == InputKind::PointerUp)
    {
      pressed = -1;
      consumed = true;
      continue;
    }
    if (w->editing)
    {
      // Editing a slider: every navigation input changes it; select or back ends it.
      switch (e.kind)
      {
      case InputKind::Delta:
        w->adjust(e.value);
        break;
      case InputKind::Up:
      case InputKind::Right:
        w->adjust(1);
        break;
      case InputKind::Down:
      case InputKind::Left:
        w->adjust(-1);
        break;
      case InputKind::Select:
      case InputKind::Back:
        w->editing = false;
        break;
      default:
        break;
      }
      consumed = true;
      continue;
    }

    switch (e.kind)
    {
    case InputKind::Delta:
      move(e.value);
      break;
    case InputKind::Left:
      move(-1);
      break;
    case InputKind::Right:
      move(1);
      break;
    case InputKind::Up:
      moveVertical(-1);
      break;
    case InputKind::Down:
      moveVertical(1);
      break;
    case InputKind::Select:
      if (e.value == 1 && w->enabled)
      {
        if (w->editable())
          w->editing = true;
        else
          w->activate();
      }
      break;
    case InputKind::PointerDown:
      pressed = -1;
      for (size_t i = 0; i < widgets.size(); i++)
      {
        if (widgets[i]->enabled && widgets[i]->bounds.contains(e.x, e.y))
        {
          pressed = static_cast<int>(i);
          focused = i;
          widgets[i]->drag(e.x, e.y);
          break;
        }
      }
      break;
    case InputKind::PointerMove:
      if (pressed >= 0)
        widgets[pressed]->drag(e.x, e.y);
      break;
    case InputKind::Tap:
      for (size_t i = 0; i < widgets.size(); i++)
      {
        if (widgets[i]->enabled && widgets[i]->bounds.contains(e.x, e.y))
        {
          focused = i;
          widgets[i]->activate();
          break;
        }
      }
      break;
    default:
      continue;
    }
    consumed = true;
  }
  return consumed;
}

void FocusGroup::draw() const
{
  // Touch-only displays have nothing to move focus with, so they show none.
  const bool showFocus = Inputs::hasKeys();
  for (size_t i = 0; i < widgets.size(); i++)
    widgets[i]->draw(showFocus && i == focused, static_cast<int>(i) == pressed);
}

namespace
{
  // Rows 0-2 hold characters; row 3 is the control row.
  const char *const kLayers[4][3] = {
      {"qwertyuiop", "asdfghjkl'", "zxcvbnm,.-"},
      {"QWERTYUIOP", "ASDFGHJKL\"", "ZXCVBNM;:_"},
      {"1234567890", "!@#$%^&*()", "+=/\\?<>[]|"},
      {"`~{}", "", ""},
  };
  // Control row: Shift, Mode, Space x4, Delete x2, Done x2.
  constexpr uint8_t kControlSpans[] = {1, 1, 4, 2, 2};
}

Rect Keyboard::keyRect(uint8_t row, uint8_t col) const
{
  const int16_t kw = bounds.w / kCols;
  const int16_t kh = bounds.h / kRows;
  Rect r{static_cast<int16_t>(bounds.x + col * kw), static_cast<int16_t>(bounds.y + row * kh), kw, kh};
  if (row == kRows - 1)
  {
    uint8_t start = 0;
    for (uint8_t span : kControlSpans)
    {
      if (col < start + span)
      {
        r.x = bounds.x + start * kw;
        r.w = span * kw;
        break;
      }
      start += span;
    }
  }
  return r;
}

Keyboard::Special Keyboard::specialAt(uint8_t col) const
{
  static constexpr Special kinds[] = {Special::Shift, Special::Mode, Special::Space, Special::Delete, Special::Done};
  uint8_t start = 0;
  for (size_t i = 0; i < sizeof(kControlSpans); i++)
  {
    if (col < start + kControlSpans[i])
      return kinds[i];
    start += kControlSpans[i];
  }
  return Special::None;
}

bool Keyboard::firstColOfKey(uint8_t row, uint8_t col) const
{
  if (row != kRows - 1)
    return true;
  uint8_t start = 0;
  for (uint8_t span : kControlSpans)
  {
    if (col == start)
      return true;
    start += span;
  }
  return false;
}

bool Keyboard::focusable(uint8_t row, uint8_t col) const
{
  return firstColOfKey(row, col) && (row == kRows - 1 || charAt(row, col) != 0);
}

char Keyboard::charAt(uint8_t row, uint8_t col) const
{
  if (row >= kRows - 1)
    return 0;
  const char *s = kLayers[static_cast<uint8_t>(layer)][row];
  return col < strlen(s) ? s[col] : 0;
}

const char *Keyboard::labelAt(uint8_t row, uint8_t col, char *buf) const
{
  if (row < kRows - 1)
  {
    buf[0] = charAt(row, col);
    buf[1] = '\0';
    return buf;
  }
  switch (specialAt(col))
  {
  case Special::Shift:
    return layer == Layer::Lower ? "^" : layer == Layer::Upper ? "v" : layer == Layer::Sym1 ? ">" : "<";
  case Special::Mode:
    return layer == Layer::Sym1 || layer == Layer::Sym2 ? "ab" : "#";
  case Special::Space:
    return "space";
  case Special::Delete:
    return "del";
  case Special::Done:
    return "ok";
  default:
    return "";
  }
}

void Keyboard::activate(uint8_t row, uint8_t col)
{
  if (row < kRows - 1)
  {
    const char c = charAt(row, col);
    if (c && onKey)
      onKey(c);
    return;
  }
  switch (specialAt(col))
  {
  case Special::Shift:
    layer = layer == Layer::Lower ? Layer::Upper : layer == Layer::Upper ? Layer::Lower
                                               : layer == Layer::Sym1    ? Layer::Sym2
                                                                         : Layer::Sym1;
    break;
  case Special::Mode:
    layer = (layer == Layer::Sym1 || layer == Layer::Sym2) ? Layer::Lower : Layer::Sym1;
    break;
  case Special::Space:
    if (onKey)
      onKey(' ');
    break;
  case Special::Delete:
    if (onDelete)
      onDelete();
    break;
  case Special::Done:
    if (onDone)
      onDone();
    break;
  default:
    break;
  }
}

void Keyboard::moveFocus(int dRow, int dCol)
{
  if (dCol != 0)
  {
    // Step over the extra columns a wide control key spans.
    int col = focusCol;
    do
      col = (col + dCol + kCols) % kCols;
    while (!focusable(focusRow, static_cast<uint8_t>(col)));
    focusCol = cursorCol = static_cast<uint8_t>(col);
    return;
  }

  int row = static_cast<int>(focusRow) + dRow;
  int col = cursorCol;
  if (row >= kRows)
  {
    row = 0;
    col = (col + 1) % kCols;
  }
  else if (row < 0)
  {
    row = kRows - 1;
    col = (col + kCols - 1) % kCols;
  }
  cursorCol = static_cast<uint8_t>(col);
  focusRow = static_cast<uint8_t>(row);
  focusCol = cursorCol;
  while (!firstColOfKey(focusRow, focusCol))
    focusCol--;
  // Blank keys on a sparse layer: keep going the same way. The control row is never blank.
  if (!focusable(focusRow, focusCol))
    moveFocus(dRow, 0);
}

void Keyboard::linearStep(int steps)
{
  const int dir = steps > 0 ? 1 : -1;
  for (int i = 0; i != steps; i += dir)
  {
    int col = focusCol, row = focusRow;
    do
    {
      col += dir;
      if (col >= kCols)
      {
        col = 0;
        row = (row + 1) % kRows;
      }
      else if (col < 0)
      {
        col = kCols - 1;
        row = (row + kRows - 1) % kRows;
      }
    } while (!focusable(static_cast<uint8_t>(row), static_cast<uint8_t>(col)));
    focusRow = static_cast<uint8_t>(row);
    focusCol = cursorCol = static_cast<uint8_t>(col);
  }
}

bool Keyboard::update()
{
  bool consumed = false;
  for (const InputEvent &e : Inputs::events())
  {
    switch (e.kind)
    {
    case InputKind::Delta:
      linearStep(e.value);
      break;
    case InputKind::Up:
      moveFocus(-1, 0);
      break;
    case InputKind::Down:
      moveFocus(1, 0);
      break;
    case InputKind::Left:
      moveFocus(0, -1);
      break;
    case InputKind::Right:
      moveFocus(0, 1);
      break;
    case InputKind::Select:
      if (e.value == 1)
        activate(focusRow, focusCol);
      break;
    case InputKind::Back:
      if (onCancel)
        onCancel();
      break;
    case InputKind::Tap:
      for (uint8_t row = 0; row < kRows; row++)
      {
        for (uint8_t col = 0; col < kCols; col++)
        {
          if (firstColOfKey(row, col) && keyRect(row, col).contains(e.x, e.y))
          {
            focusRow = row;
            focusCol = cursorCol = col;
            activate(row, col);
          }
        }
      }
      break;
    default:
      continue;
    }
    consumed = true;
  }
  return consumed;
}

void Keyboard::draw()
{
  Display &d = Display::active();
  const Theme &t = d.theme();
  const bool small = bounds.h / kRows < 14;
  const int16_t gap = small ? 1 : 2;
  d.setTextRole(small ? TextRole::Small : TextRole::Body);
  d.setTextDatum(Datum::MiddleCenter);

  char buf[2];
  for (uint8_t row = 0; row < kRows; row++)
  {
    for (uint8_t col = 0; col < kCols; col++)
    {
      if (!firstColOfKey(row, col))
        continue;
      const char *label = labelAt(row, col, buf);
      if (row < kRows - 1 && label[0] == '\0')
        continue;

      const Rect cell = keyRect(row, col);
      const Rect r{static_cast<int16_t>(cell.x + gap), static_cast<int16_t>(cell.y + gap),
                   static_cast<int16_t>(cell.w - 2 * gap), static_cast<int16_t>(cell.h - 2 * gap)};
      const bool focused = row == focusRow && col == focusCol && Inputs::hasKeys();
      const bool control = row == kRows - 1;
      const int16_t radius = min<int16_t>(t.oneBit ? 2 : 5, r.h / 2);

      uint16_t ink = t.text;
      if (focused)
      {
        d.fillRoundRect(r.x, r.y, r.w, r.h, radius, t.focus);
        ink = t.focusText;
      }
      else if (t.oneBit)
      {
        if (!small)
          d.drawRoundRect(r.x, r.y, r.w, r.h, radius, t.border);
      }
      else
        d.fillRoundRect(r.x, r.y, r.w, r.h, radius, control ? t.border : t.surface);

      d.setTextColor(ink);
      d.drawString(label, r.x + r.w / 2, r.y + r.h / 2);
    }
  }
}
