#include "ScreenManager.h"

#include "Display.h"

#undef LOG_TAG
#define LOG_TAG "SCREEN"
#include <dcr_Logger.h>

namespace
{
  constexpr size_t kMaxHistory = 10;
}

void ScreenManager::init(Display &displayRef)
{
  display = &displayRef;
  displayRef.setScreenManager(this);
}

void ScreenManager::draw()
{
  const Screen2 *s = currentScreen;
  if (s && s->draw)
    s->draw();
}

void ScreenManager::update()
{
  adoptPendingPopup();

  if (popup)
  {
    popup->update();
    if (closeRequested)
    {
      popup.reset();
      closeRequested = false;
      if (display)
        display->invalidate();
    }
  }
  else
  {
    if (currentScreen && currentScreen->backgroundTask.step && !isBackgroundTaskActive(currentScreen))
      startRegisteredBackgroundTask();

    const Screen2 *s = currentScreen;
    if (s && s->update)
      s->update();
  }

  if (applyPendingScreenChange() && display)
    display->invalidate();
}

const Screen2 *ScreenManager::getCurrentScreen() const
{
  const Screen2 *pending = pendingScreen;
  return pending ? pending : currentScreen;
}

void ScreenManager::setScreen(const Screen2 *screen)
{
  if (screen == nullptr)
    return;
  pendingScreen = screen;
}

bool ScreenManager::applyPendingScreenChange()
{
  const Screen2 *next = pendingScreen;
  if (next == nullptr)
    return false;
  pendingScreen = nullptr;

  if (next->draw == nullptr)
  {
    debugE("Screen '%s' has no draw function", next->name);
    return false;
  }
  if (next == currentScreen)
    return false;

  stopBackgroundTask();

  if (currentScreen && currentScreen->onExit)
    currentScreen->onExit();

  currentScreen = next;
  debugI(">> %s", currentScreen->name);
  updateHistory(currentScreen);

  if (currentScreen->onEnter)
    currentScreen->onEnter();

  if (currentScreen->backgroundTask.step)
    startRegisteredBackgroundTask();

  return true;
}

void ScreenManager::back()
{
  if (screenHistory.size() > 1)
  {
    screenHistory.pop_back();
    pendingScreen = screenHistory.back();
  }
}

void ScreenManager::clearHistory()
{
  const Screen2 *current = screenHistory.empty() ? nullptr : screenHistory.back();
  screenHistory.clear();
  if (current != nullptr)
    screenHistory.push_back(current);
}

void ScreenManager::updateHistory(const Screen2 *screen)
{
  if (screenHistory.empty() || screenHistory.back() != screen)
  {
    screenHistory.push_back(screen);
    if (screenHistory.size() > kMaxHistory)
      screenHistory.erase(screenHistory.begin());
  }
}

bool ScreenManager::goToHistoryIndex(size_t index)
{
  if (index >= screenHistory.size() || index == screenHistory.size() - 1)
    return false;
  pendingScreen = screenHistory[index];
  screenHistory.resize(index + 1);
  return true;
}

void ScreenManager::showPopup(Popup *p)
{
  if (p == nullptr)
    return;
  taskENTER_CRITICAL(&popupMux);
  Popup *replaced = pendingPopup;
  pendingPopup = p;
  taskEXIT_CRITICAL(&popupMux);
  // Never shown, so no other task can hold it.
  delete replaced;
  if (display)
    display->invalidate();
}

void ScreenManager::adoptPendingPopup()
{
  taskENTER_CRITICAL(&popupMux);
  Popup *next = pendingPopup;
  pendingPopup = nullptr;
  taskEXIT_CRITICAL(&popupMux);
  if (next)
  {
    popup.reset(next);
    closeRequested = false;
  }
}

void ScreenManager::closePopup()
{
  closeRequested = true;
}

void ScreenManager::drawPopup()
{
  if (popup && !closeRequested)
    popup->draw();
}

bool ScreenManager::isPopupActive() const
{
  return popup != nullptr || pendingPopup != nullptr;
}

bool ScreenManager::startRegisteredBackgroundTask()
{
  const ScreenBackgroundTaskConfig &cfg = currentScreen->backgroundTask;
  if (backgroundTaskHandle != nullptr)
    return false; // previous runner still draining; retried from update()
  return startBackgroundTask(cfg.step, cfg.name ? cfg.name : currentScreen->name, cfg.stackDepth,
                             cfg.loopDelayMs, cfg.watchdogTimeoutMs);
}

void ScreenManager::backgroundTaskRunner(void *param)
{
  auto *m = static_cast<ScreenManager *>(param);
  const Screen2 *owner = m->backgroundTaskOwner;

  while (m->isBackgroundTaskActive(owner) && m->backgroundTaskStep != nullptr)
  {
    // Start time before the flag: the watchdog must never pair the new flag with a stale time.
    m->backgroundTaskStepStartMs = millis();
    m->backgroundTaskStepInProgress = true;
    m->backgroundTaskStep(owner);
    m->backgroundTaskStepInProgress = false;

    for (uint32_t waited = 0; waited < m->backgroundTaskLoopDelayMs && m->isBackgroundTaskActive(owner); waited += 50)
      vTaskDelay(pdMS_TO_TICKS(50));
  }

  m->backgroundTaskStepInProgress = false;
  taskENTER_CRITICAL(&m->backgroundTaskMux);
  if (m->backgroundTaskHandle == xTaskGetCurrentTaskHandle())
  {
    m->backgroundTaskHandle = nullptr;
    m->backgroundTaskOwner = nullptr;
    m->backgroundTaskStep = nullptr;
    m->backgroundTaskShouldStop = false;
    m->backgroundTaskStepStartMs = 0;
  }
  taskEXIT_CRITICAL(&m->backgroundTaskMux);
  vTaskDelete(nullptr);
}

void ScreenManager::backgroundWatchdogRunner(void *param)
{
  auto *m = static_cast<ScreenManager *>(param);
  const uint32_t generation = m->backgroundTaskGeneration;

  while (m->backgroundTaskGeneration == generation && m->backgroundTaskHandle != nullptr)
  {
    if (m->backgroundTaskStepInProgress)
    {
      const uint32_t elapsed = millis() - m->backgroundTaskStepStartMs;
      if (elapsed > m->backgroundTaskWatchdogTimeoutMs)
      {
        debugE("Background step overran %lums, stopping", (unsigned long)elapsed);
        m->backgroundTaskShouldStop = true;
        break;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }

  taskENTER_CRITICAL(&m->backgroundTaskMux);
  if (m->backgroundWatchdogHandle == xTaskGetCurrentTaskHandle())
    m->backgroundWatchdogHandle = nullptr;
  taskEXIT_CRITICAL(&m->backgroundTaskMux);
  vTaskDelete(nullptr);
}

bool ScreenManager::startBackgroundTask(ScreenBackgroundStep step, const char *name, uint32_t stackDepth,
                                        uint32_t loopDelayMs, uint32_t watchdogTimeoutMs)
{
  if (currentScreen == nullptr || step == nullptr)
    return false;

  taskENTER_CRITICAL(&backgroundTaskMux);
  if (backgroundTaskHandle != nullptr || backgroundWatchdogHandle != nullptr || backgroundStartInProgress)
  {
    taskEXIT_CRITICAL(&backgroundTaskMux);
    return false;
  }
  backgroundStartInProgress = true;
  backgroundTaskGeneration++;
  taskEXIT_CRITICAL(&backgroundTaskMux);

  backgroundTaskShouldStop = false;
  backgroundTaskOwner = currentScreen;
  backgroundTaskStep = step;
  backgroundTaskLoopDelayMs = loopDelayMs;
  backgroundTaskWatchdogTimeoutMs = watchdogTimeoutMs;
  backgroundTaskStepInProgress = false;
  backgroundTaskStepStartMs = 0;

  if (xTaskCreate(backgroundTaskRunner, name, stackDepth, this, 1, &backgroundTaskHandle) != pdPASS)
  {
    backgroundTaskHandle = nullptr;
    backgroundTaskOwner = nullptr;
    backgroundTaskStep = nullptr;
    backgroundStartInProgress = false;
    debugE("Could not start background task '%s'", name);
    return false;
  }

  if (xTaskCreate(backgroundWatchdogRunner, "screenBgWdog", 3072, this, 2, &backgroundWatchdogHandle) != pdPASS)
  {
    // The runner may already be mid-step on the other core: let it see the stop and exit itself.
    backgroundWatchdogHandle = nullptr;
    backgroundTaskShouldStop = true;
    backgroundStartInProgress = false;
    return false;
  }

  backgroundStartInProgress = false;
  return true;
}

bool ScreenManager::stopBackgroundTask(bool waitForStop, uint32_t timeoutMs)
{
  backgroundTaskShouldStop = true;

  TaskHandle_t handle = backgroundTaskHandle;
  if (!waitForStop || handle == nullptr)
    return true;
  if (handle == xTaskGetCurrentTaskHandle())
    return false;

  const uint32_t start = millis();
  while (backgroundTaskHandle != nullptr)
  {
    if (timeoutMs > 0 && millis() - start >= timeoutMs)
      return false;
    vTaskDelay(pdMS_TO_TICKS(20));
  }
  return true;
}

bool ScreenManager::isBackgroundTaskActive(const Screen2 *screen) const
{
  return backgroundTaskHandle != nullptr && backgroundTaskOwner == screen && !backgroundTaskShouldStop;
}

ScreenManager screenManager;
