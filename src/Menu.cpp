#include "Menu.h"

#include "Draw.h"
#include "ScreenManager.h"

namespace MenuStyle
{
  Metrics Metrics::forPanel(uint16_t width, uint16_t height, uint8_t colorDepth) noexcept
  {
    Metrics m;
    m.fallbackDisplayHeight = height == 0 ? 240 : height;
    if (height <= 80)
    {
      m.contentTopY = 12;
      m.small = {TextRole::Small, 8, 2};
      m.medium = {TextRole::Body, 12, 3};
      m.large = {TextRole::Title, 18, 4};
      m.minTouchRowHeight = 16;
    }
    else if (height <= 160) // same bands as DisplayProfile::forPanel, so rows start below the top bar
    {
      m.contentTopY = 16;
      m.small = {TextRole::Small, 10, 2};
      m.medium = {TextRole::Body, 16, 3};
      m.large = {TextRole::Title, 22, 4};
      m.minTouchRowHeight = 28;
    }
    return m;
  }

  void setMetrics(const Metrics &m) noexcept
  {
    Display::active().setMenuMetrics(m);
  }

  const Metrics &metrics() noexcept
  {
    return Display::active().menuMetrics();
  }

  DrawStyle drawStyle(MenuSize size)
  {
    const Metrics &m = metrics();
    DrawStyle s = size == MenuSize::Small    ? m.small
                  : size == MenuSize::Medium ? m.medium
                                             : m.large;
    if (Inputs::hasPointer() && s.lineHeight < m.minTouchRowHeight)
      s.lineHeight = m.minTouchRowHeight;
    return s;
  }

  int lineHeight(MenuSize size)
  {
    return drawStyle(size).lineHeight;
  }

  uint16_t contentTopY() noexcept
  {
    return metrics().contentTopY;
  }

  uint8_t itemsPerPage(MenuSize size, uint16_t displayHeight)
  {
    const int h = displayHeight == 0 ? metrics().fallbackDisplayHeight : displayHeight;
    const int avail = h - static_cast<int>(metrics().contentTopY);
    const int row = lineHeight(size);
    const int n = row > 0 && avail > 0 ? avail / row : 1;
    return static_cast<uint8_t>(n < 1 ? 1 : (n > 255 ? 255 : n));
  }

  void editTargets(int16_t rowY, int16_t rowH, Rect &minus, Rect &plus)
  {
    const int16_t right = static_cast<int16_t>(Display::active().width()) - 4;
    plus = Rect{static_cast<int16_t>(right - rowH), rowY, rowH, rowH};
    minus = Rect{static_cast<int16_t>(plus.x - rowH - 2), rowY, rowH, rowH};
  }
}

namespace
{
  uint16_t resolve(int32_t color, uint16_t fallback)
  {
    return color == kThemeColor ? fallback : static_cast<uint16_t>(color);
  }
}

void MenuItem::addFunc(int8_t clicksToRun, std::function<void()> func)
{
  if (clicksToRun != 0 && func)
    functions.push_back(ActionFunction{std::move(func), clicksToRun});
}

void MenuItem::setHidden(bool h)
{
  if (h == hidden)
    return;
  hidden = h;
  if (parent)
    parent->ensureActiveVisible();
}

void MenuItem::executeFunc(int8_t clicks)
{
  for (ActionFunction &f : functions)
  {
    if (f.clicksToRun == clicks)
    {
      f.func();
      return;
    }
  }
}

void MenuItem::run(int clicks, int rawClicks)
{
  for (ActionFunction &f : functions)
  {
    if (f.clicksToRun == clicks || f.clicksToRun == rawClicks)
    {
      f.func();
      return;
    }
  }
}

MenuSize MenuItem::size() const
{
  return parent ? parent->getMenuSize() : MenuSize::Large;
}

void MenuItem::draw(int16_t x, int16_t y, bool active)
{
  drawRow(x, y, active);
}

void MenuItem::drawRow(int16_t x, int16_t y, bool active)
{
  Display &d = Display::active();
  const Theme &t = d.theme();
  const MenuStyle::DrawStyle style = MenuStyle::drawStyle(size());
  const int16_t h = baseRowH > 0 ? baseRowH : static_cast<int16_t>(style.lineHeight);
  const int16_t pad = t.oneBit ? 3 : 12;
  const int16_t boxW = rowBox.w > 0 ? rowBox.w : static_cast<int16_t>(d.width()) - x;
  const int16_t right = x + boxW - (t.oneBit ? 6 : 14);
  const int16_t midY = y + h / 2;

  uint16_t ink = active ? resolve(activeTextColor, t.highlightText) : resolve(textColor, t.text);
  if (active && isEditing())
    ink = resolve(textColor, t.text); // editing tints the row instead of filling it

  d.setTextRole(style.role);
  d.setTextColor(ink);
  d.setTextDatum(Datum::MiddleLeft);
  int16_t nameX = x + pad;
  if (leadingChevron())
  {
    const int16_t size = max<int16_t>(4, h / 3);
    Draw::chevron(nameX, midY, size, false, ink);
    nameX += size / 2 + (t.oneBit ? 4 : 8);
  }
  d.drawString(getName(), nameX, midY);

  drawTrailing(right, midY, h, active);
}

bool MenuItem::editTap(int16_t x, int16_t y)
{
  if (editPlus.contains(x, y))
  {
    adjust(1);
    return true;
  }
  if (editMinus.contains(x, y))
  {
    adjust(-1);
    return true;
  }
  return false;
}

void MenuItem::drawTrailing(int16_t right, int16_t midY, int16_t rowH, bool active)
{
  Display &d = Display::active();
  const Theme &t = d.theme();
  const bool editing = active && isEditing();
  const bool inlineEdit = editing && editsInline();
  const uint16_t muted = active && !editing ? resolve(activeTextColor, t.highlightText) : t.mutedText;

  if (trailingChevron())
  {
    Draw::chevron(right, midY, max<int16_t>(4, rowH / 3), true, muted);
    right -= rowH / 3 + 6;
  }

  editMinus = editPlus = Rect{};
  if (inlineEdit)
  {
    drawEditPill(right, midY, rowH);
    return;
  }

  const String value = valueText();
  if (!value.length())
    return;
  d.setTextDatum(Datum::MiddleRight);
  d.setTextColor(muted);
  d.drawString(value, right, midY);
}

void MenuItem::drawEditPill(int16_t right, int16_t midY, int16_t rowH)
{
  // A raised key | value | key pill; the keys take the row's full height for touch.
  Display &d = Display::active();
  const Theme &t = d.theme();
  const int16_t depth = t.oneBit ? 0 : t.depth;
  const int16_t pillH = t.oneBit ? max<int16_t>(9, rowH - 4) : max<int16_t>(16, rowH - 10 - depth);
  const int16_t keyW = t.oneBit ? pillH : pillH + 6;
  d.setTextRole(MenuStyle::drawStyle(size()).role);
  const String value = valueText();
  // Narrow on 1-bit panels, where the row name needs most of 128 px.
  const int16_t valueW = t.oneBit ? max<int16_t>(d.textWidth(value) + 6, 16) : max<int16_t>(d.textWidth(value) + 16, 40);
  const int16_t pillW = 2 * keyW + valueW;
  const int16_t pillX = right - pillW;
  const int16_t pillY = midY - (pillH + depth) / 2;
  const int16_t cy = pillY + pillH / 2;

  // 1-bit: one outline round key, value and key.
  if (t.oneBit)
    d.drawRoundRect(pillX, pillY, pillW, pillH, pillH / 2, t.accent);
  else
  {
    Draw::raised(pillX, pillY, pillW, pillH, pillH / 2, t.widget);
    d.fillRect(pillX + keyW, pillY, valueW, pillH, t.surface);
  }
  const int16_t glyph = t.oneBit ? max<int16_t>(5, pillH / 2 - 1) : max<int16_t>(6, pillH / 2);
  if (editCycles())
  {
    Draw::chevron(pillX + keyW / 2 - glyph / 4, cy, glyph, false, t.widgetText);
    Draw::chevron(pillX + pillW - keyW / 2 + glyph / 4, cy, glyph, true, t.widgetText);
  }
  else
  {
    Draw::sign(pillX + keyW / 2, cy, glyph, false, t.widgetText);
    Draw::sign(pillX + pillW - keyW / 2, cy, glyph, true, t.widgetText);
  }
  d.setTextColor(t.text);
  d.setTextDatum(Datum::MiddleCenter);
  d.drawString(value, pillX + keyW + valueW / 2, cy);

  const int16_t rowY = midY - rowH / 2;
  editMinus = Rect{static_cast<int16_t>(pillX - 4), rowY, static_cast<int16_t>(keyW + 4), rowH};
  editPlus = Rect{static_cast<int16_t>(pillX + keyW + valueW), rowY, static_cast<int16_t>(keyW + 4), rowH};
}

void MenuItemToggle::drawTrailing(int16_t right, int16_t midY, int16_t rowH, bool active)
{
  if (!shown || *value != lastValue)
  {
    shown ? knob.start(*value ? 1.0f : 0.0f, 160) : knob.snap(*value ? 1.0f : 0.0f);
    shown = true;
    lastValue = *value;
  }
  const bool mono = Display::active().theme().oneBit;
  const int16_t sh = max<int16_t>(6, min<int16_t>(rowH - (mono ? 4 : 10), 22));
  const int16_t sw = sh * 9 / 5;
  Draw::switchGlyph(right - sw, midY - sh / 2, sw, sh, knob.value(), active);
}

// ---- Number editors ----

NumberEdit MenuItemNumberBase::style() const
{
  return Display::active().theme().resolve(editStyle);
}

bool MenuItemNumberBase::editsInline() const
{
  return style() == NumberEdit::Inline;
}

int16_t MenuItemNumberBase::editExtraHeight(int16_t rowH) const
{
  // Three rows: a label, the value between large keys, the range bar.
  return style() == NumberEdit::Expand ? max<int16_t>(rowH * 2, 60) : 0;
}

void MenuItemNumberBase::draw(int16_t x, int16_t y, bool active)
{
  // Until it has nearly grown in, an expanding row draws as a normal one.
  if (!(active && isEditing() && style() == NumberEdit::Expand && editGrow > 0.85f))
  {
    drawRow(x, y, active);
    return;
  }

  Display &d = Display::active();
  const Theme &t = d.theme();
  const int16_t pad = 10;
  const int16_t w = rowBox.w, h = rowBox.h;
  const int16_t depth = t.oneBit ? 0 : t.depth;
  const float frac = fraction();
  const int16_t barH = frac >= 0 ? 4 : 0;

  d.setTextRole(t.oneBit ? TextRole::Small : TextRole::Body);
  d.setTextColor(t.text);
  d.setTextDatum(Datum::TopLeft);
  d.drawString(getName(), x + pad, y + 5);

  // Keys and value share the band between the label and the range bar.
  const int16_t bandTop = y + (t.oneBit ? 14 : 24);
  const int16_t bandBottom = y + h - (barH ? barH + 10 : 6);
  const int16_t keyH = max<int16_t>(20, min<int16_t>(44, bandBottom - bandTop - depth));
  const int16_t keyW = min<int16_t>(56, w / 5);
  const int16_t keyY = bandTop + (bandBottom - bandTop - keyH - depth) / 2;
  const int16_t r = Draw::radius(keyH);
  const Rect minus{static_cast<int16_t>(x + pad), keyY, keyW, keyH};
  const Rect plus{static_cast<int16_t>(x + w - pad - keyW), keyY, keyW, keyH};
  for (const Rect &k : {minus, plus})
  {
    if (t.oneBit)
      d.drawRoundRect(k.x, k.y, k.w, k.h, r, t.accent);
    else
      Draw::raised(k.x, k.y, k.w, k.h, r, t.widget);
  }
  const int16_t signSize = max<int16_t>(8, keyH * 2 / 5);
  Draw::sign(minus.x + keyW / 2, keyY + keyH / 2, signSize, false, t.widgetText);
  Draw::sign(plus.x + keyW / 2, keyY + keyH / 2, signSize, true, t.widgetText);

  d.setTextRole(keyH >= 30 ? TextRole::XLarge : TextRole::Title);
  d.setTextColor(t.text);
  d.setTextDatum(Datum::MiddleCenter);
  d.drawString(valueText(), x + w / 2, keyY + keyH / 2);

  if (barH)
  {
    const int16_t barX = x + pad, barW = w - 2 * pad, barY = y + h - barH - 8;
    d.fillRoundRect(barX, barY, barW, barH, barH / 2, t.oneBit ? Color::Black : t.track);
    if (t.oneBit)
      d.drawRoundRect(barX, barY, barW, barH, barH / 2, t.text);
    const int16_t fill = static_cast<int16_t>(barW * constrain(frac, 0.0f, 1.0f));
    if (fill > 0)
      d.fillRoundRect(barX, barY, fill, barH, barH / 2, t.accent);
  }

  // Touch: the keys, grown a little.
  editMinus = Rect{static_cast<int16_t>(minus.x - 4), static_cast<int16_t>(minus.y - 4), static_cast<int16_t>(keyW + 8), static_cast<int16_t>(keyH + depth + 8)};
  editPlus = Rect{static_cast<int16_t>(plus.x - 4), static_cast<int16_t>(plus.y - 4), static_cast<int16_t>(keyW + 8), static_cast<int16_t>(keyH + depth + 8)};
}

MenuItemAction::MenuItemAction(String name, int8_t clicksToRun, std::function<void()> func) : MenuItem(std::move(name))
{
  type = MenuItemType::Action;
  addFunc(clicksToRun, std::move(func));
}

MenuItemNavigate::MenuItemNavigate(String name, const Screen2 *target) : MenuItem(std::move(name))
{
  type = MenuItemType::Navigate;
  addRoute(MENU_DEFAULT_CLICKS, target);
}

void MenuItemNavigate::addRoute(int8_t clicksToRun, const Screen2 *target)
{
  addFunc(clicksToRun, [target]()
          { Display::active().screens().setScreen(target); });
}

MenuItemBack::MenuItemBack() : MenuItem("Back")
{
  type = MenuItemType::Back;
  addFunc(MENU_DEFAULT_CLICKS, [this]()
          {
            if (parent)
              parent->back();
            else
              Display::active().screens().back(); });
}

MenuItemToggle::MenuItemToggle(String name, bool *value, bool isMutable)
    : MenuItem(std::move(name)), value(value), isMutable(isMutable)
{
  type = MenuItemType::Toggle;
  addFunc(MENU_DEFAULT_CLICKS, [this]()
          {
            if (this->isMutable)
              *this->value = !*this->value;
            if (onChange)
              onChange(); });
}

void MenuItemToggle::set(bool v)
{
  if (isMutable)
    *value = v;
}

MenuItemString::MenuItemString(String name, String *value) : MenuItem(std::move(name)), value(value)
{
  type = MenuItemType::String;
}

void MenuItemNumberBase::toggleSelected()
{
  if (!isMutable)
  {
    selected = false;
    return;
  }
  selected = !selected;
  if (!selected && onChange)
    onChange();
}

void MenuItemNumberBase::stopEditing()
{
  if (selected)
    toggleSelected();
}

MenuItemSubmenu::MenuItemSubmenu(String name, Menu *submenu) : MenuItem(std::move(name)), submenu(submenu)
{
  type = MenuItemType::Submenu;
}

void MenuItemSubmenu::run(int clicks, int rawClicks)
{
  if (clicks == 1 && parent && submenu)
    parent->setActiveSubmenu(submenu);
  else
    MenuItem::run(clicks, rawClicks);
}

MenuItemSelect::MenuItemSelect(String name, const std::vector<String> &opts, int initialIndex)
    : MenuItem(std::move(name)), options(opts)
{
  type = MenuItemType::Select;
  setCurrentIndex(initialIndex);
}

void MenuItemSelect::nextOption()
{
  adjust(1);
}

void MenuItemSelect::prevOption()
{
  adjust(-1);
}

void MenuItemSelect::adjust(int steps)
{
  if (options.empty() || steps == 0)
    return;
  const int n = static_cast<int>(options.size());
  currentIndex = (((currentIndex + steps) % n) + n) % n;
  if (onChange)
    onChange();
}

String MenuItemSelect::getSelectedOption() const
{
  return options.empty() ? String() : options[currentIndex];
}

void MenuItemSelect::setCurrentIndex(int index)
{
  if (index >= 0 && index < static_cast<int>(options.size()))
    currentIndex = index;
}

void MenuItemSelect::toggleSelected()
{
  if (!options.empty())
    selected = !selected;
}

void MenuItemSelect::stopEditing()
{
  selected = false;
}

void MenuItemSelect::run(int clicks, int rawClicks)
{
  if (clicks == 1)
    toggleSelected();
  else
    MenuItem::run(clicks, rawClicks);
}

// ---- Select editors ----

SelectEdit MenuItemSelect::style() const
{
  return Display::active().theme().resolve(editStyle);
}

bool MenuItemSelect::editsInline() const
{
  return style() == SelectEdit::Inline || (style() == SelectEdit::Expand && expandFallback);
}

bool MenuItemSelect::editIsList() const
{
  return style() == SelectEdit::Sheet;
}

int16_t MenuItemSelect::editExtraHeight(int16_t rowH) const
{
  // Two rows: a label over the segments.
  return style() == SelectEdit::Expand ? max<int16_t>(rowH + 4, 40) : 0;
}

bool MenuItemSelect::hasEditOverlay() const
{
  return style() == SelectEdit::Sheet;
}

bool MenuItemSelect::segmentLayout(Rect &strip) const
{
  Display &d = Display::active();
  const int16_t pad = 10;
  const int16_t labelH = d.theme().oneBit ? 12 : 24;
  strip = Rect{static_cast<int16_t>(rowBox.x + pad), static_cast<int16_t>(rowBox.y + labelH),
               static_cast<int16_t>(rowBox.w - 2 * pad), static_cast<int16_t>(rowBox.h - labelH - 6)};
  if (options.empty())
    return false;
  d.setTextRole(TextRole::Body);
  const int16_t segW = strip.w / static_cast<int16_t>(options.size());
  for (const String &o : options)
    if (d.textWidth(o) + 10 > segW)
      return false;
  return true;
}

void MenuItemSelect::draw(int16_t x, int16_t y, bool active)
{
  expandFallback = false;
  if (!(active && selected && style() == SelectEdit::Expand && editGrow > 0.85f))
  {
    drawRow(x, y, active);
    return;
  }

  Display &d = Display::active();
  const Theme &t = d.theme();
  d.setTextRole(t.oneBit ? TextRole::Small : TextRole::Body);
  d.setTextColor(t.text);
  d.setTextDatum(Datum::TopLeft);
  d.drawString(getName(), x + 10, y + 5);

  Rect strip;
  const bool fits = segmentLayout(strip);
  const int16_t r = Draw::radius(strip.h);
  editMinus = editPlus = Rect{};
  if (!fits)
  {
    // Too many or too long to sit side by side: < value > and -/+ as in the row.
    expandFallback = true;
    MenuItem::drawTrailing(rowBox.x + rowBox.w - 14, strip.y + strip.h / 2, strip.h, true);
    return;
  }

  // Segmented control: a sunk track, the current option raised in the highlight.
  if (t.oneBit)
    d.drawRoundRect(strip.x, strip.y, strip.w, strip.h, r, t.text);
  else
    d.fillRoundRect(strip.x, strip.y, strip.w, strip.h, r, Color::blend(t.surface, t.track, 70));
  const int16_t n = static_cast<int16_t>(options.size());
  const int16_t segW = strip.w / n;
  const int16_t depth = t.oneBit ? 0 : t.depth;
  for (int16_t i = 0; i < n; i++)
  {
    const int16_t sx = strip.x + i * segW;
    const bool on = i == currentIndex;
    if (on)
    {
      const int16_t inset = 2;
      if (t.oneBit)
        d.fillRoundRect(sx + inset, strip.y + inset, segW - 2 * inset, strip.h - 2 * inset, max<int16_t>(0, r - 2), t.text);
      else
        Draw::raised(sx + inset, strip.y + inset, segW - 2 * inset, strip.h - 2 * inset - depth, max<int16_t>(0, r - 2), t.highlight);
    }
    d.setTextRole(TextRole::Body);
    d.setTextColor(on ? (t.oneBit ? Color::Black : t.highlightText) : t.text);
    d.setTextDatum(Datum::MiddleCenter);
    d.drawString(options[i], sx + segW / 2, strip.y + (strip.h - (on ? depth : 0)) / 2);
  }
}

void MenuItemSelect::drawEditOverlay(const Rect &area)
{
  Display &d = Display::active();
  const Theme &t = d.theme();
  const int16_t n = static_cast<int16_t>(options.size());
  if (n == 0 || editGrow <= 0)
    return;

  // A sheet from the bottom edge: a title, then as many options as fit.
  const int16_t optH = max<int16_t>(t.oneBit ? 12 : 26, baseRowH - 6);
  const int16_t titleH = t.oneBit ? 12 : 26;
  const int16_t shown = max<int16_t>(1, min<int16_t>(n, (area.h - titleH - 10) / optH));
  const int16_t sheetH = titleH + shown * optH + 8;
  const int16_t margin = t.oneBit ? 0 : 6;
  if (editGrow <= 0.001f)
    sheetDragY = 0; // fully closed: the next open starts from the bottom edge
  const int16_t sheetY = area.y + area.h - static_cast<int16_t>(sheetH * editGrow) + sheetDragY;
  const int16_t r = t.oneBit ? 3 : 12;
  sheet = Rect{static_cast<int16_t>(area.x + margin), sheetY, static_cast<int16_t>(area.w - 2 * margin), sheetH};
  // The handle and title take a drag; generous, since a thumb covers the handle.
  sheetGrip = Rect{sheet.x, static_cast<int16_t>(sheet.y - 10), sheet.w, static_cast<int16_t>(titleH + 10)};
  d.dimRect(area.x, area.y, area.w, area.h); // the menu recedes behind it

  if (t.oneBit)
  {
    d.fillRoundRect(sheet.x, sheet.y, sheet.w, sheet.h + r, r, Color::Black);
    d.drawRoundRect(sheet.x, sheet.y, sheet.w, sheet.h + r, r, t.text);
  }
  else
  {
    // A rim of shadow above, since the sheet's own drop falls off the screen.
    const int16_t rim = max<int16_t>(2, t.depth);
    d.fillRoundRect(sheet.x, sheet.y - rim, sheet.w, sheet.h + r + rim, r, t.shadowOf(t.surface));
    d.fillRoundRect(sheet.x, sheet.y, sheet.w, sheet.h + r, r, t.surface);
    d.fillRoundRect(sheet.x + sheet.w / 2 - 18, sheet.y + 5, 36, 5, 2, t.track);
  }

  d.setTextRole(t.oneBit ? TextRole::Small : TextRole::Body);
  d.setTextColor(t.text);
  d.setTextDatum(Datum::MiddleLeft);
  d.drawString(getName(), sheet.x + 12, sheet.y + titleH / 2 + (t.oneBit ? 0 : 3));

  // Keep the current option in view.
  sheetFirst = constrain(currentIndex - shown / 2, 0, n - shown);
  sheetOptionH = optH;
  sheetOptions = Rect{static_cast<int16_t>(sheet.x + 6), static_cast<int16_t>(sheet.y + titleH),
                      static_cast<int16_t>(sheet.w - 12), static_cast<int16_t>(shown * optH)};
  for (int16_t i = 0; i < shown; i++)
  {
    const int index = sheetFirst + i;
    const int16_t oy = sheetOptions.y + i * optH;
    const bool on = index == currentIndex;
    uint16_t ink = t.text, fill = t.surface;
    if (on)
    {
      fill = t.oneBit ? Color::White : t.highlight;
      ink = t.oneBit ? Color::Black : t.highlightText;
      d.fillRoundRect(sheetOptions.x, oy + 1, sheetOptions.w, optH - 2, t.oneBit ? 2 : 7, fill);
    }
    d.setTextColor(ink);
    d.setTextDatum(Datum::MiddleLeft);
    d.drawString(options[index], sheetOptions.x + 10, oy + optH / 2);
    if (!t.oneBit)
      Draw::radio(sheetOptions.x + sheetOptions.w - 16, oy + optH / 2, 7, on, on ? ink : t.mutedText, ink, fill);
  }
}

bool MenuItemSelect::editPointer(const InputEvent &e)
{
  if (style() != SelectEdit::Sheet)
    return false;
  switch (e.kind)
  {
  case InputKind::PointerDown:
    if (!sheetGrip.contains(e.x, e.y))
      return false;
    sheetDragging = true;
    sheetDragStartY = e.y;
    sheetDragY = 0;
    return true;
  case InputKind::PointerMove:
    if (!sheetDragging)
      return false;
    sheetDragY = max<int16_t>(0, e.y - sheetDragStartY);
    return true;
  case InputKind::PointerUp:
    if (!sheetDragging)
      return false;
    sheetDragging = false;
    // Far enough, or flicked down: close from where it is; else spring back.
    if (sheetDragY > sheet.h / 3 || e.value > 40)
      stopEditing();
    else
      sheetDragY = 0;
    return true;
  case InputKind::Swipe:
    if (e.value == static_cast<int16_t>(SwipeDir::Down) && sheet.contains(e.x, e.y))
    {
      stopEditing();
      return true;
    }
    return false;
  default:
    return false;
  }
}

int MenuItemSelect::optionAt(int16_t x, int16_t y) const
{
  if (!sheetOptions.contains(x, y) || sheetOptionH <= 0)
    return -1;
  const int index = sheetFirst + (y - sheetOptions.y) / sheetOptionH;
  return index < static_cast<int>(options.size()) ? index : -1;
}

bool MenuItemSelect::editTap(int16_t x, int16_t y)
{
  const SelectEdit s = style();
  if (s == SelectEdit::Sheet)
  {
    if (!sheet.contains(x, y))
      return false;
    const int index = optionAt(x, y);
    if (index >= 0)
    {
      adjust(index - currentIndex);
      stopEditing();
    }
    return true;
  }
  if (s == SelectEdit::Expand)
  {
    Rect strip;
    if (segmentLayout(strip))
    {
      if (!strip.contains(x, y))
        return false;
      const int index = (x - strip.x) / (strip.w / static_cast<int16_t>(options.size()));
      adjust(constrain(index, 0, static_cast<int>(options.size()) - 1) - currentIndex);
      stopEditing();
      return true;
    }
  }
  return MenuItem::editTap(x, y);
}

void Menu::addMenuItem(MenuItem *item)
{
  item->parent = this;
  items.push_back(item);
  ensureActiveVisible();
}

bool Menu::grouped() const
{
  return Display::active().theme().resolve(menuLook) == MenuLook::Grouped;
}

Menu::Frame Menu::frame() const
{
  Display &d = Display::active();
  const Theme &t = d.theme();
  Frame f;
  const bool g = grouped();
  const int16_t top = MenuStyle::contentTopY();
  const int16_t marginX = g ? MenuStyle::kGroupMarginX : 0;
  const int16_t marginTop = g ? MenuStyle::kGroupMarginTop : 0;
  // A grouped card keeps its shadow and a little air at the bottom.
  const int16_t marginBottom = g ? (t.oneBit ? 2 : t.depth + 4) : 0;
  f.top = top + marginTop;
  f.x = marginX;
  f.w = static_cast<int16_t>(d.width()) - 2 * marginX;
  f.viewH = static_cast<int16_t>(d.height()) - f.top - marginBottom;
  f.grouped = g;
  return f;
}

uint8_t Menu::getItemsPerPage() const
{
  if (manualRowsPerPage)
    return manualRowsPerPage;
  if (!grouped())
    return MenuStyle::itemsPerPage(menuSize, Display::active().height());
  const int rowH = MenuStyle::lineHeight(menuSize);
  const int n = rowH > 0 ? frame().viewH / rowH : 1;
  return static_cast<uint8_t>(constrain(n, 1, 255));
}

int Menu::growSlot() const
{
  return growItem ? growVisibleSlot : -1;
}

int16_t Menu::growPx() const
{
  if (!growItem || growVisibleSlot < 0)
    return 0;
  return static_cast<int16_t>(lroundf(growItem->editExtraHeight(MenuStyle::lineHeight(menuSize)) * grow.value()));
}

int32_t Menu::slotTop(int slot) const
{
  const int rowH = MenuStyle::lineHeight(menuSize);
  const int g = growSlot();
  return static_cast<int32_t>(slot) * rowH + (g >= 0 && slot > g ? growPx() : 0);
}

int16_t Menu::slotHeight(int slot) const
{
  const int16_t rowH = static_cast<int16_t>(MenuStyle::lineHeight(menuSize));
  return slot == growSlot() ? rowH + growPx() : rowH;
}

void Menu::setActive(uint16_t index)
{
  if (index < items.size())
    active = index;
  ensureActiveVisible();
}

void Menu::nextItem()
{
  for (size_t i = active + 1; i < items.size(); i++)
  {
    if (!items[i]->isHidden())
    {
      active = static_cast<uint16_t>(i);
      return;
    }
  }
}

void Menu::prevItem()
{
  for (int i = static_cast<int>(active) - 1; i >= 0; i--)
  {
    if (!items[i]->isHidden())
    {
      active = static_cast<uint16_t>(i);
      return;
    }
  }
}

void Menu::moveBy(int steps)
{
  for (int i = 0; i < abs(steps); i++)
    steps > 0 ? nextItem() : prevItem();
}

void Menu::ensureActiveVisible()
{
  if (items.empty() || active >= items.size())
  {
    active = 0;
    return;
  }
  if (!items[active]->isHidden())
    return;
  const uint16_t before = active;
  nextItem();
  if (active == before)
    prevItem();
}

void Menu::setActiveSubmenu(Menu *submenu)
{
  activeSubmenu = submenu;
  if (submenu)
    submenu->setParentMenu(this);
}

void Menu::back()
{
  if (parentMenu)
    parentMenu->clearActiveSubmenu();
  else
    Display::active().screens().back();
}

void Menu::update()
{
  for (const InputEvent &e : Inputs::events())
  {
    Menu *target = this;
    while (target->activeSubmenu)
      target = target->activeSubmenu;
    target->handle(e);
  }
}

const std::vector<uint16_t> &Menu::visibleItems()
{
  visibleScratch.clear(); // keeps its capacity: no allocation after the first frame
  for (size_t i = 0; i < items.size(); i++)
    if (!items[i]->isHidden())
      visibleScratch.push_back(static_cast<uint16_t>(i));
  return visibleScratch;
}

void Menu::handle(const InputEvent &e)
{
  if (items.empty())
  {
    if ((e.kind == InputKind::Select && e.value == 1) || e.kind == InputKind::Back || e.kind == InputKind::Left ||
        e.kind == InputKind::Tap)
      back();
    return;
  }

  ensureActiveVisible();
  MenuItem *item = items[active];
  const bool editing = item->isEditing();
  const int rowH = MenuStyle::lineHeight(menuSize);
  if (editing && item->editPointer(e))
    return;

  switch (e.kind)
  {
  case InputKind::Delta:
    editing ? item->adjust(e.value) : moveBy(e.value);
    break;
  // A vertical option list steps the way it is drawn: Up goes to the option above.
  case InputKind::Up:
    editing ? item->adjust(item->editIsList() ? -1 : 1) : moveBy(-1);
    break;
  case InputKind::Down:
    editing ? item->adjust(item->editIsList() ? 1 : -1) : moveBy(1);
    break;
  case InputKind::Right:
    editing ? item->adjust(1) : item->run(1, 1);
    break;
  case InputKind::Left:
    editing ? item->adjust(-1) : back();
    break;
  case InputKind::Back:
    editing ? item->stopEditing() : back();
    break;
  case InputKind::Select:
    item->run(e.value, e.rawClicks);
    break;
  case InputKind::Tap:
    tap(e.x, e.y);
    break;
  case InputKind::Swipe:
    if (!editing && e.value == static_cast<int16_t>(SwipeDir::Right))
      back();
    break;
  case InputKind::LongPress:
  {
    // A long press is the touch version of a long click: run the -1 action.
    const int slot = slotAt(e.y);
    const std::vector<uint16_t> &visible = visibleItems();
    if (!editing && slot >= 0 && slot < static_cast<int>(visible.size()))
    {
      active = visible[slot];
      items[active]->run(-1, -1);
    }
    break;
  }
  case InputKind::PointerUp:
    // A flick keeps scrolling: about 200 ms of travel at release speed.
    if (!editing && rowH > 0 && abs(e.value) > 40)
      moveBy(constrain(-e.value * 2 / rowH, -12, 12));
    break;
  case InputKind::PointerDown:
    dragAccum = 0;
    break;
  case InputKind::PointerMove:
    if (editing || rowH <= 0)
      break;
    dragAccum += e.value;
    while (dragAccum >= rowH)
    {
      moveBy(-1);
      dragAccum -= rowH;
    }
    while (dragAccum <= -rowH)
    {
      moveBy(1);
      dragAccum += rowH;
    }
    break;
  default:
    break;
  }
}

void Menu::tap(int16_t x, int16_t y)
{
  MenuItem *current = items[active];
  if (current->isEditing())
  {
    // The editor gets the touch first; anywhere else ends editing.
    if (!current->editTap(x, y))
      current->stopEditing();
    return;
  }

  // Hit-test against the list as drawn, mid-scroll included.
  const int slot = slotAt(y);
  const std::vector<uint16_t> &visible = visibleItems();
  if (slot < 0 || slot >= static_cast<int>(visible.size()))
    return;
  active = visible[slot];
  items[active]->run(1, 1);
}

void Menu::layoutPage(const std::vector<uint16_t> &visible, uint8_t rows)
{
  size_t activeSlot = 0;
  for (size_t i = 0; i < visible.size(); i++)
    if (visible[i] == active)
      activeSlot = i;

  // Keep one row of lookahead so the next item is always in view.
  const size_t margin = rows >= 3 ? 1 : 0;
  if (activeSlot < topVisible + margin)
    topVisible = static_cast<uint16_t>(activeSlot > margin ? activeSlot - margin : 0);
  else if (activeSlot + margin >= topVisible + rows)
    topVisible = static_cast<uint16_t>(activeSlot + margin + 1 - rows);

  const size_t maxTop = visible.size() > rows ? visible.size() - rows : 0;
  if (topVisible > maxTop)
    topVisible = static_cast<uint16_t>(maxTop);
}

int Menu::slotAt(int16_t y) const
{
  const Frame f = frame();
  const int rowH = MenuStyle::lineHeight(menuSize);
  if (y < f.top || rowH <= 0 || y >= f.top + f.viewH)
    return -1;
  int32_t c = y - f.top + drawnScroll;
  const int g = growSlot();
  if (g >= 0)
  {
    const int32_t gTop = slotTop(g);
    if (c >= gTop + slotHeight(g))
      c -= growPx();
    else if (c >= gTop)
      return g;
  }
  return static_cast<int>(c / rowH);
}

void Menu::draw()
{
  if (activeSubmenu)
  {
    activeSubmenu->draw();
    return;
  }

  ensureActiveVisible();
  const std::vector<uint16_t> &visible = visibleItems();
  if (visible.empty())
    return;
  size_t activeSlot = 0;
  for (size_t i = 0; i < visible.size(); i++)
    if (visible[i] == active)
      activeSlot = i;

  Display &d = Display::active();
  const Theme &t = d.theme();
  const uint8_t rows = getItemsPerPage();
  layoutPage(visible, rows);

  const MenuStyle::DrawStyle style = MenuStyle::drawStyle(menuSize);
  const int rowH = style.lineHeight;
  const Frame f = frame();

  // The row being edited grows in, and shrinks back after.
  MenuItem *current = items[active];
  const bool editorGrows = current->isEditing() && (current->editExtraHeight(rowH) > 0 || current->hasEditOverlay());
  if (editorGrows && growItem != current)
  {
    growItem = current;
    grow.snap(0);
    grow.start(1, 180);
  }
  else if (!editorGrows && growItem && grow.target() != 0)
    grow.start(0, 140);
  if (growItem && !editorGrows && grow.value() <= 0.001f)
    growItem = nullptr;
  growVisibleSlot = -1;
  for (size_t i = 0; growItem && i < visible.size(); i++)
    if (items[visible[i]] == growItem)
      growVisibleSlot = static_cast<int>(i);

  // Scroll by rows, then far enough that a growing row is wholly in view.
  const int32_t fullExtra = growItem && grow.target() > 0 ? growItem->editExtraHeight(rowH) : 0;
  const int32_t contentH = static_cast<int32_t>(visible.size()) * rowH + fullExtra;
  int32_t scrollTarget = static_cast<int32_t>(topVisible) * rowH;
  if (fullExtra > 0)
  {
    const int32_t activeTop = static_cast<int32_t>(activeSlot) * rowH;
    const int32_t activeBottom = activeTop + rowH + fullExtra;
    if (activeBottom - scrollTarget > f.viewH)
      scrollTarget = activeBottom - f.viewH;
    if (activeTop < scrollTarget)
      scrollTarget = activeTop;
  }
  scrollTarget = constrain(scrollTarget, 0, max<int32_t>(0, contentH - f.viewH));

  const float highlightTarget = static_cast<float>(slotTop(static_cast<int>(activeSlot)));
  if (laidOutRowH != rowH)
  {
    // First draw, or rows changed size: place everything without animating.
    scrollY.snap(scrollTarget);
    highlightY.snap(highlightTarget);
    laidOutRowH = rowH;
  }
  if (scrollY.target() != scrollTarget)
    scrollY.start(scrollTarget, 160);
  if (highlightY.target() != highlightTarget)
    highlightY.start(highlightTarget, 130);
  drawnScroll = static_cast<int32_t>(scrollY.value());
  const int32_t scroll = drawnScroll;

  const int growAt = growSlot();
  const int16_t extra = growPx();
  const int32_t listH = static_cast<int32_t>(visible.size()) * rowH + extra;
  const int16_t barRight = contentH > f.viewH ? 5 : 0;
  const int16_t radius = t.oneBit ? style.radius : style.radius + 4;

  // At rest only whole rows show; mid-scroll, partial ones too.
  const bool moving = scrollY.running() || grow.running();
  const auto shown = [&](int slot)
  {
    const int32_t top = slotTop(slot);
    const int16_t h = slotHeight(slot);
    if (top + h <= scroll || top >= scroll + f.viewH)
      return false;
    return moving || (top >= scroll && top + h <= scroll + f.viewH);
  };
  int32_t shownBottom = 0; // list px, below the last row drawn
  for (int slot = 0; slot < static_cast<int>(visible.size()); slot++)
    if (shown(slot))
      shownBottom = max<int32_t>(shownBottom, slotTop(slot) + slotHeight(slot));

  // Grouped: one card under the rows, scrolling with them; the top bar covers what rises above.
  int16_t rowX = 0, rowW = static_cast<int16_t>(d.width()) - barRight;
  if (f.grouped)
  {
    rowX = f.x;
    rowW = f.w - (barRight ? 2 : 0);
    // To the last row drawn (or the list's end); a moving list fills the viewport.
    const int32_t cardY = f.top - scroll;
    const int32_t cardBottom = moving ? min<int32_t>(listH, scroll + f.viewH) : shownBottom;
    const int16_t visibleCardH = static_cast<int16_t>(max<int32_t>(0, cardBottom));
    if (t.oneBit)
      d.drawRoundRect(rowX, static_cast<int16_t>(cardY), rowW, visibleCardH, 3, t.text);
    else
      Draw::raised(rowX, static_cast<int16_t>(cardY), rowW, visibleCardH, MenuStyle::kGroupRadius, t.surface);
  }

  // Highlight, or the editing tint, at the active row.
  const int16_t hlY = static_cast<int16_t>(f.top + highlightY.value() - scroll);
  const int16_t hlH = slotHeight(static_cast<int>(activeSlot));
  const int16_t inset = f.grouped ? 3 : (t.oneBit ? 0 : 4);
  const int16_t hlX = rowX + inset;
  const int16_t hlW = rowW - 2 * inset;
  if (current->isEditing())
  {
    if (!t.oneBit)
      d.fillRoundRect(hlX, hlY + (f.grouped ? 2 : 0), hlW, hlH - (f.grouped ? 4 : 0), f.grouped ? 7 : radius,
                      Color::blend(t.surface, t.highlight, 60));
    if (!f.grouped || t.oneBit)
      d.drawRoundRect(hlX, hlY, hlW, hlH, radius, t.accent);
  }
  else if (f.grouped && !t.oneBit)
    d.fillRoundRect(hlX, hlY + 2, hlW, rowH - 4, 7, resolve(current->bgColor, t.highlight));
  else
    d.fillRoundRect(hlX, hlY, hlW, rowH, radius, resolve(current->bgColor, t.highlight));

  // Rows the viewport touches, partial ones included while scrolling.
  for (int slot = 0; slot < static_cast<int>(visible.size()); slot++)
  {
    if (!shown(slot))
      continue;
    const int32_t top = slotTop(slot);
    const int16_t h = slotHeight(slot);
    const int16_t y = static_cast<int16_t>(f.top + top - scroll);
    MenuItem *item = items[visible[slot]];

    // Hairlines between grouped rows, clear of the highlight.
    if (f.grouped && slot > 0 && !t.oneBit && slot != static_cast<int>(activeSlot) && slot - 1 != static_cast<int>(activeSlot))
      d.drawLine(rowX + 12, y, rowX + rowW - 1, y, t.border);

    item->rowBox = Rect{rowX, y, rowW, h};
    item->baseRowH = static_cast<int16_t>(rowH);
    item->editGrow = slot == growAt ? grow.value() : 0;
    const bool underHighlight = abs(y - hlY) < rowH / 2;
    item->draw(rowX, y, underHighlight);
  }

  if (barRight)
  {
    const int trackH = f.viewH - 4;
    const int thumbH = max(6, static_cast<int>(trackH * f.viewH / max<int32_t>(contentH, 1)));
    const int32_t maxScroll = max<int32_t>(1, contentH - f.viewH);
    const int thumbY = f.top + 2 + static_cast<int>((trackH - thumbH) * min(1.0f, static_cast<float>(scroll) / maxScroll));
    d.fillRoundRect(d.width() - 4, thumbY, 3, thumbH, 1, t.oneBit ? t.text : t.mutedText);
  }

  // Editors that draw over the menu (a sheet), growing in and out with it.
  if (growItem && growItem->hasEditOverlay())
  {
    growItem->editGrow = grow.value();
    growItem->baseRowH = static_cast<int16_t>(rowH);
    growItem->drawEditOverlay(Rect{0, static_cast<int16_t>(MenuStyle::contentTopY()), static_cast<int16_t>(d.width()),
                                   static_cast<int16_t>(d.height() - MenuStyle::contentTopY())});
  }
}
