/*
 * adc.c
 *
 *  Created on: 3 мая 2016 г.
 *      Author: se
 */

#include "stm32f0xx.h"
#include "main.h"

#define MAGNETIC_ON   GPIO_SetBits(GPIOA,GPIO_Pin_8)
#define MAGNETIC_OFF  GPIO_ResetBits(GPIOA,GPIO_Pin_8)

typedef struct {
	uint32_t  state;//машинное состояние
	uint32_t  air;//значение АЦП воздуха
	uint8_t   fe_cc2;//время накачки для железа (more)
	uint8_t   al_cc2;//время накачки для алюминия (less)
	uint8_t   cc4;//
} magnetic_t;

void adc(void) {
	static uint16_t tim = 0;

	if (g.B1_push == 1) {//если нажали кнопку Б1
		g.B1_push = 0;//сбросим событие нажатия Б1

		g.ADC_value = 0;

		TIM_SetCompare1(TIM2, 2);//время где включится магнит
		TIM_SetCompare2(TIM2, 50);//где запустится АЦП а затем выключится магнит
		TIM_SetCompare3(TIM2, 200);//ожидаем спада магнитного поля

		TIM_SetCounter(TIM2, 0);
		TIM_Cmd(TIM2, ENABLE);//запустим магнитный алгоритм
	}

	tim++;
	if (tim == 1) {
		GREEN_ON;
	} else if (tim == 20) {
		GREEN_OFF;
	} else if (tim == 999) {
		tim = 0;
	}
}

void magneticShot(void) {
	MAGNETIC_ON;//накачка магнитной энергии
	//
	while ( RESET == TIM_GetFlagStatus(TIM2, TIM_FLAG_CC2) );
	TIM_ClearFlag(TIM2, TIM_FLAG_CC2);//время накачки вышло
	//
	ADC_StartOfConversion(ADC1);//пора мерить силу тока катушки
	//
	PWR_EnterSleepMode(PWR_SLEEPEntry_WFE);
	//
	while ( RESET == ADC_GetFlagStatus(ADC1, ADC_FLAG_EOSMP) );
	ADC_ClearFlag(ADC1, ADC_FLAG_EOSMP);//выборка ацп произведена
	//
	//PWR_EnterSleepMode(PWR_SLEEPEntry_WFE);
	//
	while ( RESET == ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) );
	ADC_ClearFlag(ADC1, ADC_FLAG_EOC);//преобразование АЦП завершено
	//
	MAGNETIC_OFF;//прекращаем накачку магнитной энергии
	//
	while ( RESET == TIM_GetFlagStatus(TIM2, TIM_FLAG_CC3) );
	TIM_ClearFlag(TIM2, TIM_FLAG_CC3);//магнитная энергия улетучилась в тепло
	g.ADC_value = ADC_GetConversionValue(ADC1);//here because ADC_DR register ready latyncy
}

void TIM2_IRQHandler(void) {
	if ( SET == TIM_GetITStatus(TIM2, TIM_IT_CC1) ) {
		TIM_ClearFlag(TIM2, TIM_FLAG_CC1);
		//
		magneticShot();//длительный процесс, но нужна временная суперточность
		//
		TIM_Cmd(TIM2, DISABLE);
		g.ADC_done = 1;
	}
}

