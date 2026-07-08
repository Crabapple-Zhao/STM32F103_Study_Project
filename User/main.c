/**
 * @file    main.c — HAL 入口
 */
#include "main.h"

void SysTick_Handler(void)           { HAL_IncTick(); }
void DMA1_Channel3_IRQHandler(void)  { /* register-level DMA */ }

static void Demo_Color(void)
{
    uart_puts("[TFT] Color demo...\r\n");
    LCD_Fill(0, 0, 127, 159, COLOR_RED);    HAL_Delay(800);
    LCD_Fill(0, 0, 127, 159, COLOR_GREEN);  HAL_Delay(800);
    LCD_Fill(0, 0, 127, 159, COLOR_BLUE);   HAL_Delay(800);
    LCD_Fill(0,   0,  127, 31,  COLOR_RED);
    LCD_Fill(0,  32,  127, 63,  COLOR_GREEN);
    LCD_Fill(0,  64,  127, 95,  COLOR_BLUE);
    LCD_Fill(0,  96,  127, 127, COLOR_YELLOW);
    LCD_Fill(0, 128,  127, 159, COLOR_MAGENTA);
    HAL_Delay(1500);
}

static void Demo_Info(void)
{
    uart_puts("[TFT] Info demo...\r\n");
    LCD_Fill(0, 0, 127, 159, COLOR_BLACK);
    LCD_ShowString(0, 0,   "STM32F103C8T6",  COLOR_WHITE,  COLOR_BLACK, 16);
    LCD_ShowString(0, 18,  "Dev-Beta v0.0.1",  COLOR_GREEN,  COLOR_BLACK, 12);
    LCD_Fill(0, 34, 127, 36, COLOR_BLUE);
    LCD_ShowString(0, 42,  "SYSCLK: 72MHz",  COLOR_WHITE,  COLOR_BLACK, 12);
    LCD_ShowString(0, 56,  "USART1: COM32",  COLOR_WHITE,  COLOR_BLACK, 12);
    LCD_ShowString(0, 70,  "TFT: ST7735S",   COLOR_WHITE,  COLOR_BLACK, 12);
    LCD_Fill(0, 88, 127, 90, COLOR_BLUE);
    LCD_ShowString(0, 96,  "12px Font",      COLOR_YELLOW, COLOR_BLACK, 12);
    LCD_ShowString(0, 112, "16px Font",      COLOR_CYAN,   COLOR_BLACK, 16);
    LCD_ShowString(0, 130, "24px",           COLOR_RED,    COLOR_BLACK, 24);
}

int main(void)
{
    BSP_Init();

    Demo_Color();
    Demo_Info();
    uart_puts("[OK]\r\n");

    uint32_t cnt = 0;
    while (1)
    {
        LED_Toggle();
        HAL_Delay(1000);
        uart_puts("[");
        uart_puts_hex(++cnt);
        uart_puts("]\r\n");
    }
}
