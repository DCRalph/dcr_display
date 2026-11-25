#pragma once

#include <Arduino.h>
#include <functional>
#include <vector>

#include "Animation.h"
#include "Input.h"

struct Rect
{
  int16_t x = 0, y = 0, w = 0, h = 0;

  bool contains(int16_t px, int16_t py) const { return px >= x && px < x + w && py >= y && py < y + h; }
};

// Something FocusGroup can focus and activate. Subclasses draw themselves in
// `bounds` and react to select, taps and, if editable, value steps and drags.
class Widget
{
public:
  virtual ~Widget() = default;

  virtual void draw(bool focused, bool pressed) const = 0;
  /** Select, or a tap. */
  virtual void activate() {}
  /** Editable widgets take navigation steps while being edited (by buttons/encoder). */
  virtual bool editable() const { return false; }
  virtual void adjust(int steps) {}
  /** Pointer held at x, y (sliders follow the finger). */
  virtual void drag(int16_t x, int16_t y) {}

  Rect bounds;
  bool enabled = true;
  bool editing = false; // set by FocusGroup
};

// A pressable button: Primary is filled with the accent, Secondary is a surface
// with a hairline border, Ghost is accent text only.
class Button : public Widget
{
public:
  enum class Style : uint8_t
  {
    Primary,
    Secondary,
    Ghost
  };

  Button() = default;
  explicit Button(String label, std::function<void()> onPress = nullptr, Style style = Style::Secondary)
      : label(std::move(label)), onPress(std::move(onPress)), style(style) {}

  void draw(bool focused, bool pressed = false) const override;
  void activate() override { press(); }
  void press() const;

  String label;
  std::function<void()> onPress;
  Style style = Style::Secondary;
};

// Label with an on/off switch at the right; the knob slides when it changes.
class Switch : public Widget
{
public:
  Switch(String label, bool *value, std::function<void(bool)> onChange = nullptr)
      : label(std::move(label)), value(value), onChange(std::move(onChange)) {}

  void draw(bool focused, bool pressed) const override;
  void activate() override;

  String label;

private:
  bool *value;
  std::function<void(bool)> onChange;
  mutable Tween knob{0};
  mutable bool shown = false;
  mutable bool lastValue = false;
};

// Label and value over a track. Drag it, or select it and step with any input.
class Slider : public Widget
{
public:
  /** `step` below 1 counts as 1. */
  Slider(String label, int *value, int minValue, int maxValue, int step = 1, std::function<void(int)> onChange = nullptr)
      : label(std::move(label)), value(value), minValue(minValue), maxValue(maxValue), step(step),
        onChange(std::move(onChange)) {}

  void draw(bool focused, bool pressed) const override;
  bool editable() const override { return true; }
  void activate() override {}
  void adjust(int steps) override;
  void drag(int16_t x, int16_t y) override;

  String label;
  String suffix; // appended to the value, e.g. "%"

private:
  void set(int v);
  int stepSize() const;
  Rect track() const;

  int *value;
  int minValue, maxValue, step;
  std::function<void(int)> onChange;
};

// Focus and activation for widgets laid out `columns` wide, from any input
// source. Encoder steps and left/right walk row by row; up/down walk column by
// column, so a device with only up/down buttons still reaches every widget.
// Select on an editable widget (a slider) enters editing: steps then change it.
class FocusGroup
{
public:
  void add(Widget &widget) { widgets.push_back(&widget); }
  void clear();
  void setColumns(uint8_t cols) { columns = cols < 1 ? 1 : cols; }
  void setFocus(size_t index);
  size_t focus() const { return focused; }

  /** Handles this frame's input. True if anything was consumed. */
  bool update();
  void draw() const;

private:
  void move(int steps);
  void moveVertical(int steps);
  Widget *current() const { return widgets.empty() ? nullptr : widgets[focused]; }

  std::vector<Widget *> widgets;
  size_t focused = 0;
  int pressed = -1; // widget under a held pointer
  uint8_t columns = 1;
};

// On-screen keyboard that fills `bounds`: letters, shift, and two symbol layers
// covering all printable ASCII. Works by touch, joystick, buttons or encoder:
// up/down wrap into the next column, so up/down alone reach every key.
class Keyboard
{
public:
  std::function<void(char)> onKey;
  std::function<void()> onDelete;
  std::function<void()> onDone;
  std::function<void()> onCancel; // Back input

  void setBounds(const Rect &area) { bounds = area; }
  void draw();
  /** True if anything was consumed. */
  bool update();

private:
  enum class Layer : uint8_t
  {
    Lower,
    Upper,
    Sym1,
    Sym2
  };
  enum class Special : uint8_t
  {
    None,
    Shift,
    Mode,
    Space,
    Delete,
    Done
  };

  static constexpr uint8_t kCols = 10;
  static constexpr uint8_t kRows = 4;

  Rect keyRect(uint8_t row, uint8_t col) const;
  char charAt(uint8_t row, uint8_t col) const;
  Special specialAt(uint8_t col) const;
  const char *labelAt(uint8_t row, uint8_t col, char *buf) const;
  void activate(uint8_t row, uint8_t col);
  void moveFocus(int dRow, int dCol);
  void linearStep(int steps);
  bool firstColOfKey(uint8_t row, uint8_t col) const;
  /** A key that can take focus: the first column of a key, and not blank on this layer. */
  bool focusable(uint8_t row, uint8_t col) const;

  Rect bounds;
  Layer layer = Layer::Lower;
  uint8_t focusRow = 0, focusCol = 0;
  uint8_t cursorCol = 0; // column vertical moves track, before snapping to a wide key
};
