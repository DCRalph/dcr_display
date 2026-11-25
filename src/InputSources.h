#pragma once

#include <functional>

#include "Input.h"

// Ready-made sources. Each takes plain callbacks, so they work with any button,
// encoder or touch library. Register with `Inputs::addSource(source)`.

// Click counts follow ClickButton: 1 single, 2 double, negative = long press.
using ClickReader = std::function<int()>;

class EncoderInput : public InputSource
{
public:
  struct Config
  {
    std::function<int32_t()> readDelta; // detents since the previous call (read and clear)
    ClickReader readClicks;             // encoder push button, optional
    ClickReader readBackClicks;         // dedicated back button, optional

    int8_t direction = 1;        // -1 flips clockwise
    int defaultSelectClicks = 1; // this many clicks reads as a single Select
    int backClicks = 0;          // push-button clicks that mean Back instead (0 = none)
    bool accelerate = true;      // fast spins move further
  };

  explicit EncoderInput(Config config) : config_(std::move(config)) {}
  void poll(InputQueue &queue) override;

private:
  Config config_;
  uint32_t lastStepMs_ = 0;
};

class ButtonInput : public InputSource
{
public:
  struct Config
  {
    ClickReader up, down, left, right, select, back; // any may be empty

    int defaultSelectClicks = 1;
    // Two-button devices: more than one click on up/down selects instead of moving.
    bool multiClickSelects = false;
  };

  explicit ButtonInput(Config config) : config_(std::move(config)) {}
  void poll(InputQueue &queue) override;

private:
  Config config_;
};

class JoystickInput : public InputSource
{
public:
  struct Config
  {
    std::function<int()> readX, readY; // raw ADC
    ClickReader readClicks;            // stick press, optional

    int center = -1;    // -1 samples the resting position on the first poll
    int deadzone = 700; // raw units either side of center
    bool invertX = false;
    bool invertY = false;
    uint16_t repeatDelayMs = 400;
    uint16_t repeatIntervalMs = 120;
    int defaultSelectClicks = 1;
  };

  explicit JoystickInput(Config config) : config_(std::move(config)) {}
  void poll(InputQueue &queue) override;

private:
  Config config_;
  int centerX_ = 0, centerY_ = 0;
  bool calibrated_ = false;
  int8_t heldX_ = 0, heldY_ = 0;
  uint32_t heldSinceMs_ = 0, lastRepeatMs_ = 0;
};

class TouchInput : public InputSource
{
public:
  struct Config
  {
    // Fill x, y in panel pixels and return true while touched.
    std::function<bool(int16_t &x, int16_t &y)> read;
    uint8_t tapSlop = 10;       // max travel in px for a tap
    uint16_t tapMaxMs = 700;
  };

  explicit TouchInput(Config config) : config_(std::move(config)) {}
  void poll(InputQueue &queue) override;
  bool isPointer() const override { return true; }

private:
  Config config_;
  bool down_ = false;
  int16_t startX_ = 0, startY_ = 0, lastX_ = 0, lastY_ = 0;
  bool moved_ = false;
  uint32_t downMs_ = 0;
};
