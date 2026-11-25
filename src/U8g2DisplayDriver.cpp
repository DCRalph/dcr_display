#include "U8g2DisplayDriver.h"

#if __has_include(<U8g2lib.h>)

#undef LOG_TAG
#define LOG_TAG "U8G2"
#include <dcr_Logger.h>

namespace
{
  // 1-bit: any colour but black lights the pixel, so colour app code stays visible.
  uint8_t ink(uint16_t color)
  {
    return color == Color::Black ? 0 : 1;
  }

  // u8g2's rounded primitives need a radius that fits inside the box.
  int32_t clampRadius(int32_t r, int32_t w, int32_t h)
  {
    const int32_t limit = (w < h ? w : h) / 2 - 1;
    return r > limit ? (limit < 0 ? 0 : limit) : (r < 0 ? 0 : r);
  }
}

const uint8_t *U8g2FontSet::forRole(TextRole role) const
{
  switch (role)
  {
  case TextRole::Small:
    return small;
  case TextRole::Title:
    return title;
  case TextRole::XLarge:
    return xlarge;
  default:
    return body;
  }
}

U8g2DisplayDriver::~U8g2DisplayDriver()
{
  free(shown);
}

bool U8g2DisplayDriver::begin()
{
  if (!u8g2.begin())
    return false;
  if (u8g2.getBufferTileHeight() * 8 < u8g2.getDisplayHeight())
  {
    debugE("Paged U8g2 device: use a full-buffer (_F_) panel class");
    return false;
  }
  if (partialPush)
    shown = static_cast<uint8_t *>(malloc(static_cast<size_t>(u8g2.getBufferTileWidth()) * 8 * u8g2.getBufferTileHeight()));
  u8g2.setFont(fonts.forRole(TextRole::Body));
  u8g2.setFontPosTop();
  u8g2.setFontMode(1);
  u8g2.setDrawColor(1);
  return true;
}

// Each tile row (8 px) is a contiguous tileWidth * 8 bytes in every U8g2 buffer
// layout, so a whole changed row is sent: no assumption about the byte order inside.
void U8g2DisplayDriver::push()
{
  const uint8_t tw = u8g2.getBufferTileWidth(), th = u8g2.getBufferTileHeight();
  const size_t rowBytes = static_cast<size_t>(tw) * 8;
  const uint8_t *buffer = u8g2.getBufferPtr();
  if (!shown || !panelInSync)
  {
    u8g2.sendBuffer();
    if (shown)
      memcpy(shown, buffer, rowBytes * th);
    panelInSync = true;
    pushedPercent = 100;
    return;
  }

  uint8_t sentRows = 0;
  for (uint8_t row = 0; row < th;)
  {
    if (memcmp(buffer + row * rowBytes, shown + row * rowBytes, rowBytes) == 0)
    {
      row++;
      continue;
    }
    uint8_t end = row + 1;
    while (end < th && memcmp(buffer + end * rowBytes, shown + end * rowBytes, rowBytes) != 0)
      end++;
    u8g2.updateDisplayArea(0, row, tw, end - row);
    memcpy(shown + row * rowBytes, buffer + row * rowBytes, rowBytes * (end - row));
    sentRows += end - row;
    row = end;
  }
  pushedPercent = static_cast<uint8_t>(sentRows * 100 / th);
}

void U8g2DisplayDriver::drawPixel(int32_t x, int32_t y, uint16_t color)
{
  u8g2.setDrawColor(ink(color));
  u8g2.drawPixel(x, y);
}

void U8g2DisplayDriver::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
{
  if (w <= 0 || h <= 0)
    return;
  u8g2.setDrawColor(ink(color));
  u8g2.drawBox(x, y, w, h);
}

void U8g2DisplayDriver::fillScreen(uint16_t color)
{
  u8g2.clearBuffer();
  if (ink(color))
  {
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 0, width(), height());
  }
}

void U8g2DisplayDriver::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color)
{
  u8g2.setDrawColor(ink(color));
  u8g2.drawLine(x0, y0, x1, y1);
}

void U8g2DisplayDriver::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
{
  if (w <= 0 || h <= 0)
    return;
  u8g2.setDrawColor(ink(color));
  u8g2.drawFrame(x, y, w, h);
}

void U8g2DisplayDriver::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color)
{
  if (w <= 0 || h <= 0)
    return;
  r = clampRadius(r, w, h);
  u8g2.setDrawColor(ink(color));
  r > 0 ? u8g2.drawRFrame(x, y, w, h, r) : u8g2.drawFrame(x, y, w, h);
}

void U8g2DisplayDriver::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color)
{
  if (w <= 0 || h <= 0)
    return;
  r = clampRadius(r, w, h);
  u8g2.setDrawColor(ink(color));
  r > 0 ? u8g2.drawRBox(x, y, w, h, r) : u8g2.drawBox(x, y, w, h);
}

void U8g2DisplayDriver::setFont(TextRole role)
{
  u8g2.setFont(fonts.forRole(role));
  u8g2.setFontPosTop();
}

void U8g2DisplayDriver::drawText(const char *text, int32_t x, int32_t y, uint16_t fg, int32_t bg)
{
  if (bg != kThemeColor && ink(static_cast<uint16_t>(bg)) != ink(fg))
  {
    u8g2.setDrawColor(ink(static_cast<uint16_t>(bg)));
    u8g2.drawBox(x, y, textWidth(text), fontHeight());
  }
  u8g2.setDrawColor(ink(fg));
  u8g2.setFontPosTop();
  u8g2.drawStr(x, y, text);
}

int16_t U8g2DisplayDriver::textWidth(const char *text)
{
  return u8g2.getStrWidth(text);
}

int16_t U8g2DisplayDriver::fontHeight()
{
  return u8g2.getAscent() - u8g2.getDescent();
}

uint16_t U8g2DisplayDriver::color565(uint8_t r, uint8_t g, uint8_t b)
{
  return (r * 299 + g * 587 + b * 114) / 1000 > 127 ? Color::White : Color::Black;
}

#endif
