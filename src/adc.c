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

#define CMP1          1   //всегда для регистра сравнения 1
#define CMP2START     50  //стартовая величина для регистра сравнения 2
#define CMPDEC        150 //наибольшая постоянная для спада м.поля
//#define SMPTIMES      16  //количество выборок для устреднения
#define TARGET        3000

typedef struct {
	uint16_t  cmp1;//для регистра сравнения 1
	uint16_t  cmp2;//ну пока для регистра сравнения 2
	uint16_t  cmp3;//соответсвенно регистр сравнения 3
	uint32_t  target;//задание к чему ADC_value должен стремиться
	uint16_t  samples;//cтепень числа 2 количества выборок для усреднения
	uint16_t  timprot;//защита от бесконечного цикла измерения
} ADCstruct_t;

int8_t debug(char *str);
//********* здесь начинается смысл сущестования программы **************************

static ADCstruct_t ad = {
	.cmp1    = CMP1,
	.cmp2    = CMP2START,
	.cmp3    = CMP2START + CMPDEC,
	.target  = TARGET,
	.samples = 4,// 2^4 = 16 выборок
	.timprot = 0,
};

void adc(void) {
	static uint16_t tim = 0;

	if ( g.b1_push == 1 ) {//если нажали кнопку Б1 - запустим измерительный механизм
		g.b1_push = 0;//сбросим событие нажатия Б1

		g.ADC_value = 0;
		ad.cmp1 = CMP1;
		ad.cmp2 = CMP2START;
		ad.cmp3 = CMP2START + CMPDEC;
		ad.target = TARGET;
		ad.timprot = 0;

		g.ind = 0;
		g.buf[0] = 0;
		debug("start\r\n");

		TIM_SetCompare1( TIM2, ad.cmp1 );//время где включится магнит
		TIM_SetCompare2( TIM2, ad.cmp2 );//где запустится АЦП а затем выключится магнит
		TIM_SetCompare3( TIM2, ad.cmp3 );//где ожидаем спада магнитного поля
		g.ADC_deltaTime = ad.cmp2 - ad.cmp1;
		//ad.state = 0;
		TIM_SetCounter(TIM2, 0);
		TIM_Cmd(TIM2, ENABLE);//запустим магнитный алгоритм
	}

	tim++;//мограуши лампочкой удалить потом надо
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
		MAGNETIC_OFF;//прекращаем накачку магнитной энергии
		//
		while ( RESET == ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) );
		ADC_ClearFlag(ADC1, ADC_FLAG_EOC);//преобразование АЦП завершено
		//
		while ( RESET == TIM_GetFlagStatus(TIM2, TIM_FLAG_CC3) );
		TIM_ClearFlag(TIM2, TIM_FLAG_CC3);//магнитная энергия улетучилась в тепло
		g.ADC_value += ADC_GetConversionValue(ADC1);//here because ADC_DR register ready latyncy

}
/*
void TIM2_IRQHandler_old(void) {
	static int count = 0;

	if ( SET == TIM_GetITStatus(TIM2, TIM_IT_CC1) ) {
		TIM_ClearFlag(TIM2, TIM_FLAG_CC1);
		//
		magneticShot();//длительный процесс, но нужна временная суперточность
		count++;
		ad.timprot++;
		if ( count < (1 << ad.samples) ) {
			TIM_SetCounter(TIM2, 0);
			return;
		}
		count = 0;
		g.ADC_value = g.ADC_value >> ad.samples;

		//timeRegulator ****************************************************************************
		int32_t delta = ad.target - g.ADC_value;//если + то добавить силу поля, если - то убавить
		if ( ad.timprot > 600 ) {
			debug("\r\ntimprot");
			TIM_Cmd(TIM2, DISABLE);
			TIM_SetCounter(TIM2, 0);
			g.ADC_done = 1;
		} else if ( delta > 20 ) {
			debug("+");
			g.ADC_value = 0;
			ad.cmp2++;//добавим силу поля
			ad.cmp3 = ad.cmp2 + CMPDEC;
			TIM_SetCompare1( TIM2, ad.cmp1 );//время где включится магнит
			TIM_SetCompare2( TIM2, ad.cmp2 );//где запустится АЦП а затем выключится магнит
			TIM_SetCompare3( TIM2, ad.cmp3 );//где ожидаем спада магнитного поля
			g.ADC_deltaTime = ad.cmp2 - ad.cmp1;
			TIM_SetCounter(TIM2, 0);
		} else if ( delta < -20 ) {
			debug("-");
			g.ADC_value = 0;
			ad.cmp2--;//убавим силу поля
			ad.cmp3 = ad.cmp2 + CMPDEC;
			TIM_SetCompare1( TIM2, ad.cmp1 );//время где включится магнит
			TIM_SetCompare2( TIM2, ad.cmp2 );//где запустится АЦП а затем выключится магнит
			TIM_SetCompare3( TIM2, ad.cmp3 );//где ожидаем спада магнитного поля
			g.ADC_deltaTime = ad.cmp2 - ad.cmp1;
			TIM_SetCounter(TIM2, 0);
		} else {
			debug("\r\nOK");
			TIM_Cmd(TIM2, DISABLE);
			TIM_SetCounter(TIM2, 0);
			g.ADC_done = 1;
		}
	}
}
*/

/*
 * Копирует строку str в глобальный отладочный буфер
 * символ конца строки - 0 (не копируется)
 */
int8_t debug(char *str) {
	uint16_t i = 0;

	while (str[i] != 0) {
		if (g.ind < 256) {
			g.buf[g.ind] = (uint8_t)str[i];
			g.ind++;
			i++;
		} else {
			return -1;//buffer overflow
		}
	}
	g.buf[g.ind] = 0;
	return 0;//all ok
}
