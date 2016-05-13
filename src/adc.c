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
	//uint32_t count = g.ADC_count;

	tim++;
	if (tim == 500) {
		GREEN_ON;
		TIM_SetCounter(TIM2, 0);
		TIM_Cmd(TIM2, ENABLE);
	} else if (tim == 1000) {
		GREEN_OFF;
	} else if (tim == 1500) {

		//TIM_Cmd(TIM2, ENABLE);

		//ADC_StartOfConversion(ADC1);
		//while ( RESET == ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) );
		//g.ADC_value = ADC_GetConversionValue(ADC1);
	} else if (tim == 2000) {
		tim = 1;
		//MAGNETIC_OFF;//выключаем сигнал
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

void TIM2_IRQHandler(void) {
	if ( SET == TIM_GetITStatus(TIM2, TIM_IT_CC1) ) {
		MAGNETIC_ON;
		TIM_ClearFlag(TIM2, TIM_FLAG_CC1);

		while ( RESET == TIM_GetFlagStatus(TIM2, TIM_FLAG_CC2) );
		TIM_ClearFlag(TIM2, TIM_FLAG_CC2);

		ADC_StartOfConversion(ADC1);
		while ( RESET == ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) );
		ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
		g.ADC_value = ADC_GetConversionValue(ADC1);
		g.ADC_count++;

		MAGNETIC_OFF;
		TIM_Cmd(TIM2, DISABLE);
	}/* else if ( SET == TIM_GetITStatus(TIM2, TIM_IT_CC2) ) {

	} else if ( SET == TIM_GetITStatus(TIM2, TIM_IT_Update) ) {
		MAGNETIC_OFF;
		TIM_ClearFlag(TIM2, TIM_FLAG_Update);
		TIM_Cmd(TIM2, DISABLE);
	}*/
}

