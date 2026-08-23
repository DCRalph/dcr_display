#pragma once

#include "Display.h"

class Popup
{
public:
  Popup(String _title, String _message);
  virtual void draw();
  virtual void update();

  String title;
  String message;
};

class AutoClosePopup : public Popup
{
private:
  unsigned long startTime;
  unsigned long duration;
  /** Remaining time as 0-100; converted to pixels at draw time. */
  uint8_t progressPercent = 100;

public:
  AutoClosePopup(String _title, String _message, unsigned long _duration);
  void draw() override;
  void update() override;
};
