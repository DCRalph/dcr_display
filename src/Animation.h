#pragma once

#include <Arduino.h>

// Easing curves: t in [0, 1] to progress in [0, 1] (outBack overshoots).
namespace Ease
{
  using Fn = float (*)(float t);
  float linear(float t);
  float outQuad(float t);
  float outCubic(float t);
  float inOutCubic(float t);
  float outBack(float t);
}

// A value moving from one number to another over time. Starting one keeps its
// display redrawing until it ends; on a display with animations off it jumps
// straight to the target. Call start() from that display's draw or update.
class Tween
{
public:
  Tween(float initial = 0) : from_(initial), to_(initial) {}

  void start(float to, uint16_t durationMs, Ease::Fn ease = Ease::outCubic);
  void start(float from, float to, uint16_t durationMs, Ease::Fn ease = Ease::outCubic);
  /** Jump to `v` with no animation. */
  void snap(float v);

  float value() const;
  float target() const { return to_; }
  bool running() const;

private:
  float from_, to_;
  uint32_t startMs_ = 0;
  uint16_t durationMs_ = 0;
  Ease::Fn ease_ = Ease::outCubic;
};
