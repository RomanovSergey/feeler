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

		//TIM_Cmd(TIM2, ENABLE);
		//while (count == g.ADC_count);

		//ADC_StartOfConversion(ADC1);
		//while ( RESET == ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) );
		//g.ADC_value = ADC_GetConversionValue(ADC1);
	} else if (tim == 1000) {
		MAGNETIC_ON;//включаем сигнал
	} else if (tim == 1500) {

		//TIM_Cmd(TIM2, ENABLE);

		//ADC_StartOfConversion(ADC1);
		//while ( RESET == ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) );
		//g.ADC_value = ADC_GetConversionValue(ADC1);
	} else if (tim == 2000) {
		tim = 1;
		MAGNETIC_OFF;//выключаем сигнал
	}
}

void ADC1_COMP_IRQHandler(void) {
	if ( SET == ADC_GetITStatus(ADC1, ADC_IT_EOC) ) {
		TIM_ClearFlag(TIM2, TIM_FLAG_Update);
		g.ADC_count++;
		g.ADC_value = ADC_GetConversionValue(ADC1);
		ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
	}
}

void TIM2_IRQHandler(void) {
	if (SET == TIM_GetITStatus(TIM2, TIM_IT_Update) ) {
		static int stat = 0;

		if (stat == 0) {
			GPIO_SetBits( GPIOC, GPIO_Pin_8 );
			stat = 1;
		} else {
			stat = 0;
			GPIO_ResetBits( GPIOC, GPIO_Pin_8 );
		}
		TIM_ClearFlag(TIM2, TIM_FLAG_Update);
	}
}

