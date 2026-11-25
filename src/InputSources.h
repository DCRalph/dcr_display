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

// One touch reading. `z` is pressure in the controller's raw units, 0 if unknown.
struct TouchSample
{
  int16_t x = 0, y = 0;
  uint16_t z = 0;
};

// Pointer input with the filtering a resistive panel needs. The defaults do no
// filtering; the README has values that suit an XPT2046.
class TouchInput : public InputSource
{
public:
  struct Config
  {
    // Fill x, y in panel pixels and return true while touched. Or set readSample
    // instead, to supply pressure for startPressure/holdPressure.
    std::function<bool(int16_t &x, int16_t &y)> read;
    std::function<bool(TouchSample &sample)> readSample;

    uint8_t tapSlop = 10; // max travel in px for a tap
    uint16_t tapMaxMs = 700;

    // Gestures. 0 turns each one off.
    uint8_t swipeMinPx = 40;       // travel for a swipe, released within swipeMaxMs
    uint16_t swipeMaxMs = 600;
    uint16_t longPressMs = 550;    // held still this long
    uint16_t doubleTapMs = 320;    // second tap within this

    // Empty polls (one per frame) before a press counts as lifted. Above 1, a
    // press survives brief dropouts at its last position instead of ending.
    uint8_t releasePolls = 1;
    // readSample only: pressure above startPressure begins a press, above
    // holdPressure keeps one going. 0 disables each check.
    uint16_t startPressure = 0;
    uint16_t holdPressure = 0;
    // >0: take two samples per poll. If they differ by more than this many px
    // the reading is noise: it is ignored, or mid-press keeps the last position.
    uint8_t agreePx = 0;
    // Readings outside width x height are treated as noise. 0 disables.
    uint16_t width = 0, height = 0;
  };

  explicit TouchInput(Config config) : config_(std::move(config)) {}
  void poll(InputQueue &queue) override;
  bool isPointer() const override { return true; }

  /** Call between frames (e.g. between two displays' renders) to catch taps shorter than a frame. */
  void sample();
  /** Swallow the current press until the finger lifts, e.g. after it changed screens. */
  void suppressUntilRelease() { suppressed_ = true; }

private:
  bool readOnce(TouchSample &s);
  bool readFiltered(int16_t &x, int16_t &y);
  bool readDebounced(int16_t &x, int16_t &y);

  Config config_;
  bool down_ = false;
  int16_t startX_ = 0, startY_ = 0, lastX_ = 0, lastY_ = 0;
  int16_t goodX_ = 0, goodY_ = 0; // last reading that passed the filters
  bool moved_ = false;
  uint32_t downMs_ = 0;
  bool longFired_ = false;
  uint32_t lastTapMs_ = 0;
  int16_t lastTapX_ = 0, lastTapY_ = 0;
  uint32_t lastMoveMs_ = 0;
  float velocity_ = 0; // px per 100 ms, smoothed
  uint8_t missedPolls_ = 0;
  bool suppressed_ = false;
  bool hiddenPress_ = false; // the press began while suppressed
  bool midSeen_ = false;
  int16_t midX_ = 0, midY_ = 0;
};
