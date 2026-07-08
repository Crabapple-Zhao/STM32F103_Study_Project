/**
 * @file    bsp_init.c — 集中初始化: 时钟 + LED + USART + TFT
 */
#include "bsp_init.h"
#include "led.h"
#include "usart.h"
#include "lcd.h"

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
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
    USART1_Init(115200);
    uart_puts("\r\n=== STM32F103 HAL v0.0.1 ===\r\n");
    LCD_Init();
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void DMA1_Channel3_IRQHandler(void)
{
    /* register-level DMA, no HAL callback needed */
}
