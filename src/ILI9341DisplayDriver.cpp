// Adafruit_ILI9341 (SPI) adapter implementation.
#include "ILI9341DisplayDriver.h"

namespace
{
  ILI9341DisplayDriverContext *ctx(void *context)
  {
    return static_cast<ILI9341DisplayDriverContext *>(context);
  }

  void init(void *context, const DisplayConfig &config)
  {
    ILI9341DisplayDriverContext *c = ctx(context);

    // Bring up the bus on the wired pins FIRST. ESP32's SPIClass::begin()
    // early-returns once initialised, so this fixes the pin mapping and any
    // later begin() (the driver's own, or a shared touch controller's) no-ops.
    c->spi.begin(c->pinSCLK, c->pinMISO, c->pinMOSI, c->pinCS);
    c->tft.begin();
    c->tft.setSPISpeed(c->spiFrequency);
    c->tft.setRotation(config.rotation);
    c->tft.fillScreen(0x0000);
  }

  void clear(void *context, uint16_t color) { ctx(context)->tft.fillScreen(color); }
  void fillScreen(void *context, uint16_t color) { ctx(context)->tft.fillScreen(color); }

  // Direct-render driver: nothing is buffered, so there is nothing to flush.
  void push(void *) {}

  void setTextSize(void *context, uint8_t size)
  {
    ILI9341DisplayDriverContext *c = ctx(context);
    c->textSize = size ? size : 1;
    c->tft.setTextSize(c->textSize);
  }

  void setTextDatum(void *context, uint8_t datum) { ctx(context)->datum = datum; }

  void setTextColor(void *context, uint16_t color) { ctx(context)->tft.setTextColor(color); }

  void setTextColorBg(void *context, uint16_t fg, uint16_t bg)
  {
    ctx(context)->tft.setTextColor(fg, bg);
  }

  // Emulate TFT_eSPI text datums. Adafruit positions text from the top-left of
  // the cursor, so we measure the string and offset the cursor to align it.
  void drawString(void *context, const String &text, int32_t x, int32_t y)
  {
    ILI9341DisplayDriverContext *c = ctx(context);

    int16_t bx, by;
    uint16_t bw, bh;
    c->tft.getTextBounds(text, 0, 0, &bx, &by, &bw, &bh);

    int32_t cx = x;
    int32_t cy = y;
    const uint8_t d = c->datum;
    if (d <= 8) // TL..BR grid
    {
      const uint8_t col = d % 3; // 0 left, 1 centre, 2 right
      const uint8_t row = d / 3; // 0 top,  1 middle, 2 bottom
      if (col == 1)
        cx = x - bw / 2;
      else if (col == 2)
        cx = x - bw;
      if (row == 1)
        cy = y - bh / 2;
      else if (row == 2)
        cy = y - bh;
    }

    c->tft.setCursor(cx - bx, cy - by);
    c->tft.print(text);
  }

  void drawLine(void *context, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color)
  {
    ctx(context)->tft.drawLine(x0, y0, x1, y1, color);
  }

  void drawRect(void *context, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
  {
    ctx(context)->tft.drawRect(x, y, w, h, color);
  }

  void fillRect(void *context, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
  {
    ctx(context)->tft.fillRect(x, y, w, h, color);
  }

  void drawRoundRect(void *context, int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color)
  {
    ctx(context)->tft.drawRoundRect(x, y, w, h, r, color);
  }

  void fillRoundRect(void *context, int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color)
  {
    ctx(context)->tft.fillRoundRect(x, y, w, h, r, color);
  }

  // Adafruit_GFX has no smooth-arc primitive. Approximate the outer edge with a
  // short polyline so callers that rely on it still get a visible arc.
  void drawSmoothArc(void *context, int32_t x, int32_t y, int32_t r, int32_t ir,
                     int32_t startAngle, int32_t endAngle, uint16_t fgColor,
                     uint16_t /*bgColor*/, bool /*roundEnds*/)
  {
    ILI9341DisplayDriverContext *c = ctx(context);
    if (endAngle < startAngle)
    {
      const int32_t t = startAngle;
      startAngle = endAngle;
      endAngle = t;
    }
    const int32_t rad = ir > 0 ? (r + ir) / 2 : r;
    int32_t prevX = 0, prevY = 0;
    bool havePrev = false;
    for (int32_t a = startAngle; a <= endAngle; a += 4)
    {
      const float rd = a * 0.01745329252f; // deg -> rad
      const int32_t px = x + (int32_t)lroundf(cosf(rd) * rad);
      const int32_t py = y + (int32_t)lroundf(sinf(rd) * rad);
      if (havePrev)
        c->tft.drawLine(prevX, prevY, px, py, fgColor);
      prevX = px;
      prevY = py;
      havePrev = true;
    }
  }

  int16_t textWidth(void *context, const String &text)
  {
    int16_t bx, by;
    uint16_t bw, bh;
    ctx(context)->tft.getTextBounds(text, 0, 0, &bx, &by, &bw, &bh);
    return (int16_t)bw;
  }

  int16_t fontHeight(void *context)
  {
    // Classic Adafruit font cell is 8px tall, scaled by the text size.
    return (int16_t)(8 * ctx(context)->textSize);
  }

  uint16_t color565(void *context, uint8_t r, uint8_t g, uint8_t b)
  {
    return ctx(context)->tft.color565(r, g, b);
  }
} // namespace

DisplayDriverOps ILI9341DisplayDriver::makeOps()
{
  DisplayDriverOps ops;
  ops.init = init;
  ops.clear = clear;
  ops.fillScreen = fillScreen;
  ops.push = push;
  ops.setTextSize = setTextSize;
  ops.setTextDatum = setTextDatum;
  ops.setTextColor = setTextColor;
  ops.setTextColorBg = setTextColorBg;
  ops.drawString = drawString;
  ops.drawLine = drawLine;
  ops.drawRect = drawRect;
  ops.fillRect = fillRect;
  ops.drawRoundRect = drawRoundRect;
  ops.fillRoundRect = fillRoundRect;
  ops.drawSmoothArc = drawSmoothArc;
  ops.textWidth = textWidth;
  ops.fontHeight = fontHeight;
  ops.color565 = color565;
  return ops;
}

DisplayConfig ILI9341DisplayDriver::makeConfig(ILI9341DisplayDriverContext *context,
                                               uint16_t width, uint16_t height,
                                               uint8_t rotation, uint8_t colorDepth)
{
  DisplayConfig config;
  config.width = width;
  config.height = height;
  config.rotation = rotation;
  config.colorDepth = colorDepth;
  config.context = context;
  config.ops = makeOps();
  return config;
}

SPIClass &ILI9341DisplayDriver::spi(ILI9341DisplayDriverContext *context)
{
  return context->spi;
}
