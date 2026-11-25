#pragma once

#include <Arduino.h>
#include <memory>
#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Popup.h"
#include "ScreenTypes.h"

class Display;

// Owns the current screen, navigation history, the active popup and the
// current screen's background task. Screen changes are queued and applied on
// the render task, so `setScreen` and `showPopup` are safe from any task.
class ScreenManager
{
public:
  void init(Display &display);

  void draw();
  /** Runs the popup or the screen, then applies a pending screen change. */
  void update();

  const Screen2 *getCurrentScreen() const;
  void setScreen(const Screen2 *screen);
  /** Applies a queued `setScreen` now. True if the screen changed. */
  bool applyPendingScreenChange();

  void back();
  void clearHistory();
  bool goToHistoryIndex(size_t index);

  /** Takes ownership. Replaces any popup already showing. Safe from any task. */
  void showPopup(Popup *popup);
  /** Closes the active popup after its current update returns. */
  void closePopup();
  void drawPopup();
  bool isPopupActive() const;

  bool startBackgroundTask(ScreenBackgroundStep step, const char *name, uint32_t stackDepth,
                           uint32_t loopDelayMs = 1000, uint32_t watchdogTimeoutMs = 15000);
  /** Requests a stop; with `waitForStop`, blocks up to `timeoutMs` (0 = forever) for it to exit. */
  bool stopBackgroundTask(bool waitForStop = false, uint32_t timeoutMs = 0);
  bool isBackgroundTaskActive(const Screen2 *screen) const;

private:
  void updateHistory(const Screen2 *screen);
  void adoptPendingPopup();
  bool startRegisteredBackgroundTask();
  static void backgroundTaskRunner(void *param);
  static void backgroundWatchdogRunner(void *param);

  Display *display = nullptr;
  const Screen2 *currentScreen = nullptr;
  const Screen2 *volatile pendingScreen = nullptr;
  std::vector<const Screen2 *> screenHistory;

  std::unique_ptr<Popup> popup;
  Popup *pendingPopup = nullptr;
  volatile bool closeRequested = false;
  portMUX_TYPE popupMux = portMUX_INITIALIZER_UNLOCKED;

  // Lifecycle: no task deletes another. The runner and watchdog exit themselves,
  // and each clears a handle only while it still owns it (compare-and-clear).
  TaskHandle_t backgroundTaskHandle = nullptr;
  TaskHandle_t backgroundWatchdogHandle = nullptr;
  const Screen2 *backgroundTaskOwner = nullptr;
  ScreenBackgroundStep backgroundTaskStep = nullptr;
  uint32_t backgroundTaskLoopDelayMs = 1000;
  uint32_t backgroundTaskWatchdogTimeoutMs = 15000;
  volatile bool backgroundTaskShouldStop = false;
  volatile bool backgroundTaskStepInProgress = false;
  volatile uint32_t backgroundTaskStepStartMs = 0;
  volatile uint32_t backgroundTaskGeneration = 0;
  volatile bool backgroundStartInProgress = false;
  portMUX_TYPE backgroundTaskMux = portMUX_INITIALIZER_UNLOCKED;
};

extern ScreenManager screenManager;
