// dcr_display on a colour TFT via TFT_eSPI.
//
// Needs bodmer/TFT_eSPI in lib_deps, configured for your panel.

#include <TFT_eSPI.h>

#include <Display.h>
#include <Menu.h>
#include <MenuInput.h>
#include <ScreenManager.h>
#include <TFTDisplayDriver.h>

TFTDisplayDriverContext displayCtx;

int getBatteryPercentage() { return 100; }

bool loudEnabled = false;
int brightness = 50;

Menu mainMenu(MenuSize::Large);
MenuItemToggle loudItem("Loud", &loudEnabled);
MenuItemNumber<int> brightnessItem("Bright", &brightness, 0, 100, 5);
MenuItemAction helloItem("Hello", 1, []()
                         { screenManager.showPopup(new AutoClosePopup("Hi", "Same code, bigger panel", 1500)); });

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

  display.begin(TFTDisplayDriver::makeConfig(&displayCtx, 320, 240, 1), &screenManager);
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
