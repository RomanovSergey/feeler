/*
 * magnetic.c
 *
 *  Created on: May 31, 2016
 *      Author: se
 */

#include "stm32f0xx.h"
#include "main.h"

void magnetic(void) {
	if ( g.b1_push == 1 ) {//если нажали кнопку Б1 - запустим измерительный механизм
		g.b1_push = 0;//сбросим событие нажатия Б1

		GREEN_ON;
		TIM_SetCounter(TIM2, 0);
		TIM_SetCounter(TIM3, 0);
		TIM_Cmd(TIM3, ENABLE);//measure magnetic field
	}
}

/*
 * interrupt handler
 */
void TIM3_IRQHandler(void) {

	if ( SET == TIM_GetITStatus(TIM3, TIM_IT_CC1) ) {
		g.tim_len = TIM_GetCounter( TIM2 );
		g.tim_done = 1;
		TIM_ClearFlag(TIM3, TIM_FLAG_CC1);
		TIM_Cmd(TIM3, DISABLE);
		TIM_SetCounter(TIM2, 0);
		GREEN_OFF;
	}
}

