#include "Input.h"

#include <vector>

void InputQueue::push(const InputEvent &event)
{
  if (count_ < kCapacity)
    events_[count_++] = event;
}

namespace
{
  std::vector<InputSource *> gSources;
  InputQueue gQueue;
  InputQueue gInjected;
  bool gPolled = false;
  uint32_t gLastEventMs = 0;
}

void Inputs::addSource(InputSource &source)
{
  for (InputSource *s : gSources)
    if (s == &source)
      return;
  gSources.push_back(&source);
}

void Inputs::removeSource(InputSource &source)
{
  for (auto it = gSources.begin(); it != gSources.end(); ++it)
  {
    if (*it == &source)
    {
      gSources.erase(it);
      return;
    }
  }
}

void Inputs::beginFrame()
{
  gPolled = false;
  gQueue.clear();
}

const InputQueue &Inputs::events()
{
  if (!gPolled)
  {
    gPolled = true;
    gQueue.clear();
    for (const InputEvent &e : gInjected)
      gQueue.push(e);
    gInjected.clear();
    for (InputSource *s : gSources)
      s->poll(gQueue);
    if (gQueue.size() > 0)
      gLastEventMs = millis();
  }
  return gQueue;
}

bool Inputs::hadEventsThisFrame()
{
  return gPolled && gQueue.size() > 0;
}

void Inputs::inject(const InputEvent &event)
{
  gInjected.push(event);
}

bool Inputs::hasPointer()
{
  for (InputSource *s : gSources)
    if (s->isPointer())
      return true;
  return false;
}

uint32_t Inputs::lastEventMs()
{
  return gLastEventMs;
}
