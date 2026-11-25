#include "InputSources.h"

namespace
{
  int read(const ClickReader &reader)
  {
    return reader ? reader() : 0;
  }

  void pushSelect(InputQueue &queue, int clicks, int defaultClicks)
  {
    InputEvent e{InputKind::Select};
    e.rawClicks = static_cast<int16_t>(clicks);
    e.value = static_cast<int16_t>(clicks == defaultClicks ? 1 : clicks);
    queue.push(e);
  }

  void pushKind(InputQueue &queue, InputKind kind)
  {
    queue.push(InputEvent{kind});
  }
}

void EncoderInput::poll(InputQueue &queue)
{
  int32_t delta = config_.readDelta ? config_.readDelta() : 0;
  if (delta != 0)
  {
    const uint32_t now = millis();
    if (config_.accelerate)
    {
      const uint32_t gap = now - lastStepMs_;
      if (gap < 25)
        delta *= 4;
      else if (gap < 60)
        delta *= 2;
    }
    lastStepMs_ = now;

    InputEvent e{InputKind::Delta};
    e.value = static_cast<int16_t>(delta * config_.direction);
    queue.push(e);
  }

  const int clicks = read(config_.readClicks);
  if (clicks != 0)
  {
    if (config_.backClicks != 0 && clicks == config_.backClicks)
      pushKind(queue, InputKind::Back);
    else
      pushSelect(queue, clicks, config_.defaultSelectClicks);
  }

  if (read(config_.readBackClicks) != 0)
    pushKind(queue, InputKind::Back);
}

void ButtonInput::poll(InputQueue &queue)
{
  const int up = read(config_.up);
  const int down = read(config_.down);

  if (config_.multiClickSelects && (up > 1 || down > 1))
    pushSelect(queue, up > 1 ? up : down, config_.defaultSelectClicks);
  else
  {
    if (up == 1)
      pushKind(queue, InputKind::Up);
    if (down == 1)
      pushKind(queue, InputKind::Down);
  }

  if (read(config_.left) == 1)
    pushKind(queue, InputKind::Left);
  if (read(config_.right) == 1)
    pushKind(queue, InputKind::Right);

  const int select = read(config_.select);
  if (select != 0)
    pushSelect(queue, select, config_.defaultSelectClicks);

  if (read(config_.back) != 0)
    pushKind(queue, InputKind::Back);
}

void JoystickInput::poll(InputQueue &queue)
{
  if (!config_.readX || !config_.readY)
    return;

  const int rawX = config_.readX();
  const int rawY = config_.readY();
  if (!calibrated_)
  {
    centerX_ = config_.center >= 0 ? config_.center : rawX;
    centerY_ = config_.center >= 0 ? config_.center : rawY;
    calibrated_ = true;
  }

  auto axis = [&](int raw, int center, bool invert) -> int8_t
  {
    const int d = raw - center;
    int8_t dir = d > config_.deadzone ? 1 : (d < -config_.deadzone ? -1 : 0);
    return invert ? -dir : dir;
  };
  const int8_t x = axis(rawX, centerX_, config_.invertX);
  const int8_t y = axis(rawY, centerY_, config_.invertY);

  // The dominant axis wins so a diagonal does not fire two directions.
  const int8_t dx = abs(rawX - centerX_) >= abs(rawY - centerY_) ? x : 0;
  const int8_t dy = dx == 0 ? y : 0;

  const uint32_t now = millis();
  bool fire = false;
  if (dx != heldX_ || dy != heldY_)
  {
    heldX_ = dx;
    heldY_ = dy;
    heldSinceMs_ = now;
    lastRepeatMs_ = now;
    fire = dx != 0 || dy != 0;
  }
  else if ((dx != 0 || dy != 0) && now - heldSinceMs_ >= config_.repeatDelayMs &&
           now - lastRepeatMs_ >= config_.repeatIntervalMs)
  {
    lastRepeatMs_ = now;
    fire = true;
  }

  if (fire)
  {
    if (dx > 0)
      pushKind(queue, InputKind::Right);
    else if (dx < 0)
      pushKind(queue, InputKind::Left);
    else if (dy > 0)
      pushKind(queue, InputKind::Down);
    else
      pushKind(queue, InputKind::Up);
  }

  const int clicks = read(config_.readClicks);
  if (clicks != 0)
    pushSelect(queue, clicks, config_.defaultSelectClicks);
}

bool TouchInput::readOnce(TouchSample &s)
{
  if (config_.readSample)
  {
    if (!config_.readSample(s))
      return false;
    const uint16_t threshold = down_ ? config_.holdPressure : config_.startPressure;
    return threshold == 0 || s.z > threshold;
  }
  return config_.read && config_.read(s.x, s.y);
}

// One filtered reading: pressure hysteresis, two-sample agreement and bounds.
// A noisy reading mid-press reports the last good position.
bool TouchInput::readFiltered(int16_t &x, int16_t &y)
{
  TouchSample a;
  if (!readOnce(a))
    return false;

  bool good = (config_.width == 0 || (a.x >= 0 && a.x < config_.width)) &&
              (config_.height == 0 || (a.y >= 0 && a.y < config_.height));
  if (config_.agreePx > 0)
  {
    TouchSample b;
    if (!readOnce(b))
      return false;
    good = good && abs(a.x - b.x) <= config_.agreePx && abs(a.y - b.y) <= config_.agreePx;
    a.x = (a.x + b.x) / 2;
    a.y = (a.y + b.y) / 2;
  }

  if (good)
  {
    goodX_ = a.x;
    goodY_ = a.y;
  }
  else if (!down_)
    return false;

  x = goodX_;
  y = goodY_;
  return true;
}

// readFiltered with release debounce: a press survives releasePolls - 1 empty polls.
bool TouchInput::readDebounced(int16_t &x, int16_t &y)
{
  if (readFiltered(x, y))
  {
    missedPolls_ = 0;
    return true;
  }
  if (down_ && ++missedPolls_ < config_.releasePolls)
  {
    x = goodX_;
    y = goodY_;
    return true;
  }
  return false;
}

void TouchInput::sample()
{
  int16_t x, y;
  if (!down_ && !midSeen_ && readFiltered(x, y))
  {
    midSeen_ = true;
    midX_ = x;
    midY_ = y;
  }
}

void TouchInput::poll(InputQueue &queue)
{
  if (!config_.read && !config_.readSample)
    return;

  int16_t x = 0, y = 0;
  bool touched = readDebounced(x, y);
  // A tap that began and ended between frames: report it now, it lifts next poll.
  if (!touched && !down_ && midSeen_)
  {
    touched = true;
    x = midX_;
    y = midY_;
  }
  midSeen_ = false;
  const uint32_t now = millis();

  // Hold the press state while suppressed so debounce and hysteresis still apply.
  // A press already visible when suppression began ends with PointerUp, no Tap.
  if (suppressed_)
  {
    if (touched)
    {
      if (!down_)
        hiddenPress_ = true;
      down_ = true;
      return;
    }
    if (down_ && !hiddenPress_)
    {
      InputEvent up{InputKind::PointerUp};
      up.x = lastX_;
      up.y = lastY_;
      queue.push(up);
    }
    down_ = hiddenPress_ = suppressed_ = false;
    return;
  }

  if (touched && !down_)
  {
    down_ = true;
    moved_ = false;
    startX_ = lastX_ = x;
    startY_ = lastY_ = y;
    downMs_ = now;
    InputEvent e{InputKind::PointerDown};
    e.x = x;
    e.y = y;
    queue.push(e);
    return;
  }

  if (touched && down_)
  {
    if (abs(x - startX_) > config_.tapSlop || abs(y - startY_) > config_.tapSlop)
      moved_ = true;
    if (x != lastX_ || y != lastY_)
    {
      InputEvent e{InputKind::PointerMove};
      e.x = x;
      e.y = y;
      e.value = static_cast<int16_t>(y - lastY_);
      queue.push(e);
      lastX_ = x;
      lastY_ = y;
    }
    return;
  }

  if (!touched && down_)
  {
    down_ = false;
    InputEvent up{InputKind::PointerUp};
    up.x = lastX_;
    up.y = lastY_;
    queue.push(up);
    if (!moved_ && now - downMs_ <= config_.tapMaxMs)
    {
      InputEvent tap{InputKind::Tap};
      tap.x = startX_;
      tap.y = startY_;
      queue.push(tap);
    }
  }
}
