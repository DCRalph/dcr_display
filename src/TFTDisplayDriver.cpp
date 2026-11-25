#include "TFTDisplayDriver.h"

#if __has_include(<TFT_eSPI.h>)

#undef LOG_TAG
#define LOG_TAG "TFT"
#include <dcr_Logger.h>

TFTDisplayDriver::TFTDisplayDriver(uint16_t width, uint16_t height, uint8_t rotation)
    : panel(), frame(&panel), w(width), h(height), rotation(rotation) {}

void TFTDisplayDriver::initPanel()
{
  panel.begin();
  panel.setRotation(rotation);
  panel.fillScreen(TFT_BLACK);
}

void TFTDisplayDriver::pushFrame()
{
  frame.pushSprite(0, 0);
}

bool TFTDisplayDriver::begin()
{
  initPanel();
  frame.setColorDepth(16);
  spriteReady = frame.createSprite(w, h) != nullptr;
  if (!spriteReady)
  {
    if (!allowDirectFallback())
    {
      debugE("No memory for a %ux%u frame", w, h);
      return false;
    }
    debugW("No memory for a %ux%u frame; drawing direct to the panel", w, h);
  }
  else
    frame.setSwapBytes(true);
  canvas().setTextDatum(TL_DATUM);
  return true;
}

void TFTDisplayDriver::push()
{
  if (spriteReady)
    pushFrame();
}

void TFTDisplayDriver::drawPixel(int32_t x, int32_t y, uint16_t color)
{
  canvas().drawPixel(x, y, color);
}

void TFTDisplayDriver::fillRect(int32_t x, int32_t y, int32_t rw, int32_t rh, uint16_t color)
{
  canvas().fillRect(x, y, rw, rh, color);
}

// Not frame.fillScreen(): TFT_eSprite inherits it from TFT_eSPI, which fills the
// panel's unrotated TFT_WIDTH x TFT_HEIGHT and so misses part of a rotated sprite.
void TFTDisplayDriver::fillScreen(uint16_t color)
{
  if (spriteReady)
    frame.fillSprite(color);
  else
    panel.fillScreen(color);
}

void TFTDisplayDriver::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color)
{
  canvas().drawLine(x0, y0, x1, y1, color);
}

void TFTDisplayDriver::drawRect(int32_t x, int32_t y, int32_t rw, int32_t rh, uint16_t color)
{
  canvas().drawRect(x, y, rw, rh, color);
}

void TFTDisplayDriver::drawRoundRect(int32_t x, int32_t y, int32_t rw, int32_t rh, int32_t r, uint16_t color)
{
  canvas().drawRoundRect(x, y, rw, rh, r, color);
}

void TFTDisplayDriver::fillRoundRect(int32_t x, int32_t y, int32_t rw, int32_t rh, int32_t r, uint16_t color)
{
  canvas().fillRoundRect(x, y, rw, rh, r, color);
}

void TFTDisplayDriver::drawSmoothArc(int32_t x, int32_t y, int32_t r, int32_t ir, int32_t startAngle, int32_t endAngle,
                                     uint16_t fgColor, uint16_t bgColor, bool roundEnds)
{
  canvas().drawSmoothArc(x, y, r, ir, startAngle, endAngle, fgColor, bgColor, roundEnds);
}

void TFTDisplayDriver::setFont(TextRole role)
{
  const uint8_t i = static_cast<uint8_t>(role);
  canvas().setTextSize(i < sizeof(roleScale) ? roleScale[i] : 2);
}

void TFTDisplayDriver::setTextScale(uint8_t size)
{
  canvas().setTextSize(size);
}

void TFTDisplayDriver::drawText(const char *text, int32_t x, int32_t y, uint16_t fg, int32_t bg)
{
  TFT_eSPI &c = canvas();
  if (bg == kThemeColor)
    c.setTextColor(fg);
  else
    c.setTextColor(fg, static_cast<uint16_t>(bg));
  c.setTextDatum(TL_DATUM);
  c.drawString(text, x, y);
}

int16_t TFTDisplayDriver::textWidth(const char *text)
{
  return canvas().textWidth(text);
}

int16_t TFTDisplayDriver::fontHeight()
{
  return canvas().fontHeight();
}

void TFTDisplayDriver::setRoleScale(TextRole role, uint8_t scale)
{
  const uint8_t i = static_cast<uint8_t>(role);
  if (i < sizeof(roleScale))
    roleScale[i] = scale;
}

#endif
