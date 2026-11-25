#include "TFTDisplayDriver.h"

#if __has_include(<TFT_eSPI.h>)

// With LOAD_GFXFF, TFT_eSPI.h already declares every free font (FreeSans9pt7b ...).

#undef LOG_TAG
#define LOG_TAG "TFT"
#include <dcr_Logger.h>

TFTDisplayDriver::TFTDisplayDriver(uint16_t width, uint16_t height, uint8_t rotation)
    : panel(), frame(&panel), w(width), h(height), rotation(rotation)
{
  setRoleFont(TextRole::Small, nullptr, 1);
#ifdef LOAD_GFXFF
  setRoleFont(TextRole::Body, &FreeSans9pt7b);
  setRoleFont(TextRole::Title, &FreeSansBold12pt7b);
  setRoleFont(TextRole::XLarge, &FreeSansBold18pt7b);
#else
  setRoleFont(TextRole::Body, nullptr, 2);
  setRoleFont(TextRole::Title, nullptr, 3);
  setRoleFont(TextRole::XLarge, nullptr, 4);
#endif
}

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

void TFTDisplayDriver::pushRegion(int16_t x, int16_t y, int16_t rw, int16_t rh)
{
  frame.pushSprite(x, y, x, y, rw, rh);
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
  {
    frame.setSwapBytes(true);
    if (partialPush && !diff.begin(w, h))
      debugW("No memory for change tracking; pushing whole frames");
  }
  canvas().setTextDatum(TL_DATUM);
  return true;
}

void TFTDisplayDriver::push()
{
  if (!spriteReady)
    return;
  if (!diff.active())
  {
    pushFrame();
    pushedPercent = 100;
    return;
  }
  pushedPercent = diff.diff(static_cast<const uint16_t *>(frame.getPointer()), [this](const FrameDiff::Region &r)
                            { pushRegion(r.x, r.y, r.w, r.h); });
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

void TFTDisplayDriver::setRoleFont(TextRole role, const GFXfont *font, uint8_t scale)
{
  const uint8_t i = static_cast<uint8_t>(role);
  if (i >= sizeof(roleFonts) / sizeof(roleFonts[0]))
    return;
  RoleFont f{font, static_cast<uint8_t>(scale ? scale : 1), 0, 0};
#ifdef LOAD_GFXFF
  if (font)
  {
    // Same extent TFT_eSPI's TL_DATUM uses, so a role's height is its real glyph box.
    for (uint16_t c = 0; c <= font->last - font->first; c++)
    {
      const GFXglyph &g = font->glyph[c];
      f.ascent = max<int8_t>(f.ascent, -g.yOffset);
      f.descent = max<int8_t>(f.descent, g.height + g.yOffset);
    }
  }
#endif
  roleFonts[i] = f;
}

void TFTDisplayDriver::setFont(TextRole role)
{
  const uint8_t i = static_cast<uint8_t>(role);
  currentFont = &roleFonts[i < sizeof(roleFonts) / sizeof(roleFonts[0]) ? i : 1];
  TFT_eSPI &c = canvas();
#ifdef LOAD_GFXFF
  if (currentFont->font)
  {
    c.setFreeFont(currentFont->font);
    c.setTextSize(1);
    return;
  }
  c.setFreeFont(nullptr);
#endif
  c.setTextFont(1);
  c.setTextSize(currentFont->scale);
}

void TFTDisplayDriver::setTextScale(uint8_t size)
{
  currentFont = nullptr;
  TFT_eSPI &c = canvas();
#ifdef LOAD_GFXFF
  c.setFreeFont(nullptr);
#endif
  c.setTextFont(1);
  c.setTextSize(size);
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
  if (currentFont && currentFont->font)
    return currentFont->ascent + currentFont->descent;
  return canvas().fontHeight();
}

#endif
