/*
 * leds.c
 *
 *  Created on: 30 апр. 2016 г.
 *      Author: se
 */

#include "stm32f0xx.h"

void leds(void) {
	static uint16_t tim = 0;

	tim++;
	if (tim == 1990) {
		GPIO_SetBits( GPIOC, GPIO_Pin_9 );
		TIM_SetCounter(TIM2, 0);
		TIM_Cmd(TIM2, ENABLE);
	} else if (tim == 2000) {
		GPIO_ResetBits( GPIOC, GPIO_Pin_9 );
		tim = 1;
	}
}
