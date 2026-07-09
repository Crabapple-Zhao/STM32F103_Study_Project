/**
 * @file    bsp_init.c - board init: clock + LED + USART1 + TFT + KEY
 */
#include "bsp_init.h"
#include "led.h"
#include "usart.h"
#include "lcd.h"
#include "key.h"
#include "encoder.h"
#include "app_config.h"

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    /* HSE: PD0/OSC_IN + PD1/OSC_OUT, 8MHz 外部晶振 → PLL×9 = 72MHz SYSCLK */
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.LSEState       = RCC_LSE_ON;
    osc.HSIState       = RCC_HSI_ON;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL     = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&osc);

    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                       | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
}

void BSP_Init(void)
{
    HAL_Init();
    SystemClock_Config();
    LED_Init();
    USART1_Init(APP_USART_BAUDRATE);
    LCD_Init();
    KEY_Init();      /* KEY1=PA0, KEY2=PC13 auxiliary inputs */
    Encoder_Init();  /* PB6/PB7 TIM4 encoder, PB5 SW button */
}

/* ---- 直接寄存器串口输出 (不依赖 HAL, 供 HardFault 诊断用) ---- */
static void dbg_usart1_init(void) {
    volatile uint32_t *rcc_cfgr     = (volatile uint32_t*)0x40021004;
    volatile uint32_t *rcc_apb2enr  = (volatile uint32_t*)0x40021018;
    volatile uint32_t *gpioa_crh    = (volatile uint32_t*)0x40010804;
    volatile uint32_t *usart1_brr   = (volatile uint32_t*)0x40013808;
    volatile uint32_t *usart1_cr1   = (volatile uint32_t*)0x4001380C;
    /* 使能 GPIOA + USART1 时钟 */
    *rcc_apb2enr |= (1<<2) | (1<<14);
    /* PA9 = AF push-pull 50MHz (CRH bits 4-7) */
    *gpioa_crh = (*gpioa_crh & ~(0xF<<4)) | (0xB<<4);
    /* 根据 SWS 位选择波特率: PLL(72MHz)->625, HSI(8MHz)->69 */
    if (((*rcc_cfgr >> 2) & 0x3) == 0x2) *usart1_brr = 625;   /* PLL 72MHz */
    else                                  *usart1_brr = 69;    /* HSI 8MHz */
    /* 使能 UE + TE */
    *usart1_cr1 = (1<<13) | (1<<3);
}
static void dbg_putc(char c) {
    volatile uint32_t *sr = (volatile uint32_t*)(0x40013800 + 0x00); /* USART1_SR */
    volatile uint32_t *dr = (volatile uint32_t*)(0x40013800 + 0x04); /* USART1_DR */
    uint32_t timeout = 1000000;
    while (!(*sr & (1<<7)) && --timeout);  /* TXE */
    *dr = (uint8_t)c;
}
static void dbg_puts(const char *s) { while (*s) dbg_putc(*s++); }
static void dbg_hex(uint32_t v) {
    dbg_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uint8_t nib = (v >> i) & 0xF;
        dbg_putc(nib < 10 ? '0' + nib : 'A' + nib - 10);
    }
}

/* HardFault 诊断: 初始化 USART1 + 输出 PC/LR + 死循环 */
#ifdef __cplusplus
extern "C"
#endif
void HardFault_Handler(void)
{
    uint32_t msp_val;
    __asm { MRS msp_val, MSP }
    uint32_t *msp = (uint32_t*)msp_val;

    uint32_t stacked_pc  = msp[6];
    uint32_t stacked_lr  = msp[5];
    uint32_t stacked_xpsr= msp[7];

    dbg_usart1_init();
    dbg_puts("\r\n[ERROR] [fault] type=HardFault\r\n");
    dbg_puts("[ERROR] [fault] pc="); dbg_hex(stacked_pc); dbg_puts("\r\n");
    dbg_puts("[ERROR] [fault] lr="); dbg_hex(stacked_lr); dbg_puts("\r\n");
    dbg_puts("[ERROR] [fault] xpsr="); dbg_hex(stacked_xpsr); dbg_puts("\r\n");

    volatile uint32_t *cfsr = (volatile uint32_t*)0xE000ED28;
    volatile uint32_t *hfsr = (volatile uint32_t*)0xE000ED2C;
    volatile uint32_t *bfar = (volatile uint32_t*)0xE000ED38;
    volatile uint32_t *mmfar= (volatile uint32_t*)0xE000ED34;
    dbg_puts("[ERROR] [fault] cfsr="); dbg_hex(*cfsr); dbg_puts("\r\n");
    dbg_puts("[ERROR] [fault] hfsr="); dbg_hex(*hfsr); dbg_puts("\r\n");
    dbg_puts("[ERROR] [fault] bfar="); dbg_hex(*bfar); dbg_puts("\r\n");
    dbg_puts("[ERROR] [fault] mmfar="); dbg_hex(*mmfar); dbg_puts("\r\n");

    while (1) {
        volatile uint32_t *bsrr = (volatile uint32_t*)(0x40010800 + 0x10);
        *bsrr = (1 << 11) | (1 << (11+16));
        for (volatile int i = 0; i < 200000; i++);
    }
}

/* 中断处理函数必须用 C 链接, 否则 C++ 名称修饰会导致
 * startup.s 中的 [WEAK] 符号无法被覆盖, SysTick 中断跳入死循环 */
#ifdef __cplusplus
extern "C"
#endif
void SysTick_Handler(void)
{
    HAL_IncTick();
}

#ifdef __cplusplus
extern "C"
#endif
void DMA1_Channel3_IRQHandler(void)
{
    /* register-level DMA, no HAL callback needed */
}
