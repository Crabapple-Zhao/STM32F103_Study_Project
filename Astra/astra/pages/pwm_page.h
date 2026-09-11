/**
 * @file    pwm_page.h
 * @brief   PWM output page (TIM3_CH4 @ PB1) for Astra UI.
 */
#ifndef __PWM_PAGE_H
#define __PWM_PAGE_H

namespace astra {
class Menu;
}

void PwmPage_AddToMenu(astra::Menu *menu);

#endif
