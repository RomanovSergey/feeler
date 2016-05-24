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

#define CMP1   1
#define CMP2   50

typedef struct {
	uint16_t  cmp1;//для регистра сравнения 1
	uint16_t  cmp2;//ну пока для регистра сравнения 2
	uint32_t  air;//ну пока значение для воздуха (потом от калибровки)
	uint32_t  deltaAir;//ну допустим погрешность для воздуха (уточнить статиститически)
	uint16_t  state;//машинное наверное состояние
} constADCstruct_t;

static constADCstruct_t ad = {
	.cmp1 = 1,
	.cmp2 = 50,
	.air = 2020,
	.deltaAir = 200,
	.state = 0,
};

void adc(void) {
	static uint16_t tim = 0;

	if (g.B1_push == 1) {//если нажали кнопку Б1 - запустим измерительный механизм
		g.B1_push = 0;//сбросим событие нажатия Б1

		g.ADC_value = 0;
		TIM_SetCompare1(TIM2, ad.cmp1);//время где включится магнит
		TIM_SetCompare2(TIM2, ad.cmp2);//где запустится АЦП а затем выключится магнит
		TIM_SetCompare3(TIM2, ad.cmp2 + 150);//где ожидаем спада магнитного поля
		g.ADC_deltaTime = ad.cmp2 - ad.cmp1;
		ad.state = 0;
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
		while ( RESET == ADC_GetFlagStatus(ADC1, ADC_FLAG_EOSMP) );
		ADC_ClearFlag(ADC1, ADC_FLAG_EOSMP);//выборка ацп произведена
		//
		while ( RESET == ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) );
		ADC_ClearFlag(ADC1, ADC_FLAG_EOC);//преобразование АЦП завершено
		//
		MAGNETIC_OFF;//прекращаем накачку магнитной энергии
		//
		while ( RESET == TIM_GetFlagStatus(TIM2, TIM_FLAG_CC3) );
		TIM_ClearFlag(TIM2, TIM_FLAG_CC3);//магнитная энергия улетучилась в тепло
		g.ADC_value += ADC_GetConversionValue(ADC1);//here because ADC_DR register ready latyncy

}

void TIM2_IRQHandler(void) {
	static int count = 0;
	if ( SET == TIM_GetITStatus(TIM2, TIM_IT_CC1) ) {
		TIM_ClearFlag(TIM2, TIM_FLAG_CC1);
		//
		magneticShot();//длительный процесс, но нужна временная суперточность
		count++;
		if ( count > 127 ) {
			count = 0;
			g.ADC_value = g.ADC_value / 128;
			switch (ad.state) {
			case 0://уточним что мерим чтобы мерить точнее
				if ( g.ADC_value < (ad.air - ad.deltaAir) ) {//железо рядом, замерим ещё
					g.ADC_value = 0;
					TIM_SetCompare1(TIM2, ad.cmp1);//время где включится магнит
					TIM_SetCompare2(TIM2, ad.cmp2 * 2);//где запустится АЦП а затем выключится магнит
					TIM_SetCompare3(TIM2, ad.cmp2 * 2 + 300);//где ожидаем спада магнитного поля
					g.ADC_deltaTime = ad.cmp2 * 2 - ad.cmp1;
					ad.state = 10;
					TIM_SetCounter(TIM2, 0);
				} else if ( g.ADC_value > (ad.air + ad.deltaAir) ) {//алюминий рядом
					TIM_Cmd(TIM2, DISABLE);
					g.ADC_done = 1;
				} else {//рядом ничего интересного нет
					TIM_Cmd(TIM2, DISABLE);
					g.ADC_done = 1;
				}
				break;
			case 10:
				ad.state = 0;
				TIM_Cmd(TIM2, DISABLE);
				g.ADC_done = 1;
				break;
			}
		}
		TIM_SetCounter(TIM2, 0);
	}
}

