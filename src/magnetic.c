/*
 * magnetic.c
 *
 *  Created on: May 31, 2016
 *      Author: se
 */

#include "stm32f0xx.h"
#include "main.h"
#include "displayDrv.h"

static int measureDone = 0;
static uint32_t irq_freq = 0;
static uint32_t freq = 0;

/*
 * Вызывается из main()
 * для синхронизации переменных (частоты и генерации события)
 * т.к. программа однопоточная, а measureDone устанавливается
 * гораздо реже основного цикла программы
 */
void magnetic(void) {
	if (measureDone == 1) {//для синхронизации с прерыванием
		measureDone = 0;
		freq = irq_freq;
		//put_event( Emeasure );//событие - данные измерения готовы
		dispPutEv( DIS_MEASURE );//событие - данные измерения готовы
	}
}

/*
 * выдает текущую частоту
 */
uint32_t getFreq(void) {
	return freq;
}

/*
 * interrupt handler
 */
void TIM2_IRQHandler(void) {
	if ( SET == TIM_GetITStatus(TIM2, TIM_IT_Update) ) {
//		static int ledstat = 0;
//		if (ledstat) {
//			ledstat = 0;
//			GREEN_ON;
//		} else {
//			ledstat = 1;
//			GREEN_OFF;
//		}
		irq_freq = TIM_GetCounter( TIM3 );
		TIM_SetCounter(TIM3, 0);
		measureDone = 1;//флаг - данные измерения готовы

		TIM_ClearFlag(TIM2, TIM_FLAG_Update);
	}
}



