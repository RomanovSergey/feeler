/*
 * magnetic.c
 *
 *  Created on: May 31, 2016
 *      Author: se
 */

#include "stm32f0xx.h"
#include "main.h"

static int measureDone = 0;

void magnetic(void) {
	if (measureDone == 1) {//для синхронизации с прерыванием
		measureDone = 0;
		put_event( Emeasure );//событие - данные измерения готовы
	}
}

/*
 * interrupt handler
 */
void TIM2_IRQHandler(void) {
	if ( SET == TIM_GetITStatus(TIM2, TIM_IT_Update) ) {
		static int ledstat = 0;
		if (ledstat) {
			ledstat = 0;
			GREEN_ON;
		} else {
			ledstat = 1;
			GREEN_OFF;
		}
		g.tim_len = TIM_GetCounter( TIM3 );
		TIM_SetCounter(TIM3, 0);
		measureDone = 1;//флаг - данные измерения готовы

		TIM_ClearFlag(TIM2, TIM_FLAG_Update);
	}
}



