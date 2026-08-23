#include "U8g2DisplayDriver.h"

#if __has_include(<U8g2lib.h>)

#include <esp_log.h>

static const char *TAG = "U8g2Display";

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
  case TextRole::Body:
  default:
    return body;
  }
}

namespace
{
  U8g2DisplayDriverContext *ctx(void *context)
  {
    return static_cast<U8g2DisplayDriverContext *>(context);
  }

  /** 1-bit panel: anything that is not pure black lights the pixel. */
  uint8_t inkFor(uint16_t color)
  {
    return color == TFT_BLACK ? 0 : 1;
  }

  /** u8g2's rounded primitives require a radius that fits inside the box. */
  int32_t clampRadius(int32_t r, int32_t w, int32_t h)
  {
    const int32_t limit = (w < h ? w : h) / 2 - 1;
    if (r > limit)
      r = limit;
    return r < 0 ? 0 : r;
  }

  void opInit(void *context, const DisplayConfig &config)
  {
    auto *c = ctx(context);
    c->u8g2.begin();
    c->u8g2.setFont(c->fonts.forRole(TextRole::Body));
    c->u8g2.setFontPosTop();
    c->u8g2.setDrawColor(1);
  }

  void opClear(void *context, uint16_t color)
  {
    auto *c = ctx(context);
    c->u8g2.clearBuffer();
    if (inkFor(color))
    {
      c->u8g2.setDrawColor(1);
      c->u8g2.drawBox(0, 0, c->u8g2.getDisplayWidth(), c->u8g2.getDisplayHeight());
    }
  }

  void opPush(void *context)
  {
    ctx(context)->u8g2.sendBuffer();
  }

  // A discrete-font backend cannot honour an arbitrary integer scale, so the
  // sizes the facade falls back to are mapped onto the nearest role.
  void opSetTextSize(void *context, uint8_t size)
  {
    auto *c = ctx(context);
    TextRole role = TextRole::Body;
    if (size <= 1)
      role = TextRole::Small;
    else if (size == 2)
      role = TextRole::Body;
    else if (size == 3)
      role = TextRole::Title;
    else
      role = TextRole::XLarge;

    c->u8g2.setFont(c->fonts.forRole(role));
  }

  void opSetTextRole(void *context, TextRole role)
  {
    auto *c = ctx(context);
    c->u8g2.setFont(c->fonts.forRole(role));
  }

  void opSetTextDatum(void *context, uint8_t datum)
  {
    ctx(context)->datum = datum;
  }

  void opSetTextColor(void *context, uint16_t color)
  {
    ctx(context)->textOn = inkFor(color) != 0;
  }

  // The background colour is ignored: on a 1-bit panel the caller has already
  // filled the area the text sits on.
  void opSetTextColorBg(void *context, uint16_t fg, uint16_t bg)
  {
    ctx(context)->textOn = inkFor(fg) != 0;
  }

  void opDrawString(void *context, const String &text, int32_t x, int32_t y)
  {
    auto *c = ctx(context);
    const char *s = text.c_str();
    const int16_t w = c->u8g2.getStrWidth(s);

    switch (c->datum)
    {
    case TC_DATUM:
      x -= w / 2;
      c->u8g2.setFontPosTop();
      break;
    case TR_DATUM:
      x -= w;
      c->u8g2.setFontPosTop();
      break;
    case ML_DATUM:
      c->u8g2.setFontPosCenter();
      break;
    case MR_DATUM:
      x -= w;
      c->u8g2.setFontPosCenter();
      break;
    case TL_DATUM:
    default:
      c->u8g2.setFontPosTop();
      break;
    }

    c->u8g2.setDrawColor(c->textOn ? 1 : 0);
    c->u8g2.drawStr(x, y, s);
    c->u8g2.setDrawColor(1);
  }

  void opDrawLine(void *context, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color)
  {
    auto *c = ctx(context);
    c->u8g2.setDrawColor(inkFor(color));
    c->u8g2.drawLine(x0, y0, x1, y1);
    c->u8g2.setDrawColor(1);
  }

  void opDrawRect(void *context, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
  {
    auto *c = ctx(context);
    c->u8g2.setDrawColor(inkFor(color));
    c->u8g2.drawFrame(x, y, w, h);
    c->u8g2.setDrawColor(1);
  }

  void opFillRect(void *context, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
  {
    auto *c = ctx(context);
    c->u8g2.setDrawColor(inkFor(color));
    c->u8g2.drawBox(x, y, w, h);
    c->u8g2.setDrawColor(1);
  }

  void opDrawRoundRect(void *context, int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color)
  {
    auto *c = ctx(context);
    r = clampRadius(r, w, h);
    c->u8g2.setDrawColor(inkFor(color));
    if (r > 0)
      c->u8g2.drawRFrame(x, y, w, h, r);
    else
      c->u8g2.drawFrame(x, y, w, h);
    c->u8g2.setDrawColor(1);
  }

  void opFillRoundRect(void *context, int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color)
  {
    auto *c = ctx(context);
    r = clampRadius(r, w, h);
    c->u8g2.setDrawColor(inkFor(color));
    if (r > 0)
      c->u8g2.drawRBox(x, y, w, h, r);
    else
      c->u8g2.drawBox(x, y, w, h);
    c->u8g2.setDrawColor(1);
  }

  int16_t opTextWidth(void *context, const String &text)
  {
    return ctx(context)->u8g2.getStrWidth(text.c_str());
  }

  int16_t opFontHeight(void *context)
  {
    auto *c = ctx(context);
    return c->u8g2.getAscent() - c->u8g2.getDescent();
  }

  uint16_t opColor565(void *context, uint8_t r, uint8_t g, uint8_t b)
  {
    // Rec. 601 luma, so mid-tones land on the side a viewer would expect.
    const uint16_t luma = (r * 299 + g * 587 + b * 114) / 1000;
    return luma >= 128 ? TFT_WHITE : TFT_BLACK;
  }
} // namespace

DisplayDriverOps U8g2DisplayDriver::makeOps()
{
  DisplayDriverOps ops;
  ops.init = opInit;
  ops.clear = opClear;
  ops.fillScreen = opClear;
  ops.push = opPush;

  ops.setTextSize = opSetTextSize;
  ops.setTextRole = opSetTextRole;
  ops.setTextDatum = opSetTextDatum;
  ops.setTextColor = opSetTextColor;
  ops.setTextColorBg = opSetTextColorBg;

  ops.drawString = opDrawString;
  ops.drawLine = opDrawLine;
  ops.drawRect = opDrawRect;
  ops.fillRect = opFillRect;
  ops.drawRoundRect = opDrawRoundRect;
  ops.fillRoundRect = opFillRoundRect;
  ops.drawSmoothArc = nullptr; // no arc primitive on a 1-bit panel

  ops.textWidth = opTextWidth;
  ops.fontHeight = opFontHeight;
  ops.color565 = opColor565;
  return ops;
}

DisplayConfig U8g2DisplayDriver::makeConfig(U8g2DisplayDriverContext *context,
                                            uint16_t width,
                                            uint16_t height,
                                            uint8_t rotation)
{
  U8G2 &panel = context->u8g2;

  DisplayConfig config;
  config.width = width != 0 ? width : panel.getDisplayWidth();
  config.height = height != 0 ? height : panel.getDisplayHeight();
  config.rotation = rotation;
  config.colorDepth = 1;
  config.context = context;
  config.ops = makeOps();

  // A paged device only holds a slice of the panel, and the facade draws a whole
  // frame before pushing. Catch it here rather than shipping a display that
  // renders one band.
  const uint16_t bufferedRows = panel.getBufferTileHeight() * 8;
  if (bufferedRows < config.height)
  {
    ESP_LOGE(TAG, "Paged U8g2 device (%u of %u rows buffered): use a full-buffer (_F_) variant",
             bufferedRows, config.height);
  }

  return config;
}

#endif // __has_include(<U8g2lib.h>)
