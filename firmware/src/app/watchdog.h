#pragma once

#include <Arduino.h>
#include <esp_task_wdt.h>

// Dernier passage de chaque tâche surveillée (ms). Relu par le gestionnaire
// du watchdog (main.cpp) pour nommer la tâche qui bloquait.
enum WdtTask : uint8_t { WDT_SENSORS, WDT_COMMS, WDT_WEB, WDT_COUNT };
extern volatile uint32_t wdtPetMs[WDT_COUNT];

inline void wdtPet(WdtTask t) {
  wdtPetMs[t] = xTaskGetTickCount() * portTICK_PERIOD_MS;
  esp_task_wdt_reset();
}
