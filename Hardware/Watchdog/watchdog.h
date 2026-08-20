/**
 * @file    watchdog.h
 * @brief   Independent watchdog and reset-cause diagnostics.
 */
#ifndef __WATCHDOG_H
#define __WATCHDOG_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WATCHDOG_RESET_UNKNOWN = 0,
    WATCHDOG_RESET_PIN,
    WATCHDOG_RESET_POWER_ON,
    WATCHDOG_RESET_SOFTWARE,
    WATCHDOG_RESET_IWDG,
    WATCHDOG_RESET_WWDG,
    WATCHDOG_RESET_LOW_POWER
} WatchdogResetCause;

/** Read the RCC reset flags once and clear them for the next boot. */
WatchdogResetCause Watchdog_CaptureResetCause(void);

const char *Watchdog_ResetCauseText(WatchdogResetCause cause);

/** Start IWDG with a nominal timeout of approximately 4 seconds. */
bool Watchdog_Init(void);

/** Reload the IWDG counter after one healthy main-loop iteration. */
void Watchdog_Refresh(void);

uint32_t Watchdog_GetNominalTimeoutMs(void);

#ifdef __cplusplus
}
#endif

#endif
