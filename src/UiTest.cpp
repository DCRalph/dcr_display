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
  extern const Screen2 GesturesScreen;
  extern const Screen2 AnimationScreen;
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
    Display::active().setTextRole(role);
    Display::active().setTextColor(Display::active().theme().text);
    Display::active().setTextDatum(Datum::TopLeft);
    Display::active().drawString(text, 2, y);
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
  MenuItemNavigate rootWidgets("Components", &UiTest::WidgetsScreen);
  MenuItemNavigate rootGestures("Gestures", &UiTest::GesturesScreen);
  MenuItemNavigate rootAnimation("Animation", &UiTest::AnimationScreen);
  MenuItemSelect rootTheme("Theme", {"Dark", "Light", "Bactosure", "Midnight", "Contrast", "Mono"});
  constexpr int kMonoTheme = 5;
  constexpr int kThemeCount = 6;

  Theme themeAt(int i)
  {
    switch (i)
    {
    case 1:
      return Theme::light();
    case 2:
      return Theme::bactosure();
    case 3:
      return Theme::midnight();
    case 4:
      return Theme::contrast();
    case kMonoTheme:
      return Theme::mono();
    default:
      return Theme::dark();
    }
  }

  // Which preset the display is showing, so the picker starts on it; 0 (Dark) if none.
  int themeIndexOf(const Theme &t)
  {
    for (int i = 0; i < kThemeCount; i++)
    {
      const Theme p = themeAt(i);
      if (p.background == t.background && p.accent == t.accent && p.highlight == t.highlight && p.oneBit == t.oneBit)
        return i;
    }
    return 0;
  }
  bool gMotion = true;
  MenuItemToggle rootMotion("Motion", &gMotion);
  MenuItemNavigate rootKeyboard("Keyboard", &UiTest::KeyboardScreen);
  MenuItemNavigate rootDrawing("Drawing", &UiTest::DrawingScreen);
  MenuItemNavigate rootRedraw("Redraw", &UiTest::RedrawScreen);
  MenuItemNavigate rootBackground("Bg task", &UiTest::BackgroundScreen);

  void rootEnter()
  {
    if (rootMenu.items.empty())
    {
      for (MenuItem *item : std::initializer_list<MenuItem *>{
               &rootBack, &rootPanelItem, &rootModesItem, &rootTheme, &rootMotion, &rootItems, &rootWidgets,
               &rootGestures, &rootAnimation, &rootPopups, &rootKeyboard, &rootInput, &rootDrawing, &rootRedraw,
               &rootBackground})
        rootMenu.addMenuItem(item);
      // Applies to whichever display the menu is on. A 1-bit panel can only show the mono theme.
      rootTheme.setOnChange([]()
                            {
                              Display &d = Display::active();
                              const int i = d.colorDepth() == 1 ? kMonoTheme : rootTheme.getCurrentIndex();
                              rootTheme.setCurrentIndex(i);
                              d.setTheme(themeAt(i)); });
      rootMotion.setOnChange([]()
                             { Display::active().setAnimations(gMotion); });
    }
    rootTheme.setCurrentIndex(themeIndexOf(Display::active().theme()));
    gMotion = Display::active().animationsEnabled();
    rootPanel = String(Display::active().width()) + "x" + Display::active().height() + " " + Display::active().colorDepth() + "b";
    rootModes = String(Display::active().driver()->buffered() ? "buf" : "direct") + (Inputs::hasPointer() ? " touch" : "");
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
  // Each editor style side by side; the unnamed ones follow the theme.
  int gInlineNum = 50;
  MenuItemNumber<int> inlineNumItem("Inline num", &gInlineNum, 0, 100, 5);
  MenuItemSelect sheetItem("Sheet", {"Celsius", "Fahrenheit", "Kelvin", "Rankine", "Reaumur"});
  MenuItemSelect inlineSelItem("Inline sel", {"Low", "Mid", "High"});
  MenuItemSelect lookItem("Look", {"Theme", "Flat", "Grouped"});
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

    inlineNumItem.setEditStyle(NumberEdit::Inline);
    sheetItem.setEditStyle(SelectEdit::Sheet);
    inlineSelItem.setEditStyle(SelectEdit::Inline);
    lookItem.setOnChange([]()
                         {
                           const MenuLook looks[] = {MenuLook::Theme, MenuLook::Flat, MenuLook::Grouped};
                           itemsMenu.setLook(looks[lookItem.getCurrentIndex()]); });
    for (MenuItem *item : std::initializer_list<MenuItem *>{
             &itemsBack, &lastItem, &toggleItem, &intItem, &floatItem, &inlineNumItem, &selectItem,
             &sheetItem, &inlineSelItem, &lookItem, &byteItem, &uptimeItem, &clicksItem, &hideItem, &hiddenItem,
             &colourItem, &submenuItem})
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
    static const char *const names[] = {"Up", "Down", "Left", "Right", "Select", "Back", "Delta",
                                        "Tap", "PDown", "PMove", "PUp", "Swipe", "Long", "DblTap"};
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
    Display::active().setTextRole(TextRole::Small);
    const int16_t lh = Display::active().fontHeight() + 1;
    drawLine("Back / 3 clicks to exit", top());
    const int16_t rows = (Display::active().height() - top() - lh) / lh;
    for (int16_t i = 0; i < rows && i < static_cast<int16_t>(gLogCount); i++)
      drawLine(gLog[i], top() + lh * (i + 1));
    if (gPointerX >= 0)
    {
      Display::active().drawLine(gPointerX - 6, gPointerY, gPointerX + 6, gPointerY, Display::active().theme().accent);
      Display::active().drawLine(gPointerX, gPointerY - 6, gPointerX, gPointerY + 6, Display::active().theme().accent);
    }
  }

  void inputUpdate()
  {
    for (const InputEvent &e : Inputs::events())
    {
      if (e.kind == InputKind::Back || (e.kind == InputKind::Select && e.rawClicks == 3))
      {
        Display::active().screens().back();
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
                               { Display::active().screens().showPopup(new Popup("Info", "A plain popup. Select, back or tap closes it, and long text wraps inside the box.")); });
  MenuItemAction autoPopupItem("Auto close", 1, []()
                               { Display::active().screens().showPopup(new AutoClosePopup("Auto", "Closes in 3 s", 3000)); });
  MenuItemAction choicePopupItem("Choice", 1, []()
                                 { Display::active().screens().showPopup(new ChoicePopup("Pick", "Which one?", {"No", "Yes", "Maybe"},
                                                                           [](int choice)
                                                                           { gChoice = choice < 0 ? String("cancel") : String(choice); })); });
  MenuItemString choiceResultItem("Chose", &gChoice);
  MenuItemAction taskPopupItem("From task", 1, []()
                               {
                                 xTaskCreate([](void *)
                                             {
                                               vTaskDelay(pdMS_TO_TICKS(800));
                                               Display::active().screens().showPopup(new AutoClosePopup("Task", "Shown from another task", 2000));
                                               vTaskDelete(nullptr); },
                                             "uiTestPopup", 3072, nullptr, 1, nullptr); });
  MenuItemAction replacePopupItem("Replace", 1, []()
                                  {
                                    Display::active().screens().showPopup(new Popup("First", "You should never see this"));
                                    Display::active().screens().showPopup(new Popup("Second", "Replaced the first before it drew")); });

  void popupsEnter()
  {
    if (!popupsMenu.items.empty())
      return;
    for (MenuItem *item : std::initializer_list<MenuItem *>{&popupsBack, &infoPopupItem, &autoPopupItem, &choicePopupItem,
                                                            &choiceResultItem, &taskPopupItem, &replacePopupItem})
      popupsMenu.addMenuItem(item);
  }

  // ---------------------------------------------------------------- components
  bool gWifi = true, gSound = false;
  int gLevel = 60;
  String gLastAction = "-";
  Button saveButton("Save", []()
                    { gLastAction = "Saved"; }, Button::Style::Primary);
  Button cancelButton("Cancel", []()
                      { gLastAction = "Cancelled"; });
  Button moreButton("More", []()
                    { gLastAction = "More"; }, Button::Style::Ghost);
  Button componentsBack("Back", []()
                        { Display::active().screens().back(); });
  Switch wifiSwitch("Wi-Fi", &gWifi, [](bool on)
                    { gLastAction = on ? "Wi-Fi on" : "Wi-Fi off"; });
  Switch soundSwitch("Sounds", &gSound, [](bool on)
                     { gLastAction = on ? "Sounds on" : "Sounds off"; });
  Slider levelSlider("Level", &gLevel, 0, 100, 5);
  FocusGroup components;

  // Lays the widgets out for the panel: everything on a big one, the essentials on 128x64.
  void componentsEnter()
  {
    Display &d = Display::active();
    components.clear();
    levelSlider.suffix = "%";
    const int16_t W = d.width(), gap = d.height() < 100 ? 2 : 6;
    int16_t y = top() + gap;
    auto place = [&](Widget &w, int16_t x, int16_t width, int16_t h)
    {
      w.bounds = Rect{x, y, width, h};
      components.add(w);
    };

    if (d.height() < 100)
    {
      const int16_t half = (W - 3 * gap) / 2;
      place(saveButton, gap, half, 12);
      place(componentsBack, gap * 2 + half, half, 12);
      y += 12 + gap;
      place(wifiSwitch, 0, W, 12);
      y += 12 + gap;
      place(levelSlider, 0, W, d.height() - y);
      return;
    }

    const int16_t bh = d.height() >= 200 ? 36 : 30;
    const int16_t quarter = (W - 5 * gap) / 4;
    place(saveButton, gap, quarter, bh);
    place(cancelButton, gap * 2 + quarter, quarter, bh);
    place(moreButton, gap * 3 + quarter * 2, quarter, bh);
    place(componentsBack, gap * 4 + quarter * 3, quarter, bh);
    y += bh + gap;
    const int16_t half = (W - 3 * gap) / 2;
    place(wifiSwitch, gap, half, bh);
    place(soundSwitch, gap * 2 + half, half, bh);
    y += bh + gap;
    place(levelSlider, gap, W - 2 * gap, min<int16_t>(52, d.height() - y - 14));
  }

  void componentsDraw()
  {
    components.draw();
    Display &d = Display::active();
    if (d.height() >= 100)
      drawLine(gLastAction, d.height() - 11);
  }

  void componentsUpdate()
  {
    for (const InputEvent &e : Inputs::events())
    {
      if (e.kind == InputKind::Back || (e.kind == InputKind::Select && e.rawClicks == 3))
      {
        Display::active().screens().back();
        return;
      }
    }
    components.update();
  }

  // ---------------------------------------------------------------- gestures
  String gGesture;
  int16_t gGestureX = 0, gGestureY = 0;
  SwipeDir gSwipeDir = SwipeDir::Left;
  bool gGestureIsSwipe = false;
  Tween gRipple;
  Button gesturesBack("Back", []()
                      { Display::active().screens().back(); });
  FocusGroup gesturesGroup;

  void gesturesEnter()
  {
    gGesture = Inputs::hasPointer() ? "Tap, swipe, hold or double tap" : "Needs a touch screen";
    gGestureX = gGestureY = -1;
    gesturesGroup.clear();
    Display &d = Display::active();
    gesturesBack.bounds = Rect{4, static_cast<int16_t>(d.height() - (d.height() < 100 ? 13 : 34)),
                               static_cast<int16_t>(d.height() < 100 ? 36 : 80),
                               static_cast<int16_t>(d.height() < 100 ? 12 : 30)};
    gesturesGroup.add(gesturesBack);
  }

  void gesturesDraw()
  {
    Display &d = Display::active();
    const Theme &t = d.theme();
    const int16_t cy = top() + (d.height() - top()) / 2 - 8;
    d.setTextRole(d.height() < 100 ? TextRole::Small : TextRole::Title);
    d.setTextColor(t.text);
    d.setTextDatum(Datum::MiddleCenter);
    d.drawString(gGesture, d.width() / 2, cy);

    if (gGestureX >= 0)
    {
      const float p = gRipple.value(); // 0 -> 1 over the ripple
      if (gGestureIsSwipe)
      {
        const int16_t len = static_cast<int16_t>(60 * p);
        const int16_t dx = gSwipeDir == SwipeDir::Left ? -len : gSwipeDir == SwipeDir::Right ? len : 0;
        const int16_t dy = gSwipeDir == SwipeDir::Up ? -len : gSwipeDir == SwipeDir::Down ? len : 0;
        for (int16_t o = -1; o <= 1; o++)
          d.drawLine(gGestureX + (dy ? o : 0), gGestureY + (dx ? o : 0), gGestureX + dx + (dy ? o : 0),
                     gGestureY + dy + (dx ? o : 0), t.accent);
        d.fillRoundRect(gGestureX + dx - 4, gGestureY + dy - 4, 8, 8, 4, t.accent);
      }
      else if (p < 1)
      {
        const int16_t r = static_cast<int16_t>(6 + 34 * p);
        d.drawSmoothArc(gGestureX, gGestureY, r, max<int16_t>(0, r - 3), 0, 0, t.accent, t.background, false);
      }
    }
    gesturesGroup.draw();
  }

  void gesturesUpdate()
  {
    for (const InputEvent &e : Inputs::events())
    {
      if (e.kind == InputKind::Back || e.kind == InputKind::Left || (e.kind == InputKind::Select && e.rawClicks == 3))
      {
        Display::active().screens().back();
        return;
      }
      if (gesturesBack.bounds.contains(e.x, e.y) && e.kind >= InputKind::Tap)
        continue; // the Back button's own press
      const char *name = nullptr;
      switch (e.kind)
      {
      case InputKind::Tap:
        name = "Tap";
        break;
      case InputKind::DoubleTap:
        name = "Double tap";
        break;
      case InputKind::LongPress:
        name = "Long press";
        break;
      case InputKind::Swipe:
      {
        static const char *const dirs[] = {"Swipe left", "Swipe right", "Swipe up", "Swipe down"};
        name = dirs[e.value & 3];
        gSwipeDir = static_cast<SwipeDir>(e.value);
        break;
      }
      default:
        break;
      }
      if (!name)
        continue;
      gGesture = name;
      gGestureX = e.x;
      gGestureY = e.y;
      gGestureIsSwipe = e.kind == InputKind::Swipe;
      gRipple.start(0.0f, 1.0f, gGestureIsSwipe ? 260 : 420, Ease::outCubic);
    }
    gesturesGroup.update();
  }

  // ---------------------------------------------------------------- animation
  struct Lane
  {
    const char *name;
    Ease::Fn ease;
    Tween t;
  };
  Lane lanes[] = {{"linear", Ease::linear, {}},
                  {"outQuad", Ease::outQuad, {}},
                  {"outCubic", Ease::outCubic, {}},
                  {"inOutCubic", Ease::inOutCubic, {}},
                  {"outBack", Ease::outBack, {}}};
  Tween gBar;

  void animationEnter()
  {
    for (Lane &l : lanes)
      l.t.snap(0);
    gBar.snap(0);
  }

  void animationDraw()
  {
    Display &d = Display::active();
    const Theme &t = d.theme();
    const bool small = d.height() < 100;
    const int16_t lanes_n = small ? 3 : 5;
    const int16_t labelW = small ? 0 : 92;
    const int16_t barH = small ? 6 : 10;
    const int16_t laneH = (d.height() - top() - barH - 8) / lanes_n;
    const int16_t ball = min<int16_t>(laneH - 4, small ? 8 : 14);
    // outBack overshoots by about 10%: leave room so it stays on screen.
    const int16_t x0 = labelW + 4, span = (d.width() - x0 - ball - 6) * 88 / 100;

    for (int16_t i = 0; i < lanes_n; i++)
    {
      Lane &l = lanes[small ? i * 2 : i];
      // Ping-pong: each lane heads back once it arrives.
      if (!l.t.running())
        l.t.start(l.t.value() < 0.5f ? 1.0f : 0.0f, 1200, l.ease);
      const int16_t y = top() + 2 + i * laneH;
      if (!small)
        drawLine(l.name, y + (laneH - 8) / 2);
      d.drawLine(x0, y + laneH / 2, x0 + span + ball, y + laneH / 2, t.border);
      d.fillRoundRect(x0 + static_cast<int16_t>(span * l.t.value()), y + (laneH - ball) / 2, ball, ball, ball / 2,
                      t.accent);
    }

    if (!gBar.running())
      gBar.start(gBar.value() < 0.5f ? 1.0f : 0.0f, 1600, Ease::inOutCubic);
    const int16_t by = d.height() - barH - 4;
    const int16_t bw = d.width() - 8;
    d.fillRoundRect(4, by, bw, barH, barH / 2, t.surface);
    d.drawRoundRect(4, by, bw, barH, barH / 2, t.border);
    d.fillRoundRect(4, by, max<int16_t>(barH, static_cast<int16_t>(bw * gBar.value())), barH, barH / 2, t.accent);
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
      { Display::active().screens().back(); };
      keyboard.onCancel = keyboard.onDone;
    }
    Display::active().setTextRole(TextRole::Small);
    const int16_t y = top() + Display::active().fontHeight() + 4;
    keyboard.setBounds(Rect{0, y, static_cast<int16_t>(Display::active().width()), static_cast<int16_t>(Display::active().height() - y)});
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
    const Theme &t = Display::active().theme();
    const int16_t W = Display::active().width(), H = Display::active().height(), T = top();
    const int16_t half = W / 2;
    const int16_t areaH = H - T;
    const int16_t cell = min<int16_t>(half / 3, areaH / 2) - 2;

    // Shapes, two rows of three on the left half.
    int16_t x = 1, y = T + 1;
    Display::active().drawLine(x, y, x + cell, y + cell, t.text);
    Display::active().drawLine(x, y + cell, x + cell, y, t.accent);
    x += cell + 2;
    Display::active().drawRect(x, y, cell, cell, t.text);
    Display::active().fillRect(x + cell / 4, y + cell / 4, cell / 2, cell / 2, t.accent);
    x += cell + 2;
    Display::active().drawRoundRect(x, y, cell, cell, cell / 4, t.text);
    x = 1;
    y += cell + 2;
    Display::active().fillRoundRect(x, y, cell, cell, cell / 4, t.highlight);
    x += cell + 2;
    Display::active().drawSmoothArc(x + cell / 2, y + cell / 2, cell / 2, cell / 3, 45, 315, t.accent, t.background, true);
    x += cell + 2;
    for (int16_t i = 0; i < cell; i += 3)
      Display::active().drawPixel(x + i, y + (i * i) / max<int16_t>(cell, 1), t.text);

    // Datums: nine labels anchored on one box; each should sit on its own corner/edge/centre.
    const int16_t bx = half + 2, by = T + 1, bw = W - half - 4, bh = areaH - 2;
    Display::active().drawRect(bx, by, bw, bh, t.mutedText);
    Display::active().setTextRole(TextRole::Small);
    Display::active().setTextColor(t.text);
    static const char *const labels[] = {"TL", "TC", "TR", "ML", "MC", "MR", "BL", "BC", "BR"};
    for (uint8_t d = 0; d < 9; d++)
    {
      Display::active().setTextDatum(d);
      Display::active().drawString(labels[d], bx + (d % 3) * bw / 2, by + (d / 3) * bh / 2);
    }

    // Theme swatches along the bottom of the left half.
    const uint16_t swatches[] = {t.text, t.accent, t.highlight, t.topBar, t.popupProgress, t.focus, t.widget};
    const int16_t sw = max<int16_t>(3, (half - 2) / 7);
    const int16_t sy = H - sw - 1;
    if (sy > y + cell)
      for (int i = 0; i < 7; i++)
      {
        Display::active().fillRect(1 + i * sw, sy, sw - 1, sw, swatches[i]);
        Display::active().drawRect(1 + i * sw, sy, sw - 1, sw, t.text);
      }
  }

  void exitOnInput()
  {
    for (const InputEvent &e : Inputs::events())
      if (isExit(e) || e.kind == InputKind::Select)
      {
        Display::active().screens().back();
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
    Display::active().setTextRole(TextRole::Small);
    const int16_t lh = Display::active().fontHeight() + 2;
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
    gTaskState = Display::active().screens().isBackgroundTaskActive(&UiTest::BackgroundScreen) ? "running" : "stopped";
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

  const Screen2 WidgetsScreen = makeScreen("Components", componentsDraw, componentsUpdate, componentsEnter);

  const Screen2 GesturesScreen = makeScreen("Gestures", gesturesDraw, gesturesUpdate, gesturesEnter);

  const Screen2 AnimationScreen = makeScreen("Animation", animationDraw, exitOnInput, animationEnter);

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
