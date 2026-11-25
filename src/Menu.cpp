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
  const int16_t h = static_cast<int16_t>(style.lineHeight);
  const int16_t pad = t.oneBit ? 3 : 12;
  const int16_t right = static_cast<int16_t>(d.width()) - (t.oneBit ? 6 : 16);
  const int16_t midY = y + h / 2;

  uint16_t ink = active ? resolve(activeTextColor, t.highlightText) : resolve(textColor, t.text);
  if (active && isEditing())
    ink = resolve(textColor, t.text); // editing draws an outline, not a fill

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

void MenuItem::drawTrailing(int16_t right, int16_t midY, int16_t rowH, bool active)
{
  Display &d = Display::active();
  const Theme &t = d.theme();
  const bool editing = active && isEditing();
  const uint16_t muted = active && !editing ? resolve(activeTextColor, t.highlightText) : t.mutedText;

  if (trailingChevron())
  {
    Draw::chevron(right, midY, max<int16_t>(4, rowH / 3), true, muted);
    right -= rowH / 3 + 6;
  }

  if (editing && Inputs::hasPointer())
  {
    Rect minus, plus;
    MenuStyle::editTargets(midY - rowH / 2, rowH, minus, plus);
    for (const Rect &r : {minus, plus})
    {
      d.fillRoundRect(r.x + 2, r.y + 3, r.w - 4, r.h - 6, Draw::radius(r.h - 6), t.surface);
      d.drawRoundRect(r.x + 2, r.y + 3, r.w - 4, r.h - 6, Draw::radius(r.h - 6), t.accent);
    }
    d.setTextColor(t.text);
    d.setTextDatum(Datum::MiddleCenter);
    d.drawString("-", minus.x + minus.w / 2, midY);
    d.drawString("+", plus.x + plus.w / 2, midY);
    right = minus.x - 6;
  }

  const String value = valueText();
  if (!value.length())
    return;

  d.setTextDatum(Datum::MiddleRight);
  if (editing)
  {
    // < value > in the accent while it is being changed.
    const int16_t size = max<int16_t>(4, rowH / 3);
    Draw::chevron(right, midY, size, true, t.accent);
    right -= size / 2 + 4;
    d.setTextColor(t.accent);
    d.drawString(value, right, midY);
    right -= d.textWidth(value) + 4;
    Draw::chevron(right - size / 2, midY, size, false, t.accent);
    return;
  }
  d.setTextColor(muted);
  d.drawString(value, right, midY);
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

void Menu::addMenuItem(MenuItem *item)
{
  item->parent = this;
  items.push_back(item);
  ensureActiveVisible();
}

uint8_t Menu::getItemsPerPage() const
{
  return manualRowsPerPage ? manualRowsPerPage : MenuStyle::itemsPerPage(menuSize, Display::active().height());
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

  switch (e.kind)
  {
  case InputKind::Delta:
    editing ? item->adjust(e.value) : moveBy(e.value);
    break;
  case InputKind::Up:
    editing ? item->adjust(1) : moveBy(-1);
    break;
  case InputKind::Down:
    editing ? item->adjust(-1) : moveBy(1);
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
    std::vector<uint16_t> visible;
    for (size_t i = 0; i < items.size(); i++)
      if (!items[i]->isHidden())
        visible.push_back(static_cast<uint16_t>(i));
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
  // Hit-test against the list as drawn, mid-scroll included.
  const int slot = slotAt(y);
  if (slot < 0)
    return;
  const int rowH = MenuStyle::lineHeight(menuSize);
  const int top = MenuStyle::contentTopY();

  std::vector<uint16_t> visible;
  for (size_t i = 0; i < items.size(); i++)
    if (!items[i]->isHidden())
      visible.push_back(static_cast<uint16_t>(i));

  MenuItem *current = items[active];
  if (current->isEditing())
  {
    if (slot < static_cast<int>(visible.size()) && visible[slot] == active)
    {
      Rect minus, plus;
      MenuStyle::editTargets(static_cast<int16_t>(top + slot * rowH - drawnScroll), static_cast<int16_t>(rowH),
                             minus, plus);
      if (plus.contains(x, y))
        current->adjust(1);
      else if (minus.contains(x, y))
        current->adjust(-1);
      else
        current->stopEditing();
    }
    else
      current->stopEditing();
    return;
  }

  if (slot >= static_cast<int>(visible.size()))
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
  const int rowH = MenuStyle::lineHeight(menuSize);
  const int top = MenuStyle::contentTopY();
  if (y < top || rowH <= 0 || y >= top + getItemsPerPage() * rowH)
    return -1;
  return (y - top + drawnScroll) / rowH;
}

void Menu::draw()
{
  if (activeSubmenu)
  {
    activeSubmenu->draw();
    return;
  }

  ensureActiveVisible();
  std::vector<uint16_t> visible;
  visible.reserve(items.size());
  size_t activeSlot = 0;
  for (size_t i = 0; i < items.size(); i++)
  {
    if (items[i]->isHidden())
      continue;
    if (i == active)
      activeSlot = visible.size();
    visible.push_back(static_cast<uint16_t>(i));
  }
  if (visible.empty())
    return;

  Display &d = Display::active();
  const Theme &t = d.theme();
  const uint8_t rows = getItemsPerPage();
  layoutPage(visible, rows);

  const MenuStyle::DrawStyle style = MenuStyle::drawStyle(menuSize);
  const int rowH = style.lineHeight;
  const int top = MenuStyle::contentTopY();
  const float scrollTarget = static_cast<float>(topVisible * rowH);
  const float highlightTarget = static_cast<float>(activeSlot * rowH);
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

  drawnScroll = static_cast<int16_t>(scrollY.value());
  const float scroll = drawnScroll;
  const int16_t inset = t.oneBit ? 0 : 4;
  const int16_t barRight = visible.size() > rows ? 5 : 0;
  const int16_t hlW = static_cast<int16_t>(d.width()) - 2 * inset - barRight;
  const int16_t hlY = static_cast<int16_t>(top + highlightY.value() - scroll);
  MenuItem *current = items[active];
  if (current->isEditing())
  {
    if (!t.oneBit)
      d.fillRoundRect(inset, hlY, hlW, rowH, style.radius + 4, t.surface);
    d.drawRoundRect(inset, hlY, hlW, rowH, t.oneBit ? style.radius : style.radius + 4, t.accent);
  }
  else
    d.fillRoundRect(inset, hlY, hlW, rowH, t.oneBit ? style.radius : style.radius + 4,
                    resolve(current->bgColor, t.highlight));

  // Rows whose slot the scroll position touches, partial ones included.
  const int first = max(0, static_cast<int>(scroll) / rowH);
  // Rows inside the viewport; a partial one only while scrolling between pages.
  const int last = min<int>(visible.size() - 1, (static_cast<int>(scroll) + rows * rowH - 1) / rowH);
  for (int slot = first; slot <= last; slot++)
  {
    const int16_t y = static_cast<int16_t>(top + slot * rowH - scroll);
    const bool underHighlight = abs(y - hlY) < rowH / 2;
    items[visible[slot]]->draw(0, y, underHighlight);
  }

  if (barRight)
  {
    const int trackH = d.height() - top - 4;
    const int thumbH = max(6, trackH * rows / static_cast<int>(visible.size()));
    const float maxScroll = static_cast<float>((visible.size() - rows) * rowH);
    const int thumbY = top + 2 + static_cast<int>((trackH - thumbH) * min(1.0f, scroll / maxScroll));
    d.fillRoundRect(d.width() - 4, thumbY, 3, thumbH, 1, t.oneBit ? t.text : t.mutedText);
  }
}