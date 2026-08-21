/**
 * @file    cs100a_stm32f103.c
 * @brief   Register-level CS100A capture port for STM32F103C8T6.
 */
#include "cs100a_stm32f103.h"
#include "stm32f1xx.h"

#define CS100A_TRIG_PIN             GPIO_ODR_ODR15
#define CS100A_ECHO_TIMEOUT_US      40000U
#define CS100A_TRIGGER_PULSE_US     15U

typedef enum {
    CAPTURE_IDLE = 0,
    CAPTURE_WAIT_RISE,
    CAPTURE_WAIT_FALL,
    CAPTURE_READY,
    CAPTURE_ERROR
} CaptureState;

static volatile CaptureState capture_state = CAPTURE_IDLE;
static volatile uint16_t capture_rise = 0U;
static volatile uint32_t capture_pulse_us = 0U;
static uint32_t measurement_start_cycles = 0U;
static uint32_t cycles_per_us = 0U;
static uint8_t port_initialized = 0U;

static bool port_init(void *context)
{
    uint32_t mapr;
    (void)context;
    if (port_initialized != 0U) return true;

    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN |
                    RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* Keep SWD enabled, release PA15/PB3 from JTAG, remap TIM2_CH2 to PB3. */
    mapr = AFIO->MAPR;
    mapr &= ~(AFIO_MAPR_SWJ_CFG | AFIO_MAPR_TIM2_REMAP);
    mapr |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE |
            AFIO_MAPR_TIM2_REMAP_PARTIALREMAP1;
    AFIO->MAPR = mapr;

    /* PA15: 2 MHz push-pull output, idle low. */
    GPIOA->CRH = (GPIOA->CRH & ~(0xFU << 28)) | (0x2U << 28);
    GPIOA->BRR = CS100A_TRIG_PIN;

    /* PB3: input with pull-down for a defined disconnected state. */
    GPIOB->CRL = (GPIOB->CRL & ~(0xFU << 12)) | (0x8U << 12);
    GPIOB->BRR = GPIO_ODR_ODR3;

    RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST;
    TIM2->PSC = 71U;              /* APB1 timer clock is 72 MHz: 1 us/count. */
    TIM2->ARR = 0xFFFFU;
    TIM2->CCMR1 = TIM_CCMR1_CC2S_0;
    TIM2->CCER = 0U;
    TIM2->DIER = 0U;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0U;
    TIM2->CR1 = TIM_CR1_CEN;

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    cycles_per_us = SystemCoreClock / 1000000U;
    if (cycles_per_us == 0U) return false;

    NVIC_SetPriority(TIM2_IRQn, 2U);
    NVIC_ClearPendingIRQ(TIM2_IRQn);
    NVIC_EnableIRQ(TIM2_IRQn);
    capture_state = CAPTURE_IDLE;
    port_initialized = 1U;
    return true;
}

static void port_deinit(void *context)
{
    (void)context;
    if (port_initialized == 0U) return;

    NVIC_DisableIRQ(TIM2_IRQn);
    TIM2->DIER = 0U;
    TIM2->CCER = 0U;
    TIM2->CR1 = 0U;
    RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST;
    RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN;

    GPIOA->CRH = (GPIOA->CRH & ~(0xFU << 28)) | (0x4U << 28);
    GPIOB->CRL = (GPIOB->CRL & ~(0xFU << 12)) | (0x4U << 12);
    capture_state = CAPTURE_IDLE;
    port_initialized = 0U;
}

static bool port_start(void *context)
{
    uint32_t pulse_start;
    uint32_t guard;
    (void)context;
    if (port_initialized == 0U || capture_state == CAPTURE_WAIT_RISE ||
        capture_state == CAPTURE_WAIT_FALL) {
        return false;
    }

    TIM2->DIER = 0U;
    TIM2->CCER &= ~TIM_CCER_CC2E;
    TIM2->CNT = 0U;
    TIM2->SR = 0U;
    TIM2->CCER &= ~TIM_CCER_CC2P;
    capture_rise = 0U;
    capture_pulse_us = 0U;
    capture_state = CAPTURE_WAIT_RISE;
    measurement_start_cycles = DWT->CYCCNT;
    TIM2->CCER |= TIM_CCER_CC2E;
    TIM2->DIER = TIM_DIER_CC2IE;

    GPIOA->BSRR = CS100A_TRIG_PIN;
    pulse_start = DWT->CYCCNT;
    guard = SystemCoreClock / 1000U;
    while ((DWT->CYCCNT - pulse_start) <
           cycles_per_us * CS100A_TRIGGER_PULSE_US && guard > 0U) {
        guard--;
    }
    GPIOA->BRR = CS100A_TRIG_PIN;
    if (guard == 0U) {
        TIM2->DIER = 0U;
        TIM2->CCER &= ~TIM_CCER_CC2E;
        capture_state = CAPTURE_ERROR;
        return false;
    }
    return true;
}

static CS100A_IO_Status port_poll(void *context, uint32_t *pulse_us)
{
    CaptureState state;
    (void)context;
    if (port_initialized == 0U || pulse_us == 0) return CS100A_IO_ERROR;

    state = capture_state;
    if (state == CAPTURE_READY) {
        *pulse_us = capture_pulse_us;
        capture_state = CAPTURE_IDLE;
        return CS100A_IO_READY;
    }
    if (state == CAPTURE_ERROR) {
        capture_state = CAPTURE_IDLE;
        return CS100A_IO_ERROR;
    }
    if ((state == CAPTURE_WAIT_RISE || state == CAPTURE_WAIT_FALL) &&
        (DWT->CYCCNT - measurement_start_cycles) >=
        cycles_per_us * CS100A_ECHO_TIMEOUT_US) {
        TIM2->DIER = 0U;
        TIM2->CCER &= ~TIM_CCER_CC2E;
        capture_state = CAPTURE_IDLE;
        return CS100A_IO_TIMEOUT;
    }
    return CS100A_IO_BUSY;
}

static const CS100A_Port cs100a_port = {
    0,
    port_init,
    port_deinit,
    port_start,
    port_poll
};

const CS100A_Port *CS100A_STM32F103_GetPort(void)
{
    return &cs100a_port;
}

#ifdef __cplusplus
extern "C"
#endif
void TIM2_IRQHandler(void)
{
    uint16_t capture;
    uint32_t status = TIM2->SR;

    if ((status & TIM_SR_CC2OF) != 0U) {
        TIM2->SR &= ~(TIM_SR_CC2OF | TIM_SR_CC2IF);
        TIM2->DIER = 0U;
        TIM2->CCER &= ~TIM_CCER_CC2E;
        capture_state = CAPTURE_ERROR;
        return;
    }
    if ((status & TIM_SR_CC2IF) == 0U) return;

    capture = (uint16_t)TIM2->CCR2;
    TIM2->SR &= ~TIM_SR_CC2IF;
    if (capture_state == CAPTURE_WAIT_RISE) {
        capture_rise = capture;
        TIM2->CCER |= TIM_CCER_CC2P;
        capture_state = CAPTURE_WAIT_FALL;
    } else if (capture_state == CAPTURE_WAIT_FALL) {
        capture_pulse_us = (uint16_t)(capture - capture_rise);
        TIM2->DIER = 0U;
        TIM2->CCER &= ~TIM_CCER_CC2E;
        capture_state = CAPTURE_READY;
    }
}
