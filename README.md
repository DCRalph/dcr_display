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
| `TouchInput` | a read callback filling panel x/y | `Tap`, `DoubleTap`, `LongPress`, `Swipe`, pointer down/move/up |

Click counts follow ClickButton: 1 single, 2 double, negative for a long press.
`defaultSelectClicks` sets which count reads as a plain select. With
`multiClickSelects`, a double click on up/down selects, for two-button devices.
Menu items bind actions to click counts with `addFunc(clicks, fn)`.

Gestures come from `TouchInput` (thresholds in its `Config`): `Swipe` with a
`SwipeDir`, `LongPress`, `DoubleTap`, and `PointerUp` carries the release speed
for flings. Menus go back on a right swipe, run the long-click (`-1`) action on
a long press, and fling on a quick flick; popups dismiss on a swipe.

Resistive panels need filtering. `TouchInput::Config` turns each part on
separately, and the defaults filter nothing:

| Option | Effect | XPT2046 |
| --- | --- | --- |
| `releasePolls` | empty polls before a press counts as lifted; rides out false lifts | 5 |
| `startPressure` / `holdPressure` | pressure (via `readSample`) to start / keep a press | 600 / 300 |
| `agreePx` | two samples per poll must agree; noise mid-press keeps the last position | 20 |
| `width` / `height` | readings off the panel are noise | panel size |

`touch.sample()` between frames catches taps shorter than a frame, and
`touch.suppressUntilRelease()` swallows the press in progress, e.g. after it
changed screens.

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

## Performance

- **Only what changed is sent.** Buffered TFT_eSPI and ILI9341 drivers hash
  the frame in 40 px cells (`FrameDiff`) and push changed bands. U8g2 compares
  each 8-pixel page with what the panel shows and sends changed pages only: a
  full 128x64 frame is about 25 ms of I2C at 400 kHz, a static one is nothing.
  `setPartialPush(false)` on any of them turns this off.
- **Fast fills.** Sprite and canvas fills (`fillRect`, `fillScreen`, and the
  rounded rects built from them) write 32-bit words straight into the frame
  (`FastFill`), not one pixel at a time.
- **No per-frame allocation.** Menus reuse one row list and word wrapping uses
  a stack buffer, so a steady frame does not touch the heap.
- **Fonts are switched only when the role changes**, since TFT_eSPI rescans the
  whole font on every `setFreeFont`.
- **Pre-clear skipping.** A screen whose `draw()` starts with
  `display.clearScreen()` skips the library's own clear from its second frame.
- **Pace your loop.** Render from a loop that sleeps out the rest of each frame
  (`vTaskDelay` of the time left), not one that spins or uses `xTaskDelayUntil`,
  which runs catch-up frames back to back after a slow one.

`display.stats()` reports fps, draw and push time, the share of each frame
pushed, and load for the last second.

## Rendering

`display.render()` runs one frame: input, the screen's `update`, and a redraw
when one is due. Input, popups, screen changes and `display.invalidate()` always
redraw. Otherwise `Screen2::redrawMs` sets the interval: `-1` uses
`display.setIdleRedrawMs` (0 on buffered panels, 1000 on unbuffered), and `0`
redraws every frame.

## Look and themes

Grouped cards on a soft background with one accent colour: menu rows share a
card split by hairlines, the selected row is a rounded highlight, toggles are
switches, and controls can sit raised on a solid shadow. Colour panels use
FreeSans for the Body, Title and XLarge roles (TFT_eSPI with `LOAD_GFXFF`, or
Adafruit GFX); Small and `setTextSize` keep the built-in 5x7 font.

Built-in themes, set with `display.setTheme(Theme::bactosure())`:

| Theme | Look |
| --- | --- |
| `dark()` | default on colour panels; flat, blue accent |
| `light()` | flat, white cards |
| `bactosure()` | the Bactosure bioreactor: dark-blue bar, pastel raised controls, dark-blue labels |
| `midnight()` | the bioreactor's engineering screens: black, slate raised controls |
| `contrast()` | black and white with a yellow focus, for sunlight |
| `mono()` | default on 1-bit panels; inversion instead of fills |

A theme is a value: copy a preset and change fields. Besides colours it sets:

- `depth`: px raised controls (buttons, keys, cards, popups, -/+ keys) sit above a
  shadow of their own fill at `shadowPercent` brightness. 0 is flat; ignored on 1-bit.
- `fillTopBar`: fill the top bar with `topBar` on any panel size.
- `switchOn` / `switchKnob`: switch colours; `track` is the off switch, slider
  and range track, kept at 3:1 or more on `surface`.
- Defaults for how menus and editors render, below.

### Menus and editors

Each setting has a theme default and a per-element override; `Theme` (the
enum's first value) means "use the theme's".

| Element | Options | Override |
| --- | --- | --- |
| Menu layout | `MenuLook::Grouped` (rows in one raised card), `Flat` (rows on the background) | `menu.setLook(...)` |
| Number editor | `NumberEdit::Expand` (the row grows: large value between big -/+ keys over a min-max bar), `Inline` (a raised - value + pill at the row's end) | `item.setEditStyle(...)` |
| Select editor | `SelectEdit::Expand` (the row grows into a segmented control), `Sheet` (a sheet of every option rises from the bottom), `Inline` (a raised < value > pill) | `item.setEditStyle(...)` |

Colour themes default to Grouped, Expand and Expand; `mono()` keeps Flat and
Inline, which suit 64 px panels. An Expand select whose options will not fit
side by side edits inline instead. Every editor takes the encoder, buttons and
touch: navigation steps the value (Up moves up a sheet's list), select or back
finishes, and a tap on the editor's keys, segments or options acts on them while
a tap anywhere else finishes editing. A sheet also closes when its handle is
dragged down or the sheet is swiped down. A sheet with more options than fit
scrolls: drag or flick its options, and a bar at its edge shows where you are.

Popups and sheets dim what is behind them on colour panels whose driver
implements `dimRect` (TFT_eSPI and ILI9341 with a frame buffer).

- Text roles: `Small`, `Body`, `Title`, `XLarge`. Drivers choose the face per
  role: `setRoleFont` on ILI9341 and on TFT_eSPI with `LOAD_GFXFF`,
  `setRoleScale` on TFT_eSPI without it, `fonts` on U8g2. `setTextScale` keeps
  legacy integer sizes working.
- `DisplayProfile` and `MenuStyle::Metrics` set chrome geometry from the panel
  size. Override them after `begin`, or replace the top bar with
  `setTopBarRenderer`. A screen's `topBarColor` still fills its bar.
- `Draw::` has the shared pieces (raised surfaces, switch, chevron, -/+ signs,
  radio marks, battery, focus ring) for screens that want to match.

## Animation

`Tween` moves a value over time with an easing curve (`Ease::linear`,
`outQuad`, `outCubic`, `inOutCubic`, `outBack`):

```cpp
Tween x;
x.start(0, 100, 250, Ease::outCubic);   // from a draw() or update()
display.fillRect(x.value(), 40, 10, 10, Color::White);
```

Starting one keeps its display redrawing until it ends, so nothing else has to
change. The library uses it for the menu highlight sliding between rows, smooth
list scrolling and flings, switch knobs, and popups sliding in.
`display.setAnimations(false)` turns motion off; it is off automatically on
unbuffered panels, where every frame would flicker.

## Popups and widgets

- `screenManager.showPopup(new Popup(...))` takes ownership and is safe from any
  task. Also `AutoClosePopup`, and `ChoicePopup` for a row of choices (the last
  is the primary one). Popups close on select, back, a tap or a swipe.
- Widgets: `Button` (Primary, Secondary or Ghost style), `Switch` and `Slider`,
  in a `FocusGroup` that drives them from any source: grid navigation, select,
  taps, drags, and an edit mode for sliders on button and encoder devices.
  Focus only shows on displays with a non-pointer source.
- `Keyboard`: an on-screen keyboard covering all printable ASCII.

## UI test screens

`UiTest::RootScreen` reaches screens that exercise every feature on the app's
own panel and inputs: a picker for every built-in theme, a motion switch, menu
items with each editor style side by side and a per-menu look switch, components
(buttons, switches, a slider), gestures, easing curves, popups (including one
shown from another task), the keyboard, a live input event log, drawing
primitives and datums, redraw policy, and a background task with a watchdog
overrun. Add one entry:

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
