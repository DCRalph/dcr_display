#include "Animation.h"

#include "Display.h"

float Ease::linear(float t)
{
  return t;
}

float Ease::outQuad(float t)
{
  return 1 - (1 - t) * (1 - t);
}

float Ease::outCubic(float t)
{
  const float u = 1 - t;
  return 1 - u * u * u;
}

float Ease::inOutCubic(float t)
{
  return t < 0.5f ? 4 * t * t * t : 1 - powf(-2 * t + 2, 3) / 2;
}

float Ease::outBack(float t)
{
  constexpr float c1 = 1.70158f, c3 = c1 + 1;
  const float u = t - 1;
  return 1 + c3 * u * u * u + c1 * u * u;
}

void Tween::start(float to, uint16_t durationMs, Ease::Fn ease)
{
  start(value(), to, durationMs, ease);
}

void Tween::start(float from, float to, uint16_t durationMs, Ease::Fn ease)
{
  Display &d = Display::active();
  if (!d.animationsEnabled() || durationMs == 0 || from == to)
  {
    snap(to);
    return;
  }
  from_ = from;
  to_ = to;
  startMs_ = millis();
  durationMs_ = durationMs;
  ease_ = ease ? ease : Ease::outCubic;
  d.animateFor(durationMs);
}

void Tween::snap(float v)
{
  from_ = to_ = v;
  durationMs_ = 0;
}

float Tween::value() const
{
  if (durationMs_ == 0)
    return to_;
  const uint32_t elapsed = millis() - startMs_;
  if (elapsed >= durationMs_)
    return to_;
  return from_ + (to_ - from_) * ease_(static_cast<float>(elapsed) / durationMs_);
}

bool Tween::running() const
{
  return durationMs_ != 0 && millis() - startMs_ < durationMs_;
}
