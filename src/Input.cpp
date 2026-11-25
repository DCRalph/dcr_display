#include "Input.h"

#include "Display.h"

void InputQueue::push(const InputEvent &event)
{
  if (count_ < kCapacity)
    events_[count_++] = event;
}

namespace
{
  InputSet &set()
  {
    return Display::active().inputs();
  }
}

void Inputs::addSource(InputSource &source)
{
  Display::active().addInput(source);
}

void Inputs::removeSource(InputSource &source)
{
  Display::active().removeInput(source);
}

void Inputs::beginFrame()
{
  set().polled = false;
  set().queue.clear();
}

const InputQueue &Inputs::events()
{
  InputSet &s = set();
  if (!s.polled)
  {
    s.polled = true;
    s.queue.clear();
    for (const InputEvent &e : s.injected)
      s.queue.push(e);
    s.injected.clear();
    for (InputSource *src : s.sources)
      src->poll(s.queue);
    if (s.queue.size() > 0)
      s.lastEventMs = millis();
  }
  return s.queue;
}

bool Inputs::hadEventsThisFrame()
{
  return set().polled && set().queue.size() > 0;
}

void Inputs::inject(const InputEvent &event)
{
  set().injected.push(event);
}

bool Inputs::hasPointer()
{
  for (InputSource *src : set().sources)
    if (src->isPointer())
      return true;
  return false;
}

uint32_t Inputs::lastEventMs()
{
  return set().lastEventMs;
}
