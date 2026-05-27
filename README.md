# dcr_display

ESP32 display library: screen management, menus, popups, and a TFT_eSPI-backed driver behind a small drawing facade.

## Install

**PlatformIO** — add to `platformio.ini`:

```ini
lib_deps =
  https://github.com/DCRalph/dcr_display.git
```

**Arduino** — clone into `Arduino/libraries/dcr_display` or use Library Manager once published.

## Quick start

```cpp
#include "Display.h"
#include "TFTDisplayDriver.h"
#include "ScreenManager.h"

TFTDisplayDriverContext tftCtx;
ScreenManager screenManager;

void setup() {
  DisplayConfig config = TFTDisplayDriver::makeConfig(&tftCtx, 320, 240, 1);
  display.begin(config, &screenManager);
  screenManager.init(display);
}
```

## License

GPL-3.0-or-later — see [LICENSE](LICENSE).
