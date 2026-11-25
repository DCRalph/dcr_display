#include "FrameDiff.h"

#include <esp_heap_caps.h>

namespace
{
  // One multiply per pixel pair keeps this near PSRAM read speed. The rotation
  // carries high-bit differences down so a later word cannot cancel them.
  // Internal RAM first: the table is read every frame. PSRAM if that is short.
  void *allocate(size_t bytes)
  {
    void *p = heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    return p ? p : heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }

  uint32_t hashCell(const uint16_t *px, uint16_t count)
  {
    uint32_t a = 0x811C9DC5u;
    const uint16_t pairs = count / 2;
    const auto *words = reinterpret_cast<const uint32_t *>(px);
    for (uint16_t i = 0; i < pairs; ++i)
    {
      a = (a ^ words[i]) * 0x9E3779B1u;
      a = (a << 15) | (a >> 17);
    }
    if (count & 1)
      a = (a ^ px[count - 1]) * 0x9E3779B1u;
    return a;
  }
}

bool FrameDiff::begin(uint16_t width, uint16_t height, uint8_t cellPx, uint8_t bandRows)
{
  w = width;
  h = height;
  cell = cellPx & ~1; // even, so cells start on 32-bit words
  if (cell < 2)
    cell = 2;
  bandRowCount = bandRows < 1 ? 1 : bandRows;
  cellsPerRow = (w + cell - 1) / cell;
  inSync = false;

  bands = (h + bandRowCount - 1) / bandRowCount;

  release();
  hashes = static_cast<uint32_t *>(allocate(static_cast<size_t>(cellsPerRow) * h * sizeof(uint32_t)));
  bandMin = static_cast<int16_t *>(allocate(bands * sizeof(int16_t)));
  bandMax = static_cast<int16_t *>(allocate(bands * sizeof(int16_t)));
  if (!hashes || !bandMin || !bandMax)
  {
    release();
    return false;
  }
  return true;
}

void FrameDiff::release()
{
  heap_caps_free(hashes);
  heap_caps_free(bandMin);
  heap_caps_free(bandMax);
  hashes = nullptr;
  bandMin = bandMax = nullptr;
}

void FrameDiff::markChanges(const uint16_t *frame)
{
  for (size_t b = 0; b < bands; ++b)
  {
    bandMin[b] = cellsPerRow;
    bandMax[b] = -1;
  }

  uint32_t *stored = hashes;
  for (uint16_t y = 0; y < h; ++y)
  {
    const uint16_t *row = frame + static_cast<size_t>(y) * w;
    for (uint16_t c = 0; c < cellsPerRow; ++c, ++stored)
    {
      const uint16_t x = c * cell;
      const uint32_t hash = hashCell(row + x, min<uint16_t>(cell, w - x));
      if (inSync && *stored == hash)
        continue;
      *stored = hash;
      const size_t b = y / bandRowCount;
      if (c < bandMin[b])
        bandMin[b] = c;
      if (static_cast<int16_t>(c) > bandMax[b])
        bandMax[b] = c;
    }
  }
  inSync = true;
}
