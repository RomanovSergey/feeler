/*
 * adc.c
 *
 *  Created on: 3 мая 2016 г.
 *      Author: se
 */

#include "stm32f0xx.h"
#include "main.h"

void adc(void) {
	static uint16_t tim = 0;

	tim++;
	if (tim == 1) {
		GREEN_ON;
	} else if (tim == 20) {
		GREEN_OFF;
	} else if (tim == 999) {
		tim = 0;
	}
}

void TIM2_IRQHandler(void) {
	if ( SET == TIM_GetITStatus(TIM2, TIM_IT_CC1) ) {
		MAGNETIC_ON;
		TIM_ClearFlag(TIM2, TIM_FLAG_CC1);

		while ( RESET == TIM_GetFlagStatus(TIM2, TIM_FLAG_CC2) );
		TIM_ClearFlag(TIM2, TIM_FLAG_CC2);

		ADC_StartOfConversion(ADC1);
		while ( RESET == ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) );
		ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
		g.ADC_value += ADC_GetConversionValue(ADC1);
		MAGNETIC_OFF;

		while ( RESET == TIM_GetFlagStatus(TIM2, TIM_FLAG_CC3) );
		TIM_ClearFlag(TIM2, TIM_FLAG_CC3);

		if (++g.ADC_count == 1000) {
			g.ADC_done = 1;

			g.ADC_count = 0;
			g.ADC_done  = 0;
			g.ADC_value = 0;
			//TIM_Cmd(TIM2, DISABLE);
			//TIM_SetCounter(TIM2, 0);
		}

		//TIM_Cmd(TIM2, DISABLE);
		TIM_SetCounter(TIM2, 0);
	}
}


/*void ADC1_COMP_IRQHandler(void) {
	if ( SET == ADC_GetITStatus(ADC1, ADC_IT_EOC) ) {
		TIM_ClearFlag(TIM2, TIM_FLAG_Update);
		g.ADC_count++;
		g.ADC_value = ADC_GetConversionValue(ADC1);
		ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
	}
}*/
