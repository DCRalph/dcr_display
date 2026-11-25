#include "Display.h"

#include "Input.h"
#include "Menu.h"
#include "ScreenManager.h"

#undef LOG_TAG
#define LOG_TAG "DISPLAY"
#include <dcr_Logger.h>

namespace
{
  Display *gActive = &display;

  // Makes a display active for one scope, restoring the previous on exit.
  struct ActiveScope
  {
    Display *previous;
    explicit ActiveScope(Display *d) : previous(gActive) { gActive = d; }
    ~ActiveScope() { gActive = previous; }
  };
}

Display &Display::active()
{
  return *gActive;
}

ScreenManager &Display::screens() const
{
  return *screenManager;
}

void Display::addInput(InputSource &source)
{
  for (InputSource *s : inputSet.sources)
    if (s == &source)
      return;
  inputSet.sources.push_back(&source);
}

void Display::removeInput(InputSource &source)
{
  for (auto it = inputSet.sources.begin(); it != inputSet.sources.end(); ++it)
  {
    if (*it == &source)
    {
      inputSet.sources.erase(it);
      return;
    }
  }
}

void Display::setMenuMetrics(const MenuStyle::Metrics &m)
{
  metrics = m;
  dirty = true;
}

bool Display::begin(DisplayDriver &driver, ScreenManager *manager)
{
  ActiveScope scope(this);
  panel = &driver;
  screenManager = manager != nullptr ? manager : &::screenManager;

  ready = panel->begin();
  if (!ready)
  {
    debugE("Display driver failed to start");
    return false;
  }

  const uint16_t w = panel->width(), h = panel->height();
  const uint8_t depth = panel->colorDepth();
  displayProfile = DisplayProfile::forPanel(w, h, depth);
  currentTheme = Theme::forColorDepth(depth);
  metrics = MenuStyle::Metrics::forPanel(w, h, depth);
  idleRedrawMs = panel->buffered() ? 0 : 1000;
  textFg = currentTheme.text;
  panel->setFont(TextRole::Body);

  screenManager->init(*this);

  clearScreen();
  push();
  debugI("%ux%u, %u-bit, %s", w, h, depth, panel->buffered() ? "buffered" : "direct");
  return true;
}

void Display::setScreenManager(ScreenManager *manager)
{
  screenManager = manager;
}

uint16_t Display::width() const
{
  return panel ? panel->width() : 0;
}

uint16_t Display::height() const
{
  return panel ? panel->height() : 0;
}

uint8_t Display::colorDepth() const
{
  return panel ? panel->colorDepth() : 16;
}

void Display::setTheme(const Theme &theme)
{
  currentTheme = theme;
  dirty = true;
}

void Display::setProfile(const DisplayProfile &profile)
{
  displayProfile = profile;
  dirty = true;
}

void Display::clearScreen(int32_t color)
{
  if (!panel)
    return;
  if (drawingScreen)
    sawScreenFill = true;
  panel->fillScreen(color == kThemeColor ? currentTheme.background : static_cast<uint16_t>(color));
}

void Display::fillScreen(int32_t color)
{
  clearScreen(color);
}

void Display::push()
{
  if (panel)
    panel->push();
}

void Display::setTextSize(uint8_t size)
{
  if (panel)
    panel->setTextScale(size);
}

void Display::setTextRole(TextRole role)
{
  if (panel)
    panel->setFont(role);
}

void Display::setTextDatum(uint8_t d)
{
  datum = d;
}

void Display::setTextColor(uint16_t color)
{
  textFg = color;
  textBg = kThemeColor;
}

void Display::setTextColor(uint16_t fg, uint16_t bg)
{
  textFg = fg;
  textBg = bg;
}

void Display::drawString(const String &text, int32_t x, int32_t y)
{
  drawString(text.c_str(), x, y);
}

void Display::drawString(const char *text, int32_t x, int32_t y)
{
  if (!panel || text == nullptr || *text == '\0')
    return;

  // Datums 0-8 form a 3x3 grid; 9-11 are TFT_eSPI baselines, drawn as bottom.
  const uint8_t col = datum <= 8 ? datum % 3 : datum - 9;
  const uint8_t row = datum <= 8 ? datum / 3 : 2;
  if (col > 0)
    x -= panel->textWidth(text) * col / 2;
  if (row > 0)
    y -= panel->fontHeight() * row / 2;

  panel->drawText(text, x, y, textFg, textBg);
}

void Display::drawPixel(int32_t x, int32_t y, uint16_t color)
{
  if (panel)
    panel->drawPixel(x, y, color);
}

void Display::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color)
{
  if (panel)
    panel->drawLine(x0, y0, x1, y1, color);
}

void Display::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
{
  if (panel)
    panel->drawRect(x, y, w, h, color);
}

void Display::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color)
{
  if (panel)
    panel->fillRect(x, y, w, h, color);
}

void Display::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color)
{
  if (panel)
    panel->drawRoundRect(x, y, w, h, r, color);
}

void Display::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t color)
{
  if (panel)
    panel->fillRoundRect(x, y, w, h, r, color);
}

void Display::drawSmoothArc(int32_t x, int32_t y, int32_t r, int32_t ir, int32_t startAngle, int32_t endAngle,
                            uint16_t fgColor, uint16_t bgColor, bool roundEnds)
{
  if (panel)
    panel->drawSmoothArc(x, y, r, ir, startAngle, endAngle, fgColor, bgColor, roundEnds);
}

int16_t Display::textWidth(const String &text)
{
  return textWidth(text.c_str());
}

int16_t Display::textWidth(const char *text)
{
  return panel && text ? panel->textWidth(text) : 0;
}

int16_t Display::fontHeight()
{
  return panel ? panel->fontHeight() : 0;
}

uint16_t Display::color565(uint8_t r, uint8_t g, uint8_t b)
{
  return panel ? panel->color565(r, g, b) : Color::rgb(r, g, b);
}

int Display::wrapTextInBounds(const String &text, int x, int y, int width, int height, TextRole role, int gap,
                              uint16_t color, int textAlignment)
{
  setTextRole(role);
  return drawWrapped(text, x, y, width, height, gap, color, textAlignment);
}

int Display::wrapTextInBounds(const String &text, int x, int y, int width, int height, int textSize, int gap,
                              uint16_t color, int textAlignment)
{
  setTextSize(static_cast<uint8_t>(textSize));
  return drawWrapped(text, x, y, width, height, gap, color, textAlignment);
}

int Display::drawWrapped(const String &text, int x, int y, int width, int height, int gap, uint16_t color,
                         int textAlignment)
{
  setTextColor(color);
  const uint8_t col = textAlignment <= 8 ? textAlignment % 3 : 0;
  setTextDatum(col);

  const int lineHeight = fontHeight() + gap;
  int lines = 0;
  String line;

  auto flush = [&]() -> bool
  {
    if (lineHeight > 0 && (lines + 1) * lineHeight - gap > height && lines > 0)
      return false;
    drawString(line, x, y + lines * lineHeight);
    lines++;
    line = "";
    return true;
  };

  const int len = text.length();
  int i = 0;
  while (i <= len)
  {
    int end = i;
    while (end < len && text[end] != ' ' && text[end] != '\n')
      end++;
    const String word = text.substring(i, end);
    if (word.length() == 0 && end < len && text[end] == ' ')
    {
      i = end + 1;
      continue;
    }
    const String candidate = line.length() ? line + " " + word : word;

    if (line.length() && textWidth(candidate) > width)
    {
      if (!flush())
        return lines;
      line = word;
    }
    else
      line = candidate;

    if (end >= len || text[end] == '\n')
    {
      if (!flush())
        return lines;
    }
    i = end + 1;
  }
  return lines;
}

void Display::noTopBar()
{
  showTopBar = false;
}

void Display::drawTopBar()
{
  if (!screenManager || !screenManager->getCurrentScreen())
    return;

  const Screen2 *screen = screenManager->getCurrentScreen();
  if (topBarRenderer)
  {
    topBarRenderer(*this, screen);
    return;
  }

  const DisplayProfile &p = displayProfile;
  const int16_t barHeight = static_cast<int16_t>(p.topBarHeight);
  uint16_t fg = screen->topBarTextColor != kThemeColor ? screen->topBarTextColor : currentTheme.topBarText;

  if (p.topBarFilled && colorDepth() > 1)
  {
    const uint16_t bg = screen->topBarColor != kThemeColor ? screen->topBarColor : currentTheme.topBar;
    fillRect(0, 0, width(), barHeight, bg);
  }
  else
  {
    fg = currentTheme.text;
    if (p.topBarUnderline)
      drawLine(0, barHeight - 1, width(), barHeight - 1, currentTheme.text);
  }

  setTextRole(p.topBarTextRole);
  setTextColor(fg);
  const int16_t midY = barHeight / 2;

  setTextDatum(Datum::MiddleLeft);
  drawString(screen->name, p.topBarPaddingX, midY);

  if (batteryProvider)
  {
    if (batteryReadMs == 0 || millis() - batteryReadMs >= 1000)
    {
      batteryReadMs = millis();
      snprintf(batteryText, sizeof(batteryText), "%d%%", batteryProvider());
    }
    setTextDatum(Datum::MiddleRight);
    drawString(batteryText, width() - p.topBarPaddingX, midY);
  }
}

bool Display::render()
{
  if (!ready || !screenManager)
    return false;
  ActiveScope scope(this);
  const uint32_t startUs = micros();

  Inputs::beginFrame();

  const uint32_t now = millis();
  const Screen2 *screen = screenManager->getCurrentScreen();
  const int32_t interval = screen && screen->redrawMs >= 0 ? screen->redrawMs : static_cast<int32_t>(idleRedrawMs);
  const bool draw = dirty || screenManager->isPopupActive() || interval == 0 ||
                    now - lastDrawMs >= static_cast<uint32_t>(interval);
  dirty = false;

  uint32_t drawUs = 0, pushUs = 0;
  if (draw)
  {
    // Skip the pre-clear for a screen that filled the frame itself last time; if
    // it stops doing so, clear and draw it again this frame.
    const Screen2 *drawn = screenManager->getCurrentScreen();
    if (drawn != lastDrawnScreen)
      screenFillsFrame = false;
    lastDrawnScreen = drawn;

    const bool preCleared = !screenFillsFrame;
    if (preCleared)
      clearScreen();
    sawScreenFill = false;
    drawingScreen = true;
    screenManager->draw();
    drawingScreen = false;
    if (!preCleared && !sawScreenFill)
    {
      clearScreen();
      screenManager->draw();
    }
    screenFillsFrame = sawScreenFill;

    if (showTopBar)
      drawTopBar();
    drawUs = micros() - startUs;
  }
  showTopBar = true;

  // Input lands after this frame's draw, so state it changed shows on the next.
  const uint32_t updateStartUs = micros();
  screenManager->update();
  if (Inputs::hadEventsThisFrame())
    dirty = true;
  const uint32_t updateUs = micros() - updateStartUs;

  if (draw)
  {
    const uint32_t popupStartUs = micros();
    screenManager->drawPopup();
    const uint32_t pushStartUs = micros();
    drawUs += pushStartUs - popupStartUs;
    push();
    pushUs = micros() - pushStartUs;
    frameAccum.pushedPercent += panel->lastPushPercent();
    lastDrawMs = now;
  }

  accumulateFrame(draw, drawUs, pushUs, updateUs, micros() - startUs);
  return draw;
}

void Display::accumulateFrame(bool drew, uint32_t drawUs, uint32_t pushUs, uint32_t updateUs, uint32_t renderUs)
{
  auto &a = frameAccum;
  const uint32_t now = millis();
  if (a.windowStartMs == 0)
    a.windowStartMs = now;

  a.renders++;
  a.updateUs += updateUs;
  a.renderUs += renderUs;
  if (drew)
  {
    a.frames++;
    a.drawUs += drawUs;
    a.pushUs += pushUs;
    if (drawUs + pushUs > a.maxFrameUs)
      a.maxFrameUs = drawUs + pushUs;
  }

  const uint32_t elapsedMs = now - a.windowStartMs;
  if (elapsedMs < 1000)
    return;

  FrameStats s;
  s.fps = static_cast<uint16_t>(a.frames * 1000UL / elapsedMs);
  s.renders = static_cast<uint16_t>(a.renders * 1000UL / elapsedMs);
  s.avgDrawUs = a.frames ? a.drawUs / a.frames : 0;
  s.avgPushUs = a.frames ? a.pushUs / a.frames : 0;
  s.avgPushedPercent = static_cast<uint8_t>(a.frames ? a.pushedPercent / a.frames : 0);
  s.maxFrameUs = a.maxFrameUs;
  s.avgUpdateUs = a.updateUs / a.renders;
  s.loadPercent = static_cast<uint8_t>(min<uint64_t>(100, a.renderUs / 10 / elapsedMs));
  frameStats = s;
  a = {};
  a.windowStartMs = now;
}

Display display;
