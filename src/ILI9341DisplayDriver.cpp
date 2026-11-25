#include "ILI9341DisplayDriver.h"

#if __has_include(<Adafruit_ILI9341.h>)

#undef LOG_TAG
#define LOG_TAG "ILI9341"
#include <dcr_Logger.h>

ILI9341DisplayDriver::ILI9341DisplayDriver(uint8_t spiBus, Pins pins, uint8_t rotation, uint32_t spiFrequency,
                                           bool buffered)
    : bus(spiBus), panel(&bus, pins.dc, pins.cs, pins.rst), wantBuffer(buffered), pins(pins), rotation(rotation),
      spiFrequency(spiFrequency) {}

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
  if (wantBuffer)
  {
    canvas = new GFXcanvas16(panel.width(), panel.height());
    if (canvas->getBuffer() == nullptr)
    {
      delete canvas;
      canvas = nullptr;
      debugW("No memory for a frame buffer; drawing direct to the panel");
    }
    else if (partialPush && !diff.begin(canvas->width(), canvas->height()))
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
  gfx().drawPixel(x, y, color);
}

void ILI9341DisplayDriver::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
{
  gfx().fillRect(x, y, w, h, color);
}

void ILI9341DisplayDriver::fillScreen(uint16_t color)
{
  gfx().fillScreen(color);
}

void ILI9341DisplayDriver::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color)
{
  gfx().drawLine(x0, y0, x1, y1, color);
}

void ILI9341DisplayDriver::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
{
  gfx().drawRect(x, y, w, h, color);
}

void ILI9341DisplayDriver::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color)
{
  gfx().drawRoundRect(x, y, w, h, r, color);
}

void ILI9341DisplayDriver::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color)
{
  gfx().fillRoundRect(x, y, w, h, r, color);
}

void ILI9341DisplayDriver::setFont(TextRole role)
{
  textScale = defaultTextScaleFor(role);
  gfx().setTextSize(textScale);
}

void ILI9341DisplayDriver::setTextScale(uint8_t size)
{
  textScale = size ? size : 1;
  gfx().setTextSize(textScale);
}

void ILI9341DisplayDriver::drawText(const char *text, int32_t x, int32_t y, uint16_t fg, int32_t bg)
{
  if (bg == kThemeColor)
    gfx().setTextColor(fg);
  else
    gfx().setTextColor(fg, static_cast<uint16_t>(bg));
  gfx().setCursor(x, y);
  gfx().print(text);
}

// Built-in 5x7 font in a 6x8 cell.
int16_t ILI9341DisplayDriver::textWidth(const char *text)
{
  return static_cast<int16_t>(strlen(text) * 6 * textScale);
}

int16_t ILI9341DisplayDriver::fontHeight()
{
  return static_cast<int16_t>(8 * textScale);
}

#endif
