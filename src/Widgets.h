#pragma once

#include <Arduino.h>
#include <functional>
#include <vector>

#include "Input.h"

struct Rect
{
  int16_t x = 0, y = 0, w = 0, h = 0;

  bool contains(int16_t px, int16_t py) const { return px >= x && px < x + w && py >= y && py < y + h; }
};

// A pressable box. Draws in the theme's widget colours, filled when focused.
class Button
{
public:
  Button() = default;
  explicit Button(String label, std::function<void()> onPress = nullptr)
      : label(std::move(label)), onPress(std::move(onPress)) {}

  void draw(bool focused) const;
  void press() const;

  Rect bounds;
  String label;
  std::function<void()> onPress;
  bool enabled = true;
};

// Focus and activation for buttons laid out `columns` wide, from any input
// source. Encoder steps and left/right walk row by row; up/down walk column by
// column, so a device with only up/down buttons still reaches every button.
class FocusGroup
{
public:
  void add(Button &button) { buttons.push_back(&button); }
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

  std::vector<Button *> buttons;
  size_t focused = 0;
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
