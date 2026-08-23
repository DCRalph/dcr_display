// dcr_display on a 128x64 monochrome OLED via U8g2.
//
// Needs olikraus/U8g2 in lib_deps. Use a full-buffer (_F_) panel class.

#include <U8g2lib.h>

#include <Display.h>
#include <Menu.h>
#include <MenuInput.h>
#include <ScreenManager.h>
#include <U8g2DisplayDriver.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C panel(U8G2_R0, U8X8_PIN_NONE);
U8g2DisplayDriverContext displayCtx(panel);

// The library asks the application for this.
int getBatteryPercentage() { return 100; }

bool loudEnabled = false;
int brightness = 50;

Menu mainMenu(MenuSize::Medium);
MenuItemToggle loudItem("Loud", &loudEnabled);
MenuItemNumber<int> brightnessItem("Bright", &brightness, 0, 100, 5);
MenuItemAction helloItem("Hello", 1, []()
                         { screenManager.showPopup(new AutoClosePopup("Hi", "Popup fits the panel", 1500)); });

void homeDraw() { mainMenu.draw(); }
void homeUpdate() { mainMenu.update(); }

const Screen2 HomeScreen = {
    .name = "Home",
    .draw = homeDraw,
    .update = homeUpdate,
};

void setup()
{
  mainMenu.addMenuItem(&loudItem);
  mainMenu.addMenuItem(&brightnessItem);
  mainMenu.addMenuItem(&helloItem);

  // Size and geometry are taken from the panel; no hand-tuning needed.
  display.begin(U8g2DisplayDriver::makeConfig(&displayCtx), &screenManager);
  screenManager.init(display);

  MenuInputConfig input;
  input.mode = MenuInputMode::Buttons2;
  input.getUpClicks = []() { return 0; };
  input.getDownClicks = []() { return 0; };
  MenuInput::configure(input);

  screenManager.setScreen(&HomeScreen);
}

void loop()
{
  display.render();
}
