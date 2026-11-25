#pragma once

#include <Arduino.h>
#include <functional>
#include <limits>
#include <type_traits>
#include <vector>

#include "Animation.h"
#include "Display.h"
#include "MenuMetrics.h"
#include "Input.h"
#include "ScreenTypes.h"
#include "Widgets.h"

#define MENU_DEFAULT_CLICKS 1

class Menu;

enum class MenuItemType
{
  None,
  Action,
  Navigate,
  Back,
  Toggle,
  String,
  Number,
  Select,
  Submenu
};

enum class MenuSize
{
  Large,
  Medium,
  Small
};

namespace MenuStyle
{
  /** The active display's metrics (see Display::active). */
  void setMetrics(const Metrics &metrics) noexcept;
  const Metrics &metrics() noexcept;

  /** Style for a preset, with rows enlarged for touch when a pointer is attached. */
  DrawStyle drawStyle(MenuSize size);
  int lineHeight(MenuSize size);
  uint16_t contentTopY() noexcept;
  uint8_t itemsPerPage(MenuSize size, uint16_t displayHeight);

  /** The -/+ touch targets drawn on a row being edited, at the panel's right edge. */
  void editTargets(int16_t rowY, int16_t rowH, Rect &minus, Rect &plus);

  // Space around a Grouped menu's card.
  constexpr int16_t kGroupMarginX = 8;
  constexpr int16_t kGroupMarginTop = 4;
  constexpr int16_t kGroupRadius = 10;
}

struct ActionFunction
{
  std::function<void()> func;
  int8_t clicksToRun;
};

// One menu row. Subclasses supply a value to show and, if editable, capture
// navigation while editing. Bound functions run on matching select click counts.
class MenuItem
{
  friend class Menu;

public:
  explicit MenuItem(String name) : name(std::move(name)) {}
  virtual ~MenuItem() = default;

  void addFunc(int8_t clicksToRun, std::function<void()> func);
  void setName(String newName) { name = std::move(newName); }
  const String &getName() const { return name; }
  MenuItemType getType() const { return type; }

  void setTextColor(uint16_t color) { textColor = color; }       // row text when not highlighted
  void setActiveTextColor(uint16_t color) { activeTextColor = color; }
  void setBgColor(uint16_t color) { bgColor = color; }           // highlight fill when active
  void setHidden(bool hidden);
  bool isHidden() const { return hidden; }

  void executeFunc() { executeFunc(MENU_DEFAULT_CLICKS); }
  void executeFunc(int8_t clicks);

  /** Runs the function bound to `clicks` (or the raw hardware count). */
  virtual void run(int clicks = 1, int rawClicks = 1);
  virtual String valueText() const { return String(); }
  virtual bool isEditing() const { return false; }
  /** While editing: positive steps increase. */
  virtual void adjust(int steps) {}
  virtual void stopEditing() {}

  /** Draws the row's content; the menu draws the highlight under it. `active`: highlight is under it. */
  virtual void draw(int16_t x, int16_t y, bool active);
  /** Right-hand side of the row: by default valueText(), with chevrons while editing. */
  virtual void drawTrailing(int16_t right, int16_t midY, int16_t rowH, bool active);
  /** A chevron before the name (Back) or after it (navigation). */
  virtual bool leadingChevron() const { return false; }
  virtual bool trailingChevron() const { return false; }

  // Editors. The menu grows the row by editExtraHeight() while editing, animated
  // through editGrow (0-1), draws drawEditOverlay() above every row, and hands
  // touches to editTap() first.
  /** Px the row grows by while editing; 0 keeps it one row tall. */
  virtual int16_t editExtraHeight(int16_t rowH) const { return 0; }
  /** True if the editor draws over the menu (a sheet) instead of in its row. */
  virtual bool hasEditOverlay() const { return false; }
  virtual void drawEditOverlay(const Rect &area) {}
  /** A touch while editing. True if it hit the editor; false ends editing. */
  virtual bool editTap(int16_t x, int16_t y);
  /** True if Up should step back through the options (a vertical list). */
  virtual bool editIsList() const { return false; }
  /** Pointer and swipe events while editing, before the menu's own handling; true if used. */
  virtual bool editPointer(const InputEvent &e) { return false; }

  std::vector<ActionFunction> functions;

protected:
  /** Standard row: name left, trailing content right, -/+ targets when editing by touch. */
  void drawRow(int16_t x, int16_t y, bool active);
  /** In-row editing: a raised pill of value between two keys at the row's end. */
  virtual bool editsInline() const { return true; }
  /** The pill's keys show < > (options that cycle) instead of - +. */
  virtual bool editCycles() const { return false; }
  /** Draws the pill ending at `right` and records its keys as editMinus / editPlus. */
  void drawEditPill(int16_t right, int16_t midY, int16_t rowH);
  MenuSize size() const;

  // Where the menu is drawing this row, set before draw(): its box (taller while
  // growing), its normal height, and how far an editor has grown in.
  Rect rowBox;
  int16_t baseRowH = 0;
  float editGrow = 0;
  // Touch targets as last drawn, for editTap().
  Rect editMinus, editPlus;

  Menu *parent = nullptr;
  MenuItemType type = MenuItemType::None;
  int32_t textColor = kThemeColor;
  int32_t activeTextColor = kThemeColor;
  int32_t bgColor = kThemeColor;

private:
  String name;
  bool hidden = false;
};

class MenuItemAction : public MenuItem
{
public:
  MenuItemAction(String name, int8_t clicksToRun, std::function<void()> func);
};

class MenuItemNavigate : public MenuItem
{
public:
  MenuItemNavigate(String name, const Screen2 *target);
  void addRoute(int8_t clicksToRun, const Screen2 *target);
  bool trailingChevron() const override { return true; }
};

class MenuItemBack : public MenuItem
{
public:
  MenuItemBack();
  bool leadingChevron() const override { return true; }
};

class MenuItemToggle : public MenuItem
{
public:
  MenuItemToggle(String name, bool *value, bool isMutable = true);

  void setOnChange(std::function<void()> callback) { onChange = std::move(callback); }
  void removeOnChange() { onChange = nullptr; }
  void set(bool v);
  bool get() const { return *value; }
  String valueText() const override { return *value ? "ON" : "OFF"; }
  /** A switch whose knob slides when the value changes. */
  void drawTrailing(int16_t right, int16_t midY, int16_t rowH, bool active) override;

private:
  bool *value;
  bool isMutable;
  std::function<void()> onChange;
  Tween knob;
  bool shown = false;
  bool lastValue = false;
};

class MenuItemString : public MenuItem
{
public:
  MenuItemString(String name, String *value);

  void setValue(String v) { *value = std::move(v); }
  String getValue() const { return *value; }
  String valueText() const override { return *value; }

private:
  String *value;
};

// Editable (or read-only) number. Select starts editing, navigation changes the
// value within [min, max], select again commits and fires onChange.
class MenuItemNumberBase : public MenuItem
{
public:
  explicit MenuItemNumberBase(String name) : MenuItem(std::move(name)) { type = MenuItemType::Number; }

  bool isSelected() const { return selected; }
  bool isEditing() const override { return selected; }
  void stopEditing() override;
  void toggleSelected();

  /** Fire onChange on every step while editing, not only on commit. */
  void setFastUpdate(bool fast) { fastUpdate = fast; }
  bool isFastUpdate() const { return fastUpdate; }
  void setOnChange(std::function<void()> callback) { onChange = std::move(callback); }
  void removeOnChange() { onChange = nullptr; }

  /** How this item edits; `NumberEdit::Theme` follows the theme. */
  void setEditStyle(NumberEdit style) { editStyle = style; }
  NumberEdit getEditStyle() const { return editStyle; }
  /** Where the value sits between min and max, 0-1; negative if unbounded. */
  virtual float fraction() const { return -1; }

  void draw(int16_t x, int16_t y, bool active) override;
  int16_t editExtraHeight(int16_t rowH) const override;

protected:
  bool editsInline() const override;
  NumberEdit style() const;

  NumberEdit editStyle = NumberEdit::Theme;
  bool selected = false;
  bool isMutable = false;
  bool fastUpdate = false;
  std::function<void()> onChange;
};

template <typename T>
class MenuItemNumber : public MenuItemNumberBase
{
  static_assert(std::is_arithmetic<T>::value, "MenuItemNumber needs an arithmetic type");

public:
  MenuItemNumber(String name, T *value, T min, T max, T step = 1)
      : MenuItemNumberBase(std::move(name)), value(value), min(min), max(max), step(step)
  {
    isMutable = true;
    addFunc(1, [this]()
            { toggleSelected(); });
  }

  /** Read-only display of `*value`. */
  MenuItemNumber(String name, T *value)
      : MenuItemNumberBase(std::move(name)), value(value),
        min(std::numeric_limits<T>::lowest()), max(std::numeric_limits<T>::max()) {}

  void setDecimals(uint8_t places) { decimals = places; }

  float fraction() const override
  {
    if (!value || !(max > min) || min == std::numeric_limits<T>::lowest() || max == std::numeric_limits<T>::max())
      return -1;
    return static_cast<float>(*value - min) / static_cast<float>(max - min);
  }

  void adjust(int steps) override
  {
    if (!value || !isMutable || steps == 0)
      return;
    for (int i = 0; i < abs(steps); i++)
      *value = steps > 0 ? stepUp(*value) : stepDown(*value);
    if (fastUpdate && onChange)
      onChange();
  }

  void increase() { adjust(1); }
  void decrease() { adjust(-1); }

  String valueText() const override
  {
    if (!value)
      return String();
    if constexpr (std::is_floating_point<T>::value)
      return String(static_cast<double>(*value), static_cast<unsigned int>(decimals));
    else if constexpr (std::is_signed<T>::value)
      return String(static_cast<long long>(*value));
    else
      return String(static_cast<unsigned long long>(*value));
  }

private:
  T stepUp(T v) const
  {
    if constexpr (std::is_floating_point<T>::value)
      return v + step > max ? max : v + step;
    else
    {
      T next;
      return (__builtin_add_overflow(v, step, &next) || next > max) ? max : next;
    }
  }

  T stepDown(T v) const
  {
    if constexpr (std::is_floating_point<T>::value)
      return v - step < min ? min : v - step;
    else
    {
      T next;
      return (__builtin_sub_overflow(v, step, &next) || next < min) ? min : next;
    }
  }

  T *value;
  T min;
  T max;
  T step = 1;
  uint8_t decimals = 2;
};

class MenuItemSubmenu : public MenuItem
{
public:
  MenuItemSubmenu(String name, Menu *submenu);
  void run(int clicks = 1, int rawClicks = 1) override;
  bool trailingChevron() const override { return true; }

private:
  Menu *submenu;
};

class MenuItemSelect : public MenuItem
{
public:
  MenuItemSelect(String name, const std::vector<String> &options, int initialIndex = 0);

  void nextOption();
  void prevOption();
  String getSelectedOption() const;
  void setCurrentIndex(int index);
  int getCurrentIndex() const { return currentIndex; }
  void setOnChange(std::function<void()> callback) { onChange = std::move(callback); }
  void toggleSelected();
  bool isSelected() const { return selected; }

  void run(int clicks = 1, int rawClicks = 1) override;
  String valueText() const override { return getSelectedOption(); }
  bool isEditing() const override { return selected; }
  void adjust(int steps) override;
  void stopEditing() override;

  /** How this item edits; `SelectEdit::Theme` follows the theme. */
  void setEditStyle(SelectEdit style) { editStyle = style; }
  SelectEdit getEditStyle() const { return editStyle; }
  const std::vector<String> &getOptions() const { return options; }

  void draw(int16_t x, int16_t y, bool active) override;
  int16_t editExtraHeight(int16_t rowH) const override;
  bool hasEditOverlay() const override;
  void drawEditOverlay(const Rect &area) override;
  bool editTap(int16_t x, int16_t y) override;
  bool editIsList() const override;
  bool editPointer(const InputEvent &e) override;

protected:
  bool editsInline() const override;
  bool editCycles() const override { return true; }

private:
  SelectEdit style() const;
  /** Segment strip of the Expand editor; false if the options do not fit side by side. */
  bool segmentLayout(Rect &strip) const;
  int optionAt(int16_t x, int16_t y) const;

  SelectEdit editStyle = SelectEdit::Theme;
  bool expandFallback = false; // Expand with options too wide for segments: edits inline
  // The sheet as last drawn, for taps: its box, the first option shown and option height.
  Rect sheet, sheetOptions;
  int sheetFirst = 0;
  int16_t sheetOptionH = 0;
  // Dragging the sheet down by its handle: where the finger started, and how far down it is.
  Rect sheetGrip;
  bool sheetDragging = false;
  int16_t sheetDragStartY = 0, sheetDragY = 0;
  std::vector<String> options;
  int currentIndex = 0;
  bool selected = false;
  std::function<void()> onChange;
};

// A scrolling list of MenuItems driven entirely by Input events, with optional
// nested submenus. Call draw() from the screen's draw and update() from its update.
class Menu
{
  friend class MenuItem;

public:
  explicit Menu(MenuSize size = MenuSize::Medium) : menuSize(size) {}

  void addMenuItem(MenuItem *item);

  /** Fixes the rows per page; otherwise derived from the panel. */
  void setItemsPerPage(uint8_t rows) { manualRowsPerPage = rows; }
  uint8_t getItemsPerPage() const;
  void setMenuSize(MenuSize size) { menuSize = size; }
  MenuSize getMenuSize() const { return menuSize; }
  /** Row layout; `MenuLook::Theme` follows the theme. */
  void setLook(MenuLook look) { menuLook = look; }
  MenuLook getLook() const { return menuLook; }

  void setActive(uint16_t index);
  uint16_t getActive() const { return active; }
  void nextItem();
  void prevItem();

  void setActiveSubmenu(Menu *submenu);
  void clearActiveSubmenu() { activeSubmenu = nullptr; }
  Menu *getActiveSubmenu() const { return activeSubmenu; }
  void setParentMenu(Menu *parent) { parentMenu = parent; }
  Menu *getParentMenu() const { return parentMenu; }

  void draw();
  void update();
  /** Leaves this submenu, or goes back a screen at the top level. */
  void back();

  std::vector<MenuItem *> items;

private:
  // Where rows go this frame: the list's first row y, its x and width, the
  // viewport height, and whether rows sit in a Grouped card.
  struct Frame
  {
    int16_t top, x, w, viewH;
    bool grouped;
  };
  Frame frame() const;
  bool grouped() const;

  void handle(const InputEvent &event);
  void tap(int16_t x, int16_t y);
  void moveBy(int steps);
  void ensureActiveVisible();
  void layoutPage(const std::vector<uint16_t> &visible, uint8_t rows);
  int slotAt(int16_t y) const; // visible-list index under y, or -1
  const std::vector<uint16_t> &visibleItems();
  std::vector<uint16_t> visibleScratch;

  // Only the row being edited grows: its slot and how many px it has grown by.
  int growSlot() const;
  int16_t growPx() const;
  int32_t slotTop(int slot) const;
  int16_t slotHeight(int slot) const;

  uint16_t active = 0;
  MenuLook menuLook = MenuLook::Theme;
  uint16_t topVisible = 0; // index into the visible-item list
  uint8_t manualRowsPerPage = 0;
  MenuSize menuSize;
  Menu *activeSubmenu = nullptr;
  Menu *parentMenu = nullptr;
  int16_t dragAccum = 0;

  // Motion: the list scroll and the highlight, both in px from the first row.
  Tween scrollY, highlightY;
  int laidOutRowH = 0;
  int32_t drawnScroll = 0; // scroll offset of the last drawn frame; taps hit-test against it

  // The editor growing in or out, and its progress 0-1.
  MenuItem *growItem = nullptr;
  Tween grow;
  int growVisibleSlot = -1; // growItem's slot in the last drawn visible list
};
