#pragma once

#include <Arduino.h>

// Finds what changed in a 16-bit frame since the last push, so a buffered
// driver sends only that. Each row is hashed in cells `cellPx` wide against a
// table of what the panel shows; changes go out per band of `bandRows` rows,
// each as wide as its own changes, with neighbouring bands of equal span merged.
// From the bioreactor controller, where a scrolling list went from 26 to 40 fps.
class FrameDiff
{
public:
  struct Region
  {
    int16_t x, y, w, h;
  };

  FrameDiff() = default;
  FrameDiff(const FrameDiff &) = delete;
  FrameDiff &operator=(const FrameDiff &) = delete;
  ~FrameDiff() { release(); }

  /** False if the hash table did not fit; diff() then reports whole frames. */
  bool begin(uint16_t width, uint16_t height, uint8_t cellPx = 40, uint8_t bandRows = 2);
  bool active() const { return hashes != nullptr; }

  /** Report the whole frame next time, e.g. after drawing straight to the panel. */
  void invalidate() { inSync = false; }

  /**
   * Calls `push(region)` for every changed region of `frame` (row-major, `width`
   * pixels per row) and records the frame as shown. Returns the share of the
   * frame pushed, 0-100.
   */
  template <typename Push>
  uint8_t diff(const uint16_t *frame, Push &&push)
  {
    if (!active())
    {
      push(Region{0, 0, static_cast<int16_t>(w), static_cast<int16_t>(h)});
      return 100;
    }

    markChanges(frame);

    uint32_t pushedPx = 0;
    for (size_t b = 0; b < bands;)
    {
      if (bandMax[b] < 0)
      {
        ++b;
        continue;
      }
      size_t end = b + 1;
      while (end < bands && bandMin[end] == bandMin[b] && bandMax[end] == bandMax[b])
        ++end;

      const int16_t x0 = bandMin[b] * cell;
      const int16_t x1 = min<int32_t>(w, (bandMax[b] + 1) * cell);
      const int16_t y0 = b * bandRowCount;
      const int16_t y1 = min<int32_t>(h, end * bandRowCount);
      push(Region{x0, y0, static_cast<int16_t>(x1 - x0), static_cast<int16_t>(y1 - y0)});
      pushedPx += static_cast<uint32_t>(x1 - x0) * (y1 - y0);
      b = end;
    }
    return static_cast<uint8_t>(pushedPx * 100 / (static_cast<uint32_t>(w) * h));
  }

private:
  void markChanges(const uint16_t *frame);
  void release();

  uint16_t w = 0, h = 0;
  uint8_t cell = 40, bandRowCount = 2;
  uint16_t cellsPerRow = 0;
  size_t bands = 0;
  uint32_t *hashes = nullptr; // one per cell of every row: what the panel shows
  int16_t *bandMin = nullptr;
  int16_t *bandMax = nullptr;
  bool inSync = false;
};
