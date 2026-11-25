#pragma once

#include "ScreenTypes.h"

// Screens that exercise every dcr_display feature on the app's own panel and
// inputs: menu items, input events, popups, widgets, the keyboard, drawing and
// datums, redraw policy and background tasks. Add one menu entry to reach them:
//   MenuItemNavigate uiTestItem("UI test", &UiTest::RootScreen);
// Screens without a menu exit on Back, Left, a tap, or a triple click.
namespace UiTest
{
  extern const Screen2 RootScreen;
}
