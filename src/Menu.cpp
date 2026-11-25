#include "Menu.h"

#include "ScreenManager.h"

namespace MenuStyle
{
  namespace
  {
    Metrics gMetrics{};
  }

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
    gMetrics = m;
  }

  const Metrics &metrics() noexcept
  {
    return gMetrics;
  }

  DrawStyle drawStyle(MenuSize size)
  {
    DrawStyle s = size == MenuSize::Small    ? gMetrics.small
                  : size == MenuSize::Medium ? gMetrics.medium
                                             : gMetrics.large;
    if (Inputs::hasPointer() && s.lineHeight < gMetrics.minTouchRowHeight)
      s.lineHeight = gMetrics.minTouchRowHeight;
    return s;
  }

  int lineHeight(MenuSize size)
  {
    return drawStyle(size).lineHeight;
  }

  uint16_t contentTopY() noexcept
  {
    return gMetrics.contentTopY;
  }

  uint8_t itemsPerPage(MenuSize size, uint16_t displayHeight)
  {
    const int h = displayHeight == 0 ? gMetrics.fallbackDisplayHeight : displayHeight;
    const int avail = h - static_cast<int>(gMetrics.contentTopY);
    const int row = lineHeight(size);
    const int n = row > 0 && avail > 0 ? avail / row : 1;
    return static_cast<uint8_t>(n < 1 ? 1 : (n > 255 ? 255 : n));
  }

  void editTargets(int16_t rowY, int16_t rowH, Rect &minus, Rect &plus)
  {
    const int16_t right = static_cast<int16_t>(display.width()) - 4;
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
  const Theme &t = display.theme();
  const MenuStyle::DrawStyle style = MenuStyle::drawStyle(size());
  const int16_t w = static_cast<int16_t>(display.width()) - 4;
  const int16_t h = static_cast<int16_t>(style.lineHeight);
  const bool editing = active && isEditing();

  const uint16_t text = resolve(textColor, t.text);
  uint16_t ink = text;
  if (editing)
    display.drawRoundRect(x, y, w, h, style.radius, text);
  else if (active)
  {
    display.fillRoundRect(x, y, w, h, style.radius, resolve(bgColor, t.highlight));
    ink = resolve(activeTextColor, t.highlightText);
  }

  display.setTextRole(style.role);
  display.setTextColor(ink);
  display.setTextDatum(Datum::MiddleLeft);
  const int16_t midY = y + h / 2;
  display.drawString(name, x + 2, midY);

  int16_t valueRight = x + w - 2;
  if (editing && Inputs::hasPointer())
  {
    Rect minus, plus;
    MenuStyle::editTargets(y, h, minus, plus);
    display.setTextDatum(Datum::MiddleCenter);
    display.drawRect(minus.x, minus.y + 1, minus.w, minus.h - 2, text);
    display.drawRect(plus.x, plus.y + 1, plus.w, plus.h - 2, text);
    display.drawString("-", minus.x + minus.w / 2, midY);
    display.drawString("+", plus.x + plus.w / 2, midY);
    valueRight = minus.x - 4;
  }

  const String value = valueText();
  if (value.length())
  {
    display.setTextDatum(Datum::MiddleRight);
    display.drawString(value, valueRight, midY);
  }
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
          { screenManager.setScreen(target); });
}

MenuItemBack::MenuItemBack() : MenuItem("Back")
{
  type = MenuItemType::Back;
  addFunc(MENU_DEFAULT_CLICKS, [this]()
          {
            if (parent)
              parent->back();
            else
              screenManager.back(); });
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
  return manualRowsPerPage ? manualRowsPerPage : MenuStyle::itemsPerPage(menuSize, display.height());
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
    screenManager.back();
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
  const int rowH = MenuStyle::lineHeight(menuSize);
  const int top = MenuStyle::contentTopY();
  if (y < top || rowH <= 0)
    return;

  std::vector<uint16_t> visible;
  for (size_t i = 0; i < items.size(); i++)
    if (!items[i]->isHidden())
      visible.push_back(static_cast<uint16_t>(i));

  const size_t slot = topVisible + (y - top) / rowH;
  MenuItem *current = items[active];

  if (current->isEditing())
  {
    if (slot < visible.size() && visible[slot] == active)
    {
      Rect minus, plus;
      MenuStyle::editTargets(static_cast<int16_t>(top + (slot - topVisible) * rowH), static_cast<int16_t>(rowH), minus, plus);
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

  if (slot >= visible.size())
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
  for (size_t i = 0; i < items.size(); i++)
    if (!items[i]->isHidden())
      visible.push_back(static_cast<uint16_t>(i));
  if (visible.empty())
    return;

  const uint8_t rows = getItemsPerPage();
  layoutPage(visible, rows);

  const int rowH = MenuStyle::lineHeight(menuSize);
  const int top = MenuStyle::contentTopY();
  for (uint8_t r = 0; r < rows && topVisible + r < visible.size(); r++)
  {
    const uint16_t idx = visible[topVisible + r];
    items[idx]->draw(0, static_cast<int16_t>(top + r * rowH), idx == active);
  }

  if (visible.size() > rows)
  {
    const int w = display.width();
    const int trackH = display.height() - top;
    const int thumbH = max(3, trackH * rows / static_cast<int>(visible.size()));
    const int thumbY = top + (trackH - thumbH) * topVisible / static_cast<int>(visible.size() - rows);
    display.drawLine(w - 2, top, w - 2, display.height() - 1, display.theme().text);
    display.fillRect(w - 3, thumbY, 3, thumbH, display.theme().text);
  }
}
