#include "UiTest.h"

#include "Display.h"
#include "Input.h"
#include "Menu.h"
#include "Popup.h"
#include "ScreenManager.h"
#include "Widgets.h"

namespace UiTest
{
  extern const Screen2 MenuItemsScreen;
  extern const Screen2 InputScreen;
  extern const Screen2 PopupsScreen;
  extern const Screen2 WidgetsScreen;
  extern const Screen2 KeyboardScreen;
  extern const Screen2 DrawingScreen;
  extern const Screen2 RedrawScreen;
  extern const Screen2 BackgroundScreen;
}

namespace
{
  // Back, Left, a tap or a triple click: an exit every input setup can produce.
  bool isExit(const InputEvent &e)
  {
    return e.kind == InputKind::Back || e.kind == InputKind::Left || e.kind == InputKind::Tap ||
           (e.kind == InputKind::Select && e.rawClicks == 3);
  }

  void drawLine(const String &text, int16_t y, TextRole role = TextRole::Small)
  {
    display.setTextRole(role);
    display.setTextColor(display.theme().text);
    display.setTextDatum(Datum::TopLeft);
    display.drawString(text, 2, y);
  }

  int16_t top()
  {
    return static_cast<int16_t>(MenuStyle::contentTopY());
  }

  // ---------------------------------------------------------------- root
  Menu rootMenu(MenuSize::Medium);
  String rootPanel, rootModes;
  MenuItemBack rootBack;
  MenuItemString rootPanelItem("Panel", &rootPanel);
  MenuItemString rootModesItem("Mode", &rootModes);
  MenuItemNavigate rootItems("Menu items", &UiTest::MenuItemsScreen);
  MenuItemNavigate rootInput("Input events", &UiTest::InputScreen);
  MenuItemNavigate rootPopups("Popups", &UiTest::PopupsScreen);
  MenuItemNavigate rootWidgets("Buttons", &UiTest::WidgetsScreen);
  MenuItemNavigate rootKeyboard("Keyboard", &UiTest::KeyboardScreen);
  MenuItemNavigate rootDrawing("Drawing", &UiTest::DrawingScreen);
  MenuItemNavigate rootRedraw("Redraw", &UiTest::RedrawScreen);
  MenuItemNavigate rootBackground("Bg task", &UiTest::BackgroundScreen);

  void rootEnter()
  {
    if (rootMenu.items.empty())
    {
      for (MenuItem *item : std::initializer_list<MenuItem *>{&rootBack, &rootPanelItem, &rootModesItem, &rootItems,
                                                              &rootInput, &rootPopups, &rootWidgets, &rootKeyboard,
                                                              &rootDrawing, &rootRedraw, &rootBackground})
        rootMenu.addMenuItem(item);
    }
    rootPanel = String(display.width()) + "x" + display.height() + " " + display.colorDepth() + "b";
    rootModes = String(display.driver()->buffered() ? "buf" : "direct") + (Inputs::hasPointer() ? " touch" : "");
  }

  // ---------------------------------------------------------------- menu items
  Menu itemsMenu(MenuSize::Medium);
  Menu itemsSubmenu(MenuSize::Small);
  bool gToggle = false, gHide = false, gSubToggle = true;
  int gInt = 10, gSubInt = 3;
  float gFloat = 0.5f;
  uint8_t gByte = 240;
  uint32_t gUptime = 0;
  String gLast = "-";

  MenuItemBack itemsBack;
  MenuItemString lastItem("Last", &gLast);
  MenuItemToggle toggleItem("Toggle", &gToggle);
  MenuItemNumber<int> intItem("Int +2", &gInt, 0, 20, 2);
  MenuItemNumber<float> floatItem("Float", &gFloat, -1.0f, 1.0f, 0.05f);
  MenuItemNumber<uint8_t> byteItem("Byte +16", &gByte, 0, 255, 16);
  MenuItemNumber<uint32_t> uptimeItem("Uptime", &gUptime);
  MenuItemSelect selectItem("Select", {"Red", "Green", "Blue"});
  MenuItemAction clicksItem("Clicks", 1, []()
                            { gLast = "1 click"; });
  MenuItemToggle hideItem("Hide next", &gHide);
  MenuItemAction hiddenItem("Hideable", 1, []()
                            { gLast = "hideable"; });
  MenuItemAction colourItem("Coloured", 1, []()
                            { gLast = "coloured"; });
  MenuItemSubmenu submenuItem("Submenu", &itemsSubmenu);
  MenuItemBack subBack;
  MenuItemToggle subToggle("Sub toggle", &gSubToggle);
  MenuItemNumber<int> subInt("Sub int", &gSubInt, 0, 9);

  void itemsEnter()
  {
    if (!itemsMenu.items.empty())
      return;
    clicksItem.addFunc(2, []()
                       { gLast = "2 clicks"; });
    clicksItem.addFunc(3, []()
                       { gLast = "3 clicks"; });
    clicksItem.addFunc(-1, []()
                       { gLast = "long press"; });
    hideItem.setOnChange([]()
                         { hiddenItem.setHidden(gHide); });
    intItem.setOnChange([]()
                        { gLast = "int " + String(gInt); });
    floatItem.setFastUpdate(true);
    floatItem.setOnChange([]()
                          { gLast = "float " + String(gFloat, 2); });
    selectItem.setOnChange([]()
                           { gLast = selectItem.getSelectedOption(); });
    colourItem.setTextColor(Color::Orange);
    colourItem.setBgColor(Color::Orange);

    for (MenuItem *item : std::initializer_list<MenuItem *>{&itemsBack, &lastItem, &toggleItem, &intItem, &floatItem,
                                                            &byteItem, &uptimeItem, &selectItem, &clicksItem, &hideItem,
                                                            &hiddenItem, &colourItem, &submenuItem})
      itemsMenu.addMenuItem(item);
    itemsSubmenu.addMenuItem(&subBack);
    itemsSubmenu.addMenuItem(&subToggle);
    itemsSubmenu.addMenuItem(&subInt);
  }

  void itemsUpdate()
  {
    gUptime = millis() / 1000;
    itemsMenu.update();
  }

  // ---------------------------------------------------------------- input events
  constexpr size_t kLogLines = 8;
  String gLog[kLogLines];
  size_t gLogCount = 0;
  int16_t gPointerX = -1, gPointerY = -1;

  const char *kindName(InputKind k)
  {
    static const char *const names[] = {"Up", "Down", "Left", "Right", "Select", "Back",
                                        "Delta", "Tap", "PDown", "PMove", "PUp"};
    const size_t i = static_cast<size_t>(k);
    return i < sizeof(names) / sizeof(names[0]) ? names[i] : "?";
  }

  void logEvent(const InputEvent &e)
  {
    String line = kindName(e.kind);
    if (e.kind == InputKind::Delta)
      line += " " + String(e.value);
    else if (e.kind == InputKind::Select)
      line += " v" + String(e.value) + " raw" + String(e.rawClicks);
    else if (e.kind >= InputKind::Tap)
      line += " " + String(e.x) + "," + String(e.y);
    for (size_t i = kLogLines - 1; i > 0; i--)
      gLog[i] = gLog[i - 1];
    gLog[0] = line;
    if (gLogCount < kLogLines)
      gLogCount++;
  }

  void inputDraw()
  {
    display.setTextRole(TextRole::Small);
    const int16_t lh = display.fontHeight() + 1;
    drawLine("Back / 3 clicks to exit", top());
    const int16_t rows = (display.height() - top() - lh) / lh;
    for (int16_t i = 0; i < rows && i < static_cast<int16_t>(gLogCount); i++)
      drawLine(gLog[i], top() + lh * (i + 1));
    if (gPointerX >= 0)
    {
      display.drawLine(gPointerX - 6, gPointerY, gPointerX + 6, gPointerY, display.theme().accent);
      display.drawLine(gPointerX, gPointerY - 6, gPointerX, gPointerY + 6, display.theme().accent);
    }
  }

  void inputUpdate()
  {
    for (const InputEvent &e : Inputs::events())
    {
      if (e.kind == InputKind::Back || (e.kind == InputKind::Select && e.rawClicks == 3))
      {
        screenManager.back();
        return;
      }
      if (e.kind >= InputKind::Tap)
      {
        gPointerX = e.x;
        gPointerY = e.y;
      }
      logEvent(e);
    }
  }

  void inputEnter()
  {
    gLogCount = 0;
    gPointerX = gPointerY = -1;
  }

  // ---------------------------------------------------------------- popups
  Menu popupsMenu(MenuSize::Medium);
  String gChoice = "-";
  MenuItemBack popupsBack;
  MenuItemAction infoPopupItem("Info", 1, []()
                               { screenManager.showPopup(new Popup("Info", "A plain popup. Select, back or tap closes it, and long text wraps inside the box.")); });
  MenuItemAction autoPopupItem("Auto close", 1, []()
                               { screenManager.showPopup(new AutoClosePopup("Auto", "Closes in 3 s", 3000)); });
  MenuItemAction choicePopupItem("Choice", 1, []()
                                 { screenManager.showPopup(new ChoicePopup("Pick", "Which one?", {"No", "Yes", "Maybe"},
                                                                           [](int choice)
                                                                           { gChoice = choice < 0 ? String("cancel") : String(choice); })); });
  MenuItemString choiceResultItem("Chose", &gChoice);
  MenuItemAction taskPopupItem("From task", 1, []()
                               {
                                 xTaskCreate([](void *)
                                             {
                                               vTaskDelay(pdMS_TO_TICKS(800));
                                               screenManager.showPopup(new AutoClosePopup("Task", "Shown from another task", 2000));
                                               vTaskDelete(nullptr); },
                                             "uiTestPopup", 3072, nullptr, 1, nullptr); });
  MenuItemAction replacePopupItem("Replace", 1, []()
                                  {
                                    screenManager.showPopup(new Popup("First", "You should never see this"));
                                    screenManager.showPopup(new Popup("Second", "Replaced the first before it drew")); });

  void popupsEnter()
  {
    if (!popupsMenu.items.empty())
      return;
    for (MenuItem *item : std::initializer_list<MenuItem *>{&popupsBack, &infoPopupItem, &autoPopupItem, &choicePopupItem,
                                                            &choiceResultItem, &taskPopupItem, &replacePopupItem})
      popupsMenu.addMenuItem(item);
  }

  // ---------------------------------------------------------------- buttons
  int gCount = 0;
  String gPressed = "-";
  Button buttons[] = {
      Button("A", []()
             { gPressed = "A"; }),
      Button("B", []()
             { gPressed = "B"; }),
      Button("C", []()
             { gPressed = "C"; }),
      Button("-", []()
             { gCount--; }),
      Button("+", []()
             { gCount++; }),
      Button("Back", []()
             { screenManager.back(); }),
  };
  FocusGroup buttonGroup;
  bool gButtonsBuilt = false;

  void widgetsEnter()
  {
    if (!gButtonsBuilt)
    {
      gButtonsBuilt = true;
      for (Button &b : buttons)
        buttonGroup.add(b);
      buttonGroup.setColumns(3);
    }
    display.setTextRole(TextRole::Small);
    const int16_t y0 = top() + display.fontHeight() + 4;
    const int16_t gap = 3;
    const int16_t w = (display.width() - 4 * gap) / 3;
    const int16_t h = (display.height() - y0 - 3 * gap) / 2;
    for (int i = 0; i < 6; i++)
      buttons[i].bounds = Rect{static_cast<int16_t>(gap + (i % 3) * (w + gap)),
                               static_cast<int16_t>(y0 + gap + (i / 3) * (h + gap)), w, h};
  }

  void widgetsDraw()
  {
    drawLine("Count " + String(gCount) + "  Last " + gPressed, top() + 1);
    buttonGroup.draw();
  }

  void widgetsUpdate()
  {
    for (const InputEvent &e : Inputs::events())
    {
      if (e.kind == InputKind::Back)
      {
        screenManager.back();
        return;
      }
    }
    buttonGroup.update();
  }

  // ---------------------------------------------------------------- keyboard
  Keyboard keyboard;
  String gTyped;
  bool gKeyboardBuilt = false;

  void keyboardEnter()
  {
    if (!gKeyboardBuilt)
    {
      gKeyboardBuilt = true;
      keyboard.onKey = [](char c)
      { gTyped += c; };
      keyboard.onDelete = []()
      {
        if (gTyped.length())
          gTyped.remove(gTyped.length() - 1);
      };
      keyboard.onDone = []()
      { screenManager.back(); };
      keyboard.onCancel = keyboard.onDone;
    }
    display.setTextRole(TextRole::Small);
    const int16_t y = top() + display.fontHeight() + 4;
    keyboard.setBounds(Rect{0, y, static_cast<int16_t>(display.width()), static_cast<int16_t>(display.height() - y)});
  }

  void keyboardDraw()
  {
    drawLine(gTyped + "_", top() + 1);
    keyboard.draw();
  }

  void keyboardUpdate()
  {
    keyboard.update();
  }

  // ---------------------------------------------------------------- drawing
  void drawingDraw()
  {
    const Theme &t = display.theme();
    const int16_t W = display.width(), H = display.height(), T = top();
    const int16_t half = W / 2;
    const int16_t areaH = H - T;
    const int16_t cell = min<int16_t>(half / 3, areaH / 2) - 2;

    // Shapes, two rows of three on the left half.
    int16_t x = 1, y = T + 1;
    display.drawLine(x, y, x + cell, y + cell, t.text);
    display.drawLine(x, y + cell, x + cell, y, t.accent);
    x += cell + 2;
    display.drawRect(x, y, cell, cell, t.text);
    display.fillRect(x + cell / 4, y + cell / 4, cell / 2, cell / 2, t.accent);
    x += cell + 2;
    display.drawRoundRect(x, y, cell, cell, cell / 4, t.text);
    x = 1;
    y += cell + 2;
    display.fillRoundRect(x, y, cell, cell, cell / 4, t.highlight);
    x += cell + 2;
    display.drawSmoothArc(x + cell / 2, y + cell / 2, cell / 2, cell / 3, 45, 315, t.accent, t.background, true);
    x += cell + 2;
    for (int16_t i = 0; i < cell; i += 3)
      display.drawPixel(x + i, y + (i * i) / max<int16_t>(cell, 1), t.text);

    // Datums: nine labels anchored on one box; each should sit on its own corner/edge/centre.
    const int16_t bx = half + 2, by = T + 1, bw = W - half - 4, bh = areaH - 2;
    display.drawRect(bx, by, bw, bh, t.mutedText);
    display.setTextRole(TextRole::Small);
    display.setTextColor(t.text);
    static const char *const labels[] = {"TL", "TC", "TR", "ML", "MC", "MR", "BL", "BC", "BR"};
    for (uint8_t d = 0; d < 9; d++)
    {
      display.setTextDatum(d);
      display.drawString(labels[d], bx + (d % 3) * bw / 2, by + (d / 3) * bh / 2);
    }

    // Theme swatches along the bottom of the left half.
    const uint16_t swatches[] = {t.text, t.accent, t.highlight, t.topBar, t.popupProgress, t.focus, t.widget};
    const int16_t sw = max<int16_t>(3, (half - 2) / 7);
    const int16_t sy = H - sw - 1;
    if (sy > y + cell)
      for (int i = 0; i < 7; i++)
      {
        display.fillRect(1 + i * sw, sy, sw - 1, sw, swatches[i]);
        display.drawRect(1 + i * sw, sy, sw - 1, sw, t.text);
      }
  }

  void exitOnInput()
  {
    for (const InputEvent &e : Inputs::events())
      if (isExit(e) || e.kind == InputKind::Select)
      {
        screenManager.back();
        return;
      }
  }

  // ---------------------------------------------------------------- redraw
  uint32_t gDraws = 0, gUpdates = 0;

  void redrawEnter()
  {
    gDraws = gUpdates = 0;
  }

  void redrawDraw()
  {
    gDraws++;
    display.setTextRole(TextRole::Small);
    const int16_t lh = display.fontHeight() + 2;
    drawLine("Idle redraw every 1 s", top() + 1);
    drawLine("Turn/press: redraws now", top() + 1 + lh);
    drawLine("Draws " + String(gDraws) + "  Updates " + String(gUpdates), top() + 1 + 2 * lh);
    drawLine("Select/Back exits", top() + 1 + 3 * lh);
  }

  void redrawUpdate()
  {
    gUpdates++;
    exitOnInput();
  }

  // ---------------------------------------------------------------- background task
  Menu bgMenu(MenuSize::Medium);
  volatile uint32_t gSteps = 0, gOverruns = 0;
  uint32_t gStepsShown = 0, gOverrunsShown = 0;
  bool gSlow = false;
  String gTaskState;
  MenuItemBack bgBack;
  MenuItemString bgStateItem("Task", &gTaskState);
  MenuItemNumber<uint32_t> bgStepsItem("Steps", &gStepsShown);
  MenuItemToggle bgSlowItem("Overrun", &gSlow);
  MenuItemNumber<uint32_t> bgOverrunsItem("Overruns", &gOverrunsShown);

  // Runs on its own task every 200 ms. A 1.5 s step trips the 1 s watchdog, which
  // stops the task; the manager restarts it on the next update.
  void bgStep(const Screen2 *)
  {
    gSteps = gSteps + 1;
    if (gSlow)
    {
      gOverruns = gOverruns + 1;
      vTaskDelay(pdMS_TO_TICKS(1500));
    }
  }

  void bgEnter()
  {
    if (!bgMenu.items.empty())
      return;
    for (MenuItem *item : std::initializer_list<MenuItem *>{&bgBack, &bgStateItem, &bgStepsItem, &bgSlowItem, &bgOverrunsItem})
      bgMenu.addMenuItem(item);
  }

  void bgUpdate()
  {
    gStepsShown = gSteps;
    gOverrunsShown = gOverruns;
    gTaskState = screenManager.isBackgroundTaskActive(&UiTest::BackgroundScreen) ? "running" : "stopped";
    bgMenu.update();
  }

  Screen2 makeScreen(const char *name, void (*draw)(), void (*update)(), void (*onEnter)(), int32_t redrawMs = -1)
  {
    Screen2 s{};
    s.name = name;
    s.draw = draw;
    s.update = update;
    s.onEnter = onEnter;
    s.onExit = nullptr;
    s.redrawMs = redrawMs;
    return s;
  }
}

namespace UiTest
{
  const Screen2 RootScreen = makeScreen("UI test", []()
                                        { rootMenu.draw(); }, []()
                                        { rootMenu.update(); }, rootEnter);

  const Screen2 MenuItemsScreen = []()
  {
    Screen2 s = makeScreen("Menu items", []()
                           { itemsMenu.draw(); }, itemsUpdate, itemsEnter);
    s.topBarColor = Color::Orange;
    return s;
  }();

  const Screen2 InputScreen = makeScreen("Input", inputDraw, inputUpdate, inputEnter);

  const Screen2 PopupsScreen = makeScreen("Popups", []()
                                          { popupsMenu.draw(); }, []()
                                          { popupsMenu.update(); }, popupsEnter);

  const Screen2 WidgetsScreen = makeScreen("Buttons", widgetsDraw, widgetsUpdate, widgetsEnter);

  const Screen2 KeyboardScreen = makeScreen("Keyboard", keyboardDraw, keyboardUpdate, keyboardEnter);

  const Screen2 DrawingScreen = makeScreen("Drawing", drawingDraw, exitOnInput, nullptr, 1000);

  const Screen2 RedrawScreen = makeScreen("Redraw", redrawDraw, redrawUpdate, redrawEnter, 1000);

  const Screen2 BackgroundScreen = []()
  {
    Screen2 s = makeScreen("Bg task", []()
                           { bgMenu.draw(); }, bgUpdate, bgEnter);
    s.backgroundTask.step = bgStep;
    s.backgroundTask.name = "uiTestBg";
    s.backgroundTask.stackDepth = 3072;
    s.backgroundTask.loopDelayMs = 200;
    s.backgroundTask.watchdogTimeoutMs = 1000;
    return s;
  }();
}
