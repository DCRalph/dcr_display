#pragma once

#include <Arduino.h>

// Hardware-independent input. Sources (encoder, buttons, joystick, touch) turn
// their hardware into InputEvents; menus, popups and widgets only see events.
enum class InputKind : uint8_t
{
  Up,
  Down,
  Left,
  Right,
  Select,      // value = click count (normalised: the source's default select count reads as 1)
  Back,
  Delta,       // value = signed encoder steps, positive = clockwise / forward
  Tap,         // x, y = where a pointer pressed and released without moving
  PointerDown, // x, y
  PointerMove, // x, y; value = dy since the previous pointer event
  PointerUp,   // x, y
};

struct InputEvent
{
  InputKind kind;
  int16_t value = 0;
  int16_t rawClicks = 0; // Select only: clicks as the hardware reported them
  int16_t x = 0;
  int16_t y = 0;
};

class InputQueue
{
public:
  static constexpr size_t kCapacity = 16;

  void push(const InputEvent &event);
  void clear() { count_ = 0; }

  size_t size() const { return count_; }
  const InputEvent *begin() const { return events_; }
  const InputEvent *end() const { return events_ + count_; }

private:
  InputEvent events_[kCapacity];
  size_t count_ = 0;
};

class InputSource
{
public:
  virtual ~InputSource() = default;
  /** Called at most once per frame. Push this frame's events. */
  virtual void poll(InputQueue &queue) = 0;
  /** True for pointer devices; menus enlarge rows and show +/- targets when one is attached. */
  virtual bool isPointer() const { return false; }
};

namespace Input
{
  void addSource(InputSource &source);
  void removeSource(InputSource &source);

  /**
   * Starts a new frame. `Display::render` calls this. Sources are polled lazily,
   * on the first query of a frame, so screens that read hardware directly keep
   * working while no library consumer asks for input.
   */
  void beginFrame();

  /** This frame's events. Polls the sources on first use in a frame. */
  const InputQueue &events();

  /** True if any consumer polled this frame and events arrived. */
  bool hadEventsThisFrame();

  /** Inject an event, e.g. from a custom source outside the registry. */
  void inject(const InputEvent &event);

  bool hasPointer();
  uint32_t lastEventMs();
}
