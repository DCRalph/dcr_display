#include "Widgets.h"

#include "Display.h"

void Button::draw(bool focused) const
{
  const Theme &t = display.theme();
  const DisplayProfile &p = display.profile();
  const uint16_t fill = focused ? t.focus : t.widget;
  const uint16_t ink = focused ? t.focusText : (enabled ? t.widgetText : t.mutedText);
  const int32_t r = p.popupRadius / 2;

  display.fillRoundRect(bounds.x, bounds.y, bounds.w, bounds.h, r, fill);
  display.drawRoundRect(bounds.x, bounds.y, bounds.w, bounds.h, r, focused ? t.focusText : t.text);

  display.setTextRole(bounds.h >= 28 ? TextRole::Body : TextRole::Small);
  display.setTextColor(ink);
  display.setTextDatum(Datum::MiddleCenter);
  display.drawString(label, bounds.x + bounds.w / 2, bounds.y + bounds.h / 2);
}

void Button::press() const
{
  if (enabled && onPress)
    onPress();
}

void FocusGroup::clear()
{
  buttons.clear();
  focused = 0;
}

void FocusGroup::setFocus(size_t index)
{
  if (index < buttons.size())
    focused = index;
}

void FocusGroup::move(int steps)
{
  if (buttons.empty())
    return;
  const int n = static_cast<int>(buttons.size());
  int next = static_cast<int>(focused) + steps;
  next = ((next % n) + n) % n;
  focused = static_cast<size_t>(next);
}

bool FocusGroup::update()
{
  if (buttons.empty())
    return false;

  bool consumed = false;
  for (const InputEvent &e : Inputs::events())
  {
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
      move(-static_cast<int>(columns));
      break;
    case InputKind::Down:
      move(columns);
      break;
    case InputKind::Select:
      if (e.value == 1)
        buttons[focused]->press();
      break;
    case InputKind::Tap:
      for (size_t i = 0; i < buttons.size(); i++)
      {
        if (buttons[i]->bounds.contains(e.x, e.y))
        {
          focused = i;
          buttons[i]->press();
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
  for (size_t i = 0; i < buttons.size(); i++)
    buttons[i]->draw(i == focused);
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
  int row = (static_cast<int>(focusRow) + dRow + kRows) % kRows;
  int col = static_cast<int>(focusCol);
  if (dCol != 0)
  {
    // Step over the extra columns a wide control key spans.
    do
      col = (col + dCol + kCols) % kCols;
    while (!firstColOfKey(static_cast<uint8_t>(row), static_cast<uint8_t>(col)));
  }
  focusRow = static_cast<uint8_t>(row);
  focusCol = static_cast<uint8_t>(col);
  while (!firstColOfKey(focusRow, focusCol))
    focusCol--;
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
    } while (!firstColOfKey(static_cast<uint8_t>(row), static_cast<uint8_t>(col)));
    focusRow = static_cast<uint8_t>(row);
    focusCol = static_cast<uint8_t>(col);
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
            focusCol = col;
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
  const Theme &t = display.theme();
  const bool small = bounds.h / kRows < 14;
  display.setTextRole(small ? TextRole::Small : TextRole::Body);
  display.setTextDatum(Datum::MiddleCenter);

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

      const Rect r = keyRect(row, col);
      const bool focused = row == focusRow && col == focusCol;
      if (focused)
        display.fillRect(r.x, r.y, r.w, r.h, t.highlight);
      else if (!small)
        display.drawRect(r.x, r.y, r.w, r.h, t.widget);

      display.setTextColor(focused ? t.highlightText : t.text);
      display.drawString(label, r.x + r.w / 2, r.y + r.h / 2);
    }
  }
}
