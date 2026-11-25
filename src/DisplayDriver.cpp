#include "DisplayDriver.h"

#include <math.h>

namespace
{
  int32_t clampRadius(int32_t r, int32_t w, int32_t h)
  {
    const int32_t limit = (w < h ? w : h) / 2;
    if (r > limit)
      r = limit;
    return r < 0 ? 0 : r;
  }
}

void DisplayDriver::fillScreen(uint16_t color)
{
  fillRect(0, 0, width(), height(), color);
}

void DisplayDriver::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color)
{
  if (x0 == x1)
  {
    fillRect(x0, y0 < y1 ? y0 : y1, 1, abs(y1 - y0) + 1, color);
    return;
  }
  if (y0 == y1)
  {
    fillRect(x0 < x1 ? x0 : x1, y0, abs(x1 - x0) + 1, 1, color);
    return;
  }

  const int32_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  const int32_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int32_t err = dx + dy;
  for (;;)
  {
    drawPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1)
      break;
    const int32_t e2 = 2 * err;
    if (e2 >= dy)
    {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

void DisplayDriver::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
{
  if (w <= 0 || h <= 0)
    return;
  fillRect(x, y, w, 1, color);
  fillRect(x, y + h - 1, w, 1, color);
  fillRect(x, y, 1, h, color);
  fillRect(x + w - 1, y, 1, h, color);
}

// quadrants: 1 top-left, 2 top-right, 4 bottom-right, 8 bottom-left.
void DisplayDriver::drawCircleQuadrants(int32_t cx, int32_t cy, int32_t r, uint8_t quadrants, uint16_t color)
{
  int32_t f = 1 - r, ddx = 1, ddy = -2 * r, x = 0, y = r;
  while (x <= y)
  {
    if (quadrants & 1)
    {
      drawPixel(cx - y, cy - x, color);
      drawPixel(cx - x, cy - y, color);
    }
    if (quadrants & 2)
    {
      drawPixel(cx + x, cy - y, color);
      drawPixel(cx + y, cy - x, color);
    }
    if (quadrants & 4)
    {
      drawPixel(cx + x, cy + y, color);
      drawPixel(cx + y, cy + x, color);
    }
    if (quadrants & 8)
    {
      drawPixel(cx - y, cy + x, color);
      drawPixel(cx - x, cy + y, color);
    }
    if (f >= 0)
    {
      y--;
      ddy += 2;
      f += ddy;
    }
    x++;
    ddx += 2;
    f += ddx;
  }
}

// TFT_eSPI's corner rasteriser, ported so buffered drivers keep its exact shapes
// while drawing through their own (fast) fillRect. corners: 1 bottom, 2 top.
void DisplayDriver::fillCircleHelper(int32_t x0, int32_t y0, int32_t r, uint8_t corners, int32_t delta, uint16_t color)
{
  int32_t f = 1 - r, ddF_x = 1, ddF_y = -r - r, y = 0;
  delta++;
  while (y < r)
  {
    if (f >= 0)
    {
      if (corners & 0x1)
        fillRect(x0 - y, y0 + r, y + y + delta, 1, color);
      if (corners & 0x2)
        fillRect(x0 - y, y0 - r, y + y + delta, 1, color);
      r--;
      ddF_y += 2;
      f += ddF_y;
    }
    y++;
    ddF_x += 2;
    f += ddF_x;
    if (corners & 0x1)
      fillRect(x0 - r, y0 + y, r + r + delta, 1, color);
    if (corners & 0x2)
      fillRect(x0 - r, y0 - y, r + r + delta, 1, color);
  }
}

void DisplayDriver::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color)
{
  r = clampRadius(r, w, h);
  if (r == 0)
  {
    drawRect(x, y, w, h, color);
    return;
  }
  fillRect(x + r, y, w - 2 * r, 1, color);
  fillRect(x + r, y + h - 1, w - 2 * r, 1, color);
  fillRect(x, y + r, 1, h - 2 * r, color);
  fillRect(x + w - 1, y + r, 1, h - 2 * r, color);
  drawCircleQuadrants(x + r, y + r, r, 1, color);
  drawCircleQuadrants(x + w - r - 1, y + r, r, 2, color);
  drawCircleQuadrants(x + w - r - 1, y + h - r - 1, r, 4, color);
  drawCircleQuadrants(x + r, y + h - r - 1, r, 8, color);
}

void DisplayDriver::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color)
{
  r = clampRadius(r, w, h);
  fillRect(x, y + r, w, h - r - r, color);
  fillCircleHelper(x + r, y + h - r - 1, r, 1, w - r - r - 1, color);
  fillCircleHelper(x + r, y + r, r, 2, w - r - r - 1, color);
}

// Ring drawn as horizontal spans: a full ring is two fills per row; an arc tests
// angles only inside the ring, never the whole bounding box. Not anti-aliased.
void DisplayDriver::drawSmoothArc(int32_t x, int32_t y, int32_t r, int32_t ir, int32_t startAngle, int32_t endAngle,
                                  uint16_t fgColor, uint16_t, bool)
{
  if (r <= 0 || ir > r)
    return;
  startAngle = ((startAngle % 360) + 360) % 360;
  endAngle = ((endAngle % 360) + 360) % 360;
  const bool full = startAngle == endAngle;
  const int64_t r2 = static_cast<int64_t>(r) * r, ir2 = static_cast<int64_t>(ir) * ir;

  for (int32_t dy = -r; dy <= r; dy++)
  {
    // Float estimates, corrected with exact integer tests: outer is the widest dx with
    // dx^2 <= r^2 - dy^2, inner the narrowest with dx^2 >= ir^2 - dy^2.
    const int64_t dy2 = static_cast<int64_t>(dy) * dy;
    const int64_t outerSq = r2 - dy2;
    int64_t outer = static_cast<int64_t>(sqrtf(static_cast<float>(outerSq)));
    while (outer > 0 && outer * outer > outerSq)
      outer--;
    while ((outer + 1) * (outer + 1) <= outerSq)
      outer++;
    const int64_t innerSq = ir2 - dy2;
    int64_t inner = -1;
    if (innerSq > 0)
    {
      inner = static_cast<int64_t>(ceilf(sqrtf(static_cast<float>(innerSq))));
      while (inner > 0 && (inner - 1) * (inner - 1) >= innerSq)
        inner--;
      while (inner * inner < innerSq)
        inner++;
    }

    if (full)
    {
      if (inner < 0)
        fillRect(x - static_cast<int32_t>(outer), y + dy, static_cast<int32_t>(2 * outer + 1), 1, fgColor);
      else if (inner <= outer)
      {
        fillRect(x - static_cast<int32_t>(outer), y + dy, static_cast<int32_t>(outer - inner + 1), 1, fgColor);
        fillRect(x + static_cast<int32_t>(inner), y + dy, static_cast<int32_t>(outer - inner + 1), 1, fgColor);
      }
      continue;
    }

    for (int32_t dx = static_cast<int32_t>(-outer); dx <= outer; dx++)
    {
      if (inner >= 0 && abs(dx) < inner)
        continue;
      // 0 deg at six o'clock, increasing clockwise.
      float a = atan2f(static_cast<float>(-dx), static_cast<float>(dy)) * 57.29578f;
      if (a < 0)
        a += 360.0f;
      const bool inside = startAngle < endAngle ? (a >= startAngle && a <= endAngle)
                                                : (a >= startAngle || a <= endAngle);
      if (inside)
        drawPixel(x + dx, y + dy, fgColor);
    }
  }
}
