# dcr_display

Screens, menus, popups and widgets for ESP32, on any panel and any input. The same
app code runs on a 128x64 mono OLED with a rotary encoder and on a 480x320 colour
TFT with touch.

## Install

```ini
lib_deps =
  https://github.com/DCRalph/dcr_display.git
  https://github.com/DCRalph/dcr_logger.git
  olikraus/U8g2            ; or bodmer/TFT_eSPI, or adafruit/Adafruit ILI9341
```

Panel libraries are not dependencies. Each driver is guarded by `__has_include`,
so only the one you add is compiled. Keep PlatformIO's default LDF mode: `deep+`
evaluates those guards before your panel library is on the include path.

## Drivers

| Driver | Backend | Notes |
| --- | --- | --- |
| `TFTDisplayDriver` | `bodmer/TFT_eSPI` | Full-frame sprite; draws direct if the sprite will not fit |
| `ILI9341DisplayDriver` | `adafruit/Adafruit ILI9341` | Optional PSRAM frame buffer; unbuffered redraws only on change |
| `U8g2DisplayDriver` | `olikraus/U8g2` | Any mono U8g2 panel, full-buffer (`_F_`) classes only |

A new panel is a `DisplayDriver` subclass. Implement `begin`, `width`, `height`,
`drawPixel`, `fillRect`, `setFont`, `drawText`, `textWidth` and `fontHeight`;
lines, rounded rects and arcs have defaults built from those. Override them when
the backend has faster primitives.

To keep TFT_eSPI's sprite but push to a panel it cannot drive, subclass
`TFTDisplayDriver` and override `initPanel` and `pushFrame`. oled 3's RM67162
AMOLED driver in esp32-remote is a working example.

## Quick start

```cpp
#include <U8g2DisplayDriver.h>
#include <dcr_display.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C panel(U8G2_R0, U8X8_PIN_NONE);
U8g2DisplayDriver driver(panel);

Menu menu;
MenuItemNumber<int> volume("Volume", &volumeValue, 0, 10);

void homeDraw() { menu.draw(); }
void homeUpdate() { menu.update(); }
const Screen2 HomeScreen = {.name = "Home", .draw = homeDraw, .update = homeUpdate};

void setup() {
  menu.addMenuItem(&volume);
  display.begin(driver);          // also seeds theme, profile and menu metrics
  Inputs::addSource(encoderInput);
  screenManager.setScreen(&HomeScreen);
}

void loop() { display.render(); }
```

See `examples/MonoOled` (U8g2 and an encoder), `examples/ColourTft` (TFT_eSPI,
buttons and a joystick) and `examples/TouchIli9341` (touch and the keyboard).

## Input

Sources turn hardware into `InputEvent`s (`Up/Down/Left/Right`, `Select`, `Back`,
`Delta`, `Tap` and pointer events). Menus, popups and widgets only see events, so
any mix of sources works with no changes to screen code.

| Source | Takes | Emits |
| --- | --- | --- |
| `EncoderInput` | detents since the last read, push-button clicks | `Delta` (with acceleration), `Select`, `Back` |
| `ButtonInput` | click counts for up/down/left/right/select/back | directions, `Select`, `Back` |
| `JoystickInput` | raw X/Y ADC, press clicks | directions with auto-repeat, `Select` |
| `TouchInput` | a read callback filling panel x/y | `Tap`, pointer down/move/up |

Click counts follow ClickButton: 1 single, 2 double, negative for a long press.
`defaultSelectClicks` sets which count reads as a plain select. With
`multiClickSelects`, a double click on up/down selects, for two-button devices.
Menu items bind actions to click counts with `addFunc(clicks, fn)`.

Sources are polled lazily, on the first query in a frame. A screen that reads
its buttons or encoder directly still works while no menu is asking.

In a menu, `Delta` and up/down move the selection, select or right activates,
and left or back goes back. While a number or select item is being edited,
navigation changes its value instead. With a pointer source attached, rows grow
to `minTouchRowHeight`, a tap activates a row, a drag scrolls, and rows being
edited show `-`/`+` targets.

## Several displays

Each `Display` has its own `ScreenManager`, menu metrics and input sources.
Menus, popups and widgets draw on, and navigate, whichever display is
rendering them (`Display::active()`), so the same screen code runs on either.

```cpp
Display touchDisplay;
ScreenManager touchScreens;

display.begin(lcdDriver);                         // global display + screenManager
display.addInput(buttons);
touchDisplay.begin(ili9341Driver, &touchScreens);
touchDisplay.addInput(touch);

void loop() { display.render(); touchDisplay.render(); }  // one task
```

`ILI9341DisplayDriver` takes `buffered = true` to draw into a PSRAM frame and
push it in one burst. The esp32-remote `Dual Screen` project is a full example.

## Rendering

`display.render()` runs one frame: input, the screen's `update`, and a redraw
when one is due. Input, popups, screen changes and `display.invalidate()` always
redraw. Otherwise `Screen2::redrawMs` sets the interval: `-1` uses
`display.setIdleRedrawMs` (0 on buffered panels, 1000 on unbuffered), and `0`
redraws every frame.

## Themes, roles and profiles

- `Theme` holds the chrome colours. `Theme::forColorDepth` picks dark or mono;
  replace it with `display.setTheme`. `Screen2::topBarColor` and item colours
  default to the theme (`kThemeColor`).
- Text uses roles (`Small`, `Body`, `Title`, `XLarge`) so discrete-font backends
  render sensibly. `setTextSize` still works, and drivers can override
  `setTextScale` to map sizes to their own fonts.
- `DisplayProfile` and `MenuStyle::Metrics` set chrome geometry from the panel
  size. Override them after `begin`, or replace the top bar with
  `setTopBarRenderer`.

## Popups and widgets

- `screenManager.showPopup(new Popup(...))` takes ownership and is safe to call
  from any task. Also available: `AutoClosePopup`, and `ChoicePopup` for a row
  of choices.
- `Button` plus `FocusGroup` give focus and activation from any source: grid
  navigation, select, and taps.
- `Keyboard` is an on-screen keyboard covering all printable ASCII, usable by
  touch, joystick, buttons or encoder.

## UI test screens

`UiTest::RootScreen` reaches screens that exercise every feature on the app's
own panel and inputs: menu items (numbers, floats, selects, click-count actions,
hiding, submenus), a live input event log, popups (including one shown from
another task), a button grid, the keyboard, drawing primitives and datums,
redraw policy, and a background task with a watchdog overrun. Add one entry:

```cpp
MenuItemNavigate uiTestItem("UI test", &UiTest::RootScreen);
```

## Screens

`Screen2` is a plain descriptor. A screen can also declare `backgroundTask`: a
worker the manager runs while the screen is current, with a watchdog that stops
it if a step overruns. This supersedes `dcr_screen_manager`.

## Migrating from 0.x

- `display.begin(driver)` replaces `begin(DisplayConfig, &screenManager)`. It
  also calls `screenManager.init`.
- `MenuInput` is gone. Register sources with `Inputs::addSource`.
- `getBatteryPercentage()` is gone. Use `display.setBatteryProvider`.
- Library code no longer defines `TFT_*` colours or `*_DATUM`. Use `Color::` and
  `Datum::`, which share TFT_eSPI's values.
- `MenuItem::draw` takes `int16_t` coordinates. Override `valueText` instead to
  show a value.
- Encoder `Delta > 0` now increases numbers, matching row navigation.

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).
