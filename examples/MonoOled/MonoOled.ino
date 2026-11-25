// dcr_display on a 128x64 mono OLED (U8g2) driven by a rotary encoder.
// lib_deps: olikraus/U8g2, madhephaestus/ESP32Encoder

#include <ESP32Encoder.h>
#include <U8g2lib.h>

#include <U8g2DisplayDriver.h>
#include <dcr_display.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C panel(U8G2_R0, U8X8_PIN_NONE);
U8g2DisplayDriver driver(panel);

ESP32Encoder encoder;
constexpr int kEncoderButton = 1;

bool loudEnabled = false;
int brightness = 50;

Menu mainMenu(MenuSize::Medium);
MenuItemToggle loudItem("Loud", &loudEnabled);
MenuItemNumber<int> brightnessItem("Bright", &brightness, 0, 100, 5);
MenuItemAction helloItem("Hello", 1, []()
                         { screenManager.showPopup(new AutoClosePopup("Hi", "Popup fits the panel", 1500)); });

EncoderInput encoderInput({
    .readDelta = []() -> int32_t
    {
      const int32_t steps = encoder.getCount() / 4;
      if (steps != 0)
        encoder.setCount(encoder.getCount() % 4);
      return steps;
    },
    .readClicks = []()
    {
      static bool was = false;
      const bool now = digitalRead(kEncoderButton) == LOW;
      const bool pressed = now && !was;
      was = now;
      return pressed ? 1 : 0;
    },
});

void homeDraw() { mainMenu.draw(); }
void homeUpdate() { mainMenu.update(); }

const Screen2 HomeScreen = {
    .name = "Home",
    .draw = homeDraw,
    .update = homeUpdate,
};

void setup()
{
  pinMode(kEncoderButton, INPUT_PULLUP);
  encoder.attachFullQuad(17, 18);

  mainMenu.addMenuItem(&loudItem);
  mainMenu.addMenuItem(&brightnessItem);
  mainMenu.addMenuItem(&helloItem);

  display.begin(driver);
  display.setBatteryProvider([]()
                             { return 100; });
  Input::addSource(encoderInput);
  screenManager.setScreen(&HomeScreen);
}

void loop()
{
  display.render();
}
