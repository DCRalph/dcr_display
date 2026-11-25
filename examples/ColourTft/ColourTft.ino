// dcr_display on a colour TFT (TFT_eSPI), with three buttons and a joystick.
// Every source feeds the same menu; register only the ones your board has.
// lib_deps: bodmer/TFT_eSPI (configured for your panel)

#include <TFT_eSPI.h>

#include <TFTDisplayDriver.h>
#include <dcr_display.h>

TFTDisplayDriver driver(320, 240, 1);

constexpr int kUp = 0, kDown = 14, kSelect = 35, kJoyX = 4, kJoyY = 5;

bool loudEnabled = false;
int brightness = 50;
float gain = 1.5f;

Menu mainMenu(MenuSize::Large);
MenuItemToggle loudItem("Loud", &loudEnabled);
MenuItemNumber<int> brightnessItem("Bright", &brightness, 0, 100, 5);
MenuItemNumber<float> gainItem("Gain", &gain, 0.0f, 4.0f, 0.1f);
MenuItemAction askItem("Reset", 1, []()
                       { screenManager.showPopup(new ChoicePopup("Reset?", "Settings go back to defaults.", {"No", "Yes"},
                                                                 [](int choice)
                                                                 { if (choice == 1) brightness = 50; })); });

// Edge-detected press; a real app would use a debounced button library.
int pressed(int pin)
{
  static bool last[48] = {};
  const bool now = digitalRead(pin) == LOW;
  const bool edge = now && !last[pin];
  last[pin] = now;
  return edge ? 1 : 0;
}

ButtonInput buttons({
    .up = []() { return pressed(kUp); },
    .down = []() { return pressed(kDown); },
    .select = []() { return pressed(kSelect); },
});

JoystickInput joystick({
    .readX = []() { return analogRead(kJoyX); },
    .readY = []() { return analogRead(kJoyY); },
});

void homeDraw() { mainMenu.draw(); }
void homeUpdate() { mainMenu.update(); }

const Screen2 HomeScreen = {
    .name = "Home",
    .draw = homeDraw,
    .update = homeUpdate,
    .topBarColor = Color::Cyan,
};

void setup()
{
  pinMode(kUp, INPUT_PULLUP);
  pinMode(kDown, INPUT_PULLUP);
  pinMode(kSelect, INPUT_PULLUP);

  gainItem.setDecimals(1);
  mainMenu.addMenuItem(&loudItem);
  mainMenu.addMenuItem(&brightnessItem);
  mainMenu.addMenuItem(&gainItem);
  mainMenu.addMenuItem(&askItem);

  display.begin(driver);
  Input::addSource(buttons);
  Input::addSource(joystick);
  screenManager.setScreen(&HomeScreen);
}

void loop()
{
  display.render();
}
