// dcr_display on an SPI ILI9341 with an XPT2046 touch panel: a touch menu and an
// on-screen keyboard. The panel is unbuffered, so it only redraws on change.
// lib_deps: adafruit/Adafruit ILI9341, paulstoffregen/XPT2046_Touchscreen

#include <XPT2046_Touchscreen.h>

#include <ILI9341DisplayDriver.h>
#include <dcr_display.h>

ILI9341DisplayDriver driver(HSPI, {.cs = 15, .dc = 2, .rst = -1, .sclk = 14, .miso = 12, .mosi = 13});
XPT2046_Touchscreen touch(33);

String typed;
Keyboard keyboard;

// Filtering for a resistive XPT2046: ride out false lifts, pressure hysteresis,
// and drop readings off the panel.
TouchInput touchInput([]
                      {
  TouchInput::Config c;
  c.readSample = [](TouchSample &s)
  {
    const TS_Point p = touch.getPoint(); // z is 0 below the library's own threshold
    // Raw 200-3900 to panel pixels; calibrate for your panel.
    s.x = map(p.x, 200, 3900, 0, display.width());
    s.y = map(p.y, 200, 3900, 0, display.height());
    s.z = p.z;
    return p.z > 0;
  };
  c.releasePolls = 5;
  c.startPressure = 600;
  c.holdPressure = 300;
  c.width = 320;
  c.height = 240;
  return c; }());

void keyboardDraw()
{
  display.setTextRole(TextRole::Body);
  display.setTextColor(display.theme().text);
  display.setTextDatum(Datum::TopLeft);
  display.drawString(typed + "_", 8, MenuStyle::contentTopY() + 8);
  keyboard.draw();
}

void keyboardUpdate()
{
  keyboard.update();
}

const Screen2 KeyboardScreen = {
    .name = "Type",
    .draw = keyboardDraw,
    .update = keyboardUpdate,
};

Menu mainMenu(MenuSize::Large);
MenuItemNavigate typeItem("Keyboard", &KeyboardScreen);
int volume = 5;
MenuItemNumber<int> volumeItem("Volume", &volume, 0, 10);

void homeDraw() { mainMenu.draw(); }
void homeUpdate() { mainMenu.update(); }

const Screen2 HomeScreen = {
    .name = "Home",
    .draw = homeDraw,
    .update = homeUpdate,
};

void setup()
{
  mainMenu.addMenuItem(&typeItem);
  mainMenu.addMenuItem(&volumeItem);

  display.begin(driver);
  touch.begin(); // registry XPT2046_Touchscreen uses the default SPI bus
  touch.setRotation(1);
  Inputs::addSource(touchInput);

  keyboard.setBounds({0, static_cast<int16_t>(display.height() / 2), static_cast<int16_t>(display.width()),
                      static_cast<int16_t>(display.height() / 2)});
  keyboard.onKey = [](char c)
  { typed += c; };
  keyboard.onDelete = []()
  { if (typed.length()) typed.remove(typed.length() - 1); };
  keyboard.onDone = []()
  { screenManager.back(); };
  keyboard.onCancel = keyboard.onDone;

  screenManager.setScreen(&HomeScreen);
}

void loop()
{
  display.render();
}
