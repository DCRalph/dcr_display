#include "TFTDisplayDriver.h"

#if __has_include(<TFT_eSPI.h>)

// With LOAD_GFXFF, TFT_eSPI.h already declares every free font (FreeSans9pt7b ...).

#include "FastFill.h"

#undef LOG_TAG
#define LOG_TAG "TFT"
#include <dcr_Logger.h>

TFTDisplayDriver::TFTDisplayDriver(uint16_t width, uint16_t height, uint8_t rotation)
    : panel(), frame(&panel), w(width), h(height), rotation(rotation)
{
#ifdef LOAD_GFXFF
  setRoleFont(TextRole::Small, nullptr, 1);
  setRoleFont(TextRole::Body, &FreeSans9pt7b);
  setRoleFont(TextRole::Title, &FreeSansBold12pt7b);
  setRoleFont(TextRole::XLarge, &FreeSansBold18pt7b);
#else
  setRoleScale(TextRole::Small, 1);
  setRoleScale(TextRole::Body, 2);
  setRoleScale(TextRole::Title, 3);
  setRoleScale(TextRole::XLarge, 4);
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
  target().setTextDatum(TL_DATUM);
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
  target().drawPixel(x, y, color);
}

// Sprite fills go straight into the frame in 32-bit words; TFT_eSprite stores pixels
// byte-swapped. Once app code holds the sprite, TFT_eSPI's own fill keeps any viewport.
void TFTDisplayDriver::fillRect(int32_t x, int32_t y, int32_t rw, int32_t rh, uint16_t color)
{
  if (!spriteReady || !rawFills)
  {
    target().fillRect(x, y, rw, rh, color);
    return;
  }
  if (FastFill::clip(x, y, rw, rh, w, h))
    FastFill::rect(static_cast<uint16_t *>(frame.getPointer()), w, x, y, rw, rh,
                   static_cast<uint16_t>(color >> 8 | color << 8));
}

// Not frame.fillScreen(): TFT_eSprite inherits it from TFT_eSPI, which fills the
// panel's unrotated TFT_WIDTH x TFT_HEIGHT and so misses part of a rotated sprite.
void TFTDisplayDriver::fillScreen(uint16_t color)
{
  fillRect(0, 0, w, h, color);
}

bool TFTDisplayDriver::dimRect(int32_t x, int32_t y, int32_t rw, int32_t rh)
{
  if (!spriteReady || !FastFill::clip(x, y, rw, rh, w, h))
    return spriteReady;
  // Sprite pixels are byte-swapped RGB565: unswap, halve each channel, swap back.
  uint16_t *frameBuf = static_cast<uint16_t *>(frame.getPointer());
  for (int32_t row = y; row < y + rh; row++)
  {
    uint16_t *p = frameBuf + row * w + x;
    for (int32_t i = 0; i < rw; i++)
    {
      const uint16_t c = static_cast<uint16_t>(p[i] >> 8 | p[i] << 8);
      const uint16_t half = (c >> 1) & 0x7BEF;
      p[i] = static_cast<uint16_t>(half >> 8 | half << 8);
    }
  }
  return true;
}

void TFTDisplayDriver::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color)
{
  target().drawLine(x0, y0, x1, y1, color);
}

void TFTDisplayDriver::drawRect(int32_t x, int32_t y, int32_t rw, int32_t rh, uint16_t color)
{
  target().drawRect(x, y, rw, rh, color);
}

void TFTDisplayDriver::drawRoundRect(int32_t x, int32_t y, int32_t rw, int32_t rh, int32_t r, uint16_t color)
{
  target().drawRoundRect(x, y, rw, rh, r, color);
}

void TFTDisplayDriver::fillRoundRect(int32_t x, int32_t y, int32_t rw, int32_t rh, int32_t r, uint16_t color)
{
  // The base version is TFT_eSPI's algorithm built on fillRect, so it gets the fast fill.
  if (spriteReady && rawFills)
    DisplayDriver::fillRoundRect(x, y, rw, rh, r, color);
  else
    target().fillRoundRect(x, y, rw, rh, r, color);
}

void TFTDisplayDriver::drawSmoothArc(int32_t x, int32_t y, int32_t r, int32_t ir, int32_t startAngle, int32_t endAngle,
                                     uint16_t fgColor, uint16_t bgColor, bool roundEnds)
{
  target().drawSmoothArc(x, y, r, ir, startAngle, endAngle, fgColor, bgColor, roundEnds);
}

#ifdef LOAD_GFXFF
void TFTDisplayDriver::setRoleFont(TextRole role, const GFXfont *font, uint8_t scale)
{
  const uint8_t i = static_cast<uint8_t>(role);
  if (i >= sizeof(roleFonts) / sizeof(roleFonts[0]))
    return;
  RoleFont f{font, static_cast<uint8_t>(scale ? scale : 1), 0, 0};
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
  roleFonts[i] = f;
  appliedFont = nullptr;
  if (currentFont == &roleFonts[i])
    setFont(role); // the active role: install the new font now so glyphs and metrics agree
}
#else
void TFTDisplayDriver::setRoleScale(TextRole role, uint8_t scale)
{
  const uint8_t i = static_cast<uint8_t>(role);
  if (i < sizeof(roleFonts) / sizeof(roleFonts[0]))
  {
    roleFonts[i] = RoleFont{nullptr, static_cast<uint8_t>(scale ? scale : 1), 0, 0};
    appliedFont = nullptr;
    if (currentFont == &roleFonts[i])
      setFont(role);
  }
}
#endif

void TFTDisplayDriver::setFont(TextRole role)
{
  const uint8_t i = static_cast<uint8_t>(role);
  currentFont = &roleFonts[i < sizeof(roleFonts) / sizeof(roleFonts[0]) ? i : 1];
  // setFreeFont rescans every glyph; menus ask for the same role row after row.
  if (currentFont == appliedFont)
    return;
  appliedFont = currentFont;
  TFT_eSPI &c = target();
#ifdef LOAD_GFXFF
  if (currentFont->font)
  {
    c.setFreeFont(static_cast<const GFXfont *>(currentFont->font));
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
  currentFont = appliedFont = nullptr;
  TFT_eSPI &c = target();
#ifdef LOAD_GFXFF
  c.setFreeFont(nullptr);
#endif
  c.setTextFont(1);
  c.setTextSize(size);
}

void TFTDisplayDriver::drawText(const char *text, int32_t x, int32_t y, uint16_t fg, int32_t bg)
{
  TFT_eSPI &c = target();
  if (bg == kThemeColor)
    c.setTextColor(fg);
  else
    c.setTextColor(fg, static_cast<uint16_t>(bg));
  c.setTextDatum(TL_DATUM);
  c.drawString(text, x, y);
}

int16_t TFTDisplayDriver::textWidth(const char *text)
{
  return target().textWidth(text);
}

int16_t TFTDisplayDriver::fontHeight()
{
  if (currentFont && currentFont->font)
    return currentFont->ascent + currentFont->descent;
  return target().fontHeight();
}

#endif
