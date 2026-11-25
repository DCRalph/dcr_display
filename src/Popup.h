#pragma once

#include <Arduino.h>
#include <functional>
#include <vector>

#include "Animation.h"
#include "Widgets.h"

// Modal box over the current screen. While one is showing, the screen keeps
// drawing but its update() does not run. Show with `screenManager.showPopup(new ...)`;
// the manager owns and deletes it.
class Popup
{
public:
  Popup(String title, String message) : title(std::move(title)), message(std::move(message)) {}
  virtual ~Popup() = default;

  /** Closes on Select, Back, a tap or a swipe. */
  virtual void update();
  virtual void draw();
  void close();

  String title;
  String message;

protected:
  /** Box and text area in panel coordinates, from the display profile. */
  struct Box
  {
    int32_t x, y, w, h;
    int32_t textX, textY, textW, textH;
  };

  /** Includes the slide-in offset while the popup is appearing. */
  Box box() const;
  /** Frame, title and wrapped message; returns the y below the message area. */
  void drawFrame(const Box &b);
  void drawText(const Box &b, uint16_t titleColor);

private:
  mutable Tween enter;
  mutable bool entered = false;
};

// Closes itself after `durationMs`, or early on input, with a countdown bar.
class AutoClosePopup : public Popup
{
public:
  AutoClosePopup(String title, String message, unsigned long durationMs);
  void update() override;
  void draw() override;

private:
  unsigned long startMs;
  unsigned long durationMs;
  uint8_t remainingPercent = 100;
};

// Message plus a row of choices. `onChoose` receives the chosen index, or -1 on Back.
class ChoicePopup : public Popup
{
public:
  ChoicePopup(String title, String message, std::vector<String> choices, std::function<void(int)> onChoose);
  void update() override;
  void draw() override;

private:
  void layout(const Box &b);

  std::vector<Button> buttons;
  FocusGroup group;
  std::function<void(int)> onChoose;
  int chosen = -2;
};
