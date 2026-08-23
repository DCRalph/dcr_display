# dcr_display

ESP32 display library: screen management, menus, popups, and a panel-agnostic
drawing facade. Ships backends for colour TFTs and monochrome OLEDs.

## Install

**PlatformIO** — add the library plus the one panel driver your board needs:

```ini
lib_deps =
  https://github.com/DCRalph/dcr_display.git
  olikraus/U8g2            ; or bodmer/TFT_eSPI, or adafruit/Adafruit ILI9341
```

Panel libraries are deliberately not declared as dependencies. Each driver is
guarded by `__has_include`, so a driver whose backend is absent compiles to
nothing and costs you no download.

**Arduino** — clone into `Arduino/libraries/dcr_display`.

## Backends

| Driver | Backend | Panels |
| --- | --- | --- |
| `TFTDisplayDriver` | `bodmer/TFT_eSPI` | Colour TFTs, sprite double-buffered |
| `ILI9341DisplayDriver` | `adafruit/Adafruit ILI9341` | SPI ILI9341, direct to panel |
| `U8g2DisplayDriver` | `olikraus/U8g2` | Any monochrome U8g2 panel |

The U8g2 driver takes a `U8G2 &` — the common base every U8g2 panel class
derives from — so SH1106, SSD1306, SSD1309, ST7565 and the rest all work
without the library knowing which is attached. Two constraints:

- Use a **full-buffer** panel class (the `_F_` variants). Paged variants
  (`_1_`, `_2_`) need a `firstPage`/`nextPage` loop, which does not fit a
  draw-then-push facade. `makeConfig` logs an error if handed one.
- Panels wider or taller than 255px also need `-D U8G2_16BIT`.

## Quick start

Monochrome OLED — size and geometry come from the panel, so there is nothing
to tune:

```cpp
#include <U8g2lib.h>
#include <Display.h>
#include <ScreenManager.h>
#include <U8g2DisplayDriver.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C panel(U8G2_R0, U8X8_PIN_NONE);
U8g2DisplayDriverContext displayCtx(panel);

int getBatteryPercentage() { return 100; }   // the library asks you for this

void setup() {
  display.begin(U8g2DisplayDriver::makeConfig(&displayCtx), &screenManager);
  screenManager.init(display);
}
```

Colour TFT — same code below `begin`:

```cpp
TFTDisplayDriverContext displayCtx;
display.begin(TFTDisplayDriver::makeConfig(&displayCtx, 320, 240, 1), &screenManager);
```

See `examples/MonoOled` and `examples/ColourTft`.

## Text roles

The facade talks in roles, not pixel sizes, because the two backends disagree
about what a "size" is: TFT_eSPI scales one bitmap font by an integer, U8g2
selects a discrete named font.

```cpp
display.setTextRole(TextRole::Title);   // Small | Body | Title | XLarge
```

A TFT driver maps roles to integer scales (1/2/3/4). The U8g2 driver maps them
to fonts, which you can replace per role:

```cpp
displayCtx.fonts.title = u8g2_font_logisoso16_tf;
```

`setTextSize` still exists for app code on a scaling backend, but library chrome
only ever uses roles.

## Panel profiles

`DisplayProfile::forPanel` derives the top bar and popup geometry from the
panel's size and colour depth, in three bands: `<=80px` tall (small mono),
`<=160px`, and larger. `Display::begin` applies it — along with the matching
`MenuStyle::Metrics` — so a 128x64 panel gets a 12px underlined top bar and
8/12/18px menu rows instead of the 20px colour bar and 12/20/28px rows that
suit a 240px TFT.

Override after `begin`:

```cpp
display.begin(config, &screenManager);

DisplayProfile p = display.profile();
p.topBarHeight = 14;
display.setProfile(p);

MenuStyle::Metrics m = MenuStyle::metrics();
m.large.lineHeight = 20;
MenuStyle::setMetrics(m);
```

For a top bar the profile cannot express, replace it wholesale:

```cpp
display.setTopBarRenderer([](Display &d, const Screen2 *screen) { /* ... */ });
```

## License

GPL-3.0-or-later — see [LICENSE](LICENSE).
