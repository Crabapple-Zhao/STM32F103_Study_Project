/**
 * @file    watchdog.c
 * @brief   STM32F103 independent watchdog implementation.
 */
#include "watchdog.h"
#include "stm32f1xx_hal.h"

#define WATCHDOG_IWDG_START_KEY          0xCCCCU
#define WATCHDOG_IWDG_WRITE_ACCESS_KEY   0x5555U
#define WATCHDOG_IWDG_REFRESH_KEY        0xAAAAU
#define WATCHDOG_IWDG_PRESCALER_256      0x06U
#define WATCHDOG_IWDG_RELOAD             624U
#define WATCHDOG_UPDATE_TIMEOUT_MS       100U
#define WATCHDOG_NOMINAL_TIMEOUT_MS      4000U

WatchdogResetCause Watchdog_CaptureResetCause(void)
{
    uint32_t flags = RCC->CSR;
    WatchdogResetCause cause = WATCHDOG_RESET_UNKNOWN;

    if ((flags & RCC_CSR_IWDGRSTF) != 0U) {
        cause = WATCHDOG_RESET_IWDG;
    } else if ((flags & RCC_CSR_WWDGRSTF) != 0U) {
        cause = WATCHDOG_RESET_WWDG;
    } else if ((flags & RCC_CSR_SFTRSTF) != 0U) {
        cause = WATCHDOG_RESET_SOFTWARE;
    } else if ((flags & RCC_CSR_LPWRRSTF) != 0U) {
        cause = WATCHDOG_RESET_LOW_POWER;
    } else if ((flags & RCC_CSR_PORRSTF) != 0U) {
        cause = WATCHDOG_RESET_POWER_ON;
    } else if ((flags & RCC_CSR_PINRSTF) != 0U) {
        cause = WATCHDOG_RESET_PIN;
    }

    RCC->CSR |= RCC_CSR_RMVF;
    return cause;
}

const char *Watchdog_ResetCauseText(WatchdogResetCause cause)
{
    switch (cause) {
        case WATCHDOG_RESET_PIN:       return "pin";
        case WATCHDOG_RESET_POWER_ON:  return "power_on";
        case WATCHDOG_RESET_SOFTWARE:  return "software";
        case WATCHDOG_RESET_IWDG:      return "watchdog";
        case WATCHDOG_RESET_WWDG:      return "window_watchdog";
        case WATCHDOG_RESET_LOW_POWER: return "low_power";
        default:                       return "unknown";
    }
}

bool Watchdog_Init(void)
{
    uint32_t startTick;

    /* LSI nominal 40 kHz: (624 + 1) * 256 / 40000 = 4 seconds. */
    IWDG->KR = WATCHDOG_IWDG_START_KEY;
    IWDG->KR = WATCHDOG_IWDG_WRITE_ACCESS_KEY;
    IWDG->PR = WATCHDOG_IWDG_PRESCALER_256;
    IWDG->RLR = WATCHDOG_IWDG_RELOAD;

    startTick = HAL_GetTick();
    while ((IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) != 0U) {
        if ((HAL_GetTick() - startTick) > WATCHDOG_UPDATE_TIMEOUT_MS) {
            return false;
        }
    }

    IWDG->KR = WATCHDOG_IWDG_REFRESH_KEY;
    return true;
}

void Watchdog_Refresh(void)
{
    IWDG->KR = WATCHDOG_IWDG_REFRESH_KEY;
}

uint32_t Watchdog_GetNominalTimeoutMs(void)
{
    return WATCHDOG_NOMINAL_TIMEOUT_MS;
}
