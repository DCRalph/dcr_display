#include "ILI9341DisplayDriver.h"

#if __has_include(<Adafruit_ILI9341.h>)

#include "FastFill.h"

#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>

#undef LOG_TAG
#define LOG_TAG "ILI9341"
#include <dcr_Logger.h>

ILI9341DisplayDriver::ILI9341DisplayDriver(uint8_t spiBus, Pins pins, uint8_t rotation, uint32_t spiFrequency,
                                           bool buffered)
    : bus(spiBus), panel(&bus, pins.dc, pins.cs, pins.rst), wantBuffer(buffered), pins(pins), rotation(rotation),
      spiFrequency(spiFrequency)
{
  setRoleFont(TextRole::Small, nullptr, 1);
  setRoleFont(TextRole::Body, &FreeSans9pt7b);
  setRoleFont(TextRole::Title, &FreeSansBold12pt7b);
  setRoleFont(TextRole::XLarge, &FreeSansBold18pt7b);
}

void ILI9341DisplayDriver::setRoleFont(TextRole role, const GFXfont *font, uint8_t scale)
{
  const uint8_t i = static_cast<uint8_t>(role);
  if (i >= sizeof(roleFonts) / sizeof(roleFonts[0]))
    return;
  RoleFont f{font, static_cast<uint8_t>(scale ? scale : 1), 0, 0};
  if (font)
  {
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

ILI9341DisplayDriver::~ILI9341DisplayDriver()
{
  delete canvas;
}

bool ILI9341DisplayDriver::begin()
{
  // ESP32 SPIClass::begin() no-ops once initialised, so wiring the pins first
  // fixes the mapping for the panel's own begin() and any shared touch driver.
  bus.begin(pins.sclk, pins.miso, pins.mosi, pins.cs);
  panel.begin(spiFrequency);
  panel.setRotation(rotation);
  panel.fillScreen(ILI9341_BLACK);
  panel.setTextWrap(false); // Display does its own wrapping; GFX wrapping breaks width measurement
  if (wantBuffer)
  {
    canvas = new GFXcanvas16(panel.width(), panel.height());
    if (canvas->getBuffer() == nullptr)
    {
      delete canvas;
      canvas = nullptr;
      debugW("No memory for a frame buffer; drawing direct to the panel");
    }
    else
    {
      canvas->setTextWrap(false);
      appliedFont = nullptr; // fonts so far went to the panel; apply the next one to the canvas
    }
    if (canvas && partialPush && !diff.begin(canvas->width(), canvas->height()))
      debugW("No memory for change tracking; pushing whole frames");
  }
  return true;
}

void ILI9341DisplayDriver::push()
{
  if (!canvas)
    return;
  uint16_t *buffer = canvas->getBuffer();
  if (!diff.active())
  {
    panel.drawRGBBitmap(0, 0, buffer, canvas->width(), canvas->height());
    pushedPercent = 100;
    return;
  }

  const int16_t stride = canvas->width();
  panel.startWrite();
  pushedPercent = diff.diff(buffer, [&](const FrameDiff::Region &r)
                            {
                              panel.setAddrWindow(r.x, r.y, r.w, r.h);
                              for (int16_t row = 0; row < r.h; row++)
                                panel.writePixels(buffer + static_cast<size_t>(r.y + row) * stride + r.x, r.w); });
  panel.endWrite();
}

uint16_t ILI9341DisplayDriver::width() const
{
  return const_cast<Adafruit_ILI9341 &>(panel).width();
}

uint16_t ILI9341DisplayDriver::height() const
{
  return const_cast<Adafruit_ILI9341 &>(panel).height();
}

void ILI9341DisplayDriver::drawPixel(int32_t x, int32_t y, uint16_t color)
{
  target().drawPixel(x, y, color);
}

void ILI9341DisplayDriver::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
{
  if (!canvas)
  {
    panel.fillRect(x, y, w, h, color);
    return;
  }
  // Canvas fills in 32-bit words; GFXcanvas16 stores native-order pixels.
  if (FastFill::clip(x, y, w, h, canvas->width(), canvas->height()))
    FastFill::rect(canvas->getBuffer(), canvas->width(), x, y, w, h, color);
}

void ILI9341DisplayDriver::fillScreen(uint16_t color)
{
  if (canvas)
    fillRect(0, 0, canvas->width(), canvas->height(), color);
  else
    panel.fillScreen(color);
}

void ILI9341DisplayDriver::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color)
{
  target().drawLine(x0, y0, x1, y1, color);
}

void ILI9341DisplayDriver::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
{
  target().drawRect(x, y, w, h, color);
}

void ILI9341DisplayDriver::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color)
{
  target().drawRoundRect(x, y, w, h, r, color);
}

void ILI9341DisplayDriver::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color)
{
  if (canvas)
    DisplayDriver::fillRoundRect(x, y, w, h, r, color);
  else
    panel.fillRoundRect(x, y, w, h, r, color);
}

void ILI9341DisplayDriver::setFont(TextRole role)
{
  const uint8_t i = static_cast<uint8_t>(role);
  currentFont = &roleFonts[i < sizeof(roleFonts) / sizeof(roleFonts[0]) ? i : 1];
  if (currentFont == appliedFont)
    return;
  appliedFont = currentFont;
  target().setFont(currentFont->font);
  textScale = currentFont->font ? 1 : currentFont->scale;
  target().setTextSize(textScale);
}

void ILI9341DisplayDriver::setTextScale(uint8_t size)
{
  currentFont = appliedFont = nullptr;
  target().setFont(nullptr);
  textScale = size ? size : 1;
  target().setTextSize(textScale);
}

void ILI9341DisplayDriver::drawText(const char *text, int32_t x, int32_t y, uint16_t fg, int32_t bg)
{
  if (bg == kThemeColor)
    target().setTextColor(fg);
  else
    target().setTextColor(fg, static_cast<uint16_t>(bg));
  // GFX free fonts draw from the baseline; `y` is the top of the glyph box.
  const bool free = currentFont && currentFont->font;
  target().setCursor(x, free ? y + currentFont->ascent : y);
  target().print(text);
}

int16_t ILI9341DisplayDriver::textWidth(const char *text)
{
  if (currentFont && currentFont->font)
  {
    int16_t x1, y1;
    uint16_t bw, bh;
    target().getTextBounds(text, 0, 0, &x1, &y1, &bw, &bh);
    return static_cast<int16_t>(bw + (x1 > 0 ? x1 : 0));
  }
  return static_cast<int16_t>(strlen(text) * 6 * textScale);
}

int16_t ILI9341DisplayDriver::fontHeight()
{
  if (currentFont && currentFont->font)
    return currentFont->ascent + currentFont->descent;
  return static_cast<int16_t>(8 * textScale);
}

#endif
