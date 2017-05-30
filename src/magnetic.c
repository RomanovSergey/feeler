/*
 * magnetic.c
 *
 *  Created on: May 31, 2016
 *      Author: se
 */

#include "stm32f0xx.h"
#include "main.h"
#include "displayDrv.h"
#include "magnetic.h"
#include "sound.h"

static int measureDone = 0;
static uint16_t irq_freq = 0;
static uint16_t freq = 0;
static int magstat = 0; // magnetic status: 0-off; 1-on.

//===========================================================================
//===========================================================================
//для кругового буфера событий
#define MG_LEN_BITS   4
#define MG_LEN_BUF    (1<<MG_LEN_BITS) // 8 или 2^3 или (1<<3)
#define MG_LEN_MASK   (MG_LEN_BUF-1)   // bits: 0000 0111
static uint8_t mgbufEv[MG_LEN_BUF] = {0};
static uint8_t mgtail = 0;
static uint8_t mghead = 0;
/*
 * возвращает 1 если в кольцевом буфере есть свободное место для элемента, иначе 0
 */
static int mgHasFree(void) {
	if ( ((mghead + 1) & MG_LEN_MASK) == mgtail ) {
		return 0;//свободного места нет
	}
	return 1;//есть свободное место
}
/*
 * помещает событие в круговой буфер
 * return 1 - успешно; 0 - нет места в буфере
 */
int mgPutEv(uint8_t event) { // public
	if (event == 0) {
		return 1;//событие с нулевым кодом пусть не будет для удобства
	}
	if ( mgHasFree() ) {
		mgbufEv[mghead] = event;
		mghead = (1 + mghead) & MG_LEN_MASK;//инкремент кругового индекса
		return 1;
	} else {
		return 0;//нет места в буфере
	}
}
/*
 *  извлекает событие из кругового буфера
 *  если 0 - нет событий
 */
uint8_t mgGetEv(void) { // private
	uint8_t event = 0;
	if (mghead != mgtail) {//если в буфере есть данные
		event = mgbufEv[mgtail];
		mgtail = (1 + mgtail) & MG_LEN_MASK;//инкремент кругового индекса
	}
	return event;
}
//===========================================================================
//===========================================================================


/*
 * interrupt handler
 */
void TIM2_IRQHandler(void) {
	if ( SET == TIM_GetITStatus(TIM2, TIM_IT_Update) ) {
		irq_freq = (uint16_t)TIM_GetCounter( TIM3 );
		TIM_SetCounter(TIM3, 0);
		measureDone = 1; // флаг - данные измерения готовы
		TIM_ClearFlag(TIM2, TIM_FLAG_Update);
	}
}

/*
 * выдает текущую частоту
 */
uint16_t getFreq(void) {
	return freq;
}

/*
 * magGetStat() получить состояние магнита
 * Return: 0 - magnit is off; 1 - magnit is on.
 */
int magGetStat(void) {
	return magstat;
}

/*
 * Вызывается из main()
 * для синхронизации переменных (частоты и генерации события)
 * т.к. программа однопоточная, а measureDone устанавливается
 * гораздо реже основного цикла программы
 */
void magnetic(void)
{
	uint8_t event;
	if (measureDone == 1) { //для синхронизации с прерыванием
		measureDone = 0;
		freq = irq_freq;
		dispPutEv( DIS_MEASURE ); //событие - данные измерения готовы
	}
	event = mgGetEv(); // заберем событие из кругового буфера событий магнита
	switch ( event ) {
	case MG_ON: // включим магнит
		magstat = 1;
		TIM_ClearFlag(TIM2, TIM_FLAG_Update);
		TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

		GPIOA->MODER  &= ~(GPIO_MODER_MODER0 << (6 * 2)); // reset PA6 MODER bits
		GPIOA->MODER |= (((uint32_t)GPIO_Mode_AF) << (6 * 2)); // set PA6 as AF

		TIM_SetCounter(TIM3, 0);
		TIM_Cmd(TIM3, ENABLE);
		TIM_SetCounter(TIM2, 0);
		TIM_Cmd(TIM2, ENABLE);
		dispPutEv( DIS_MEASURE );
		freq = 0;
		break;
	case MG_OFF: // выключим магнит
		magstat = 0;
		GPIOA->MODER  &= ~(GPIO_MODER_MODER0 << (6 * 2)); // reset PA6 MODER bits
		GPIOA->MODER |= (((uint32_t)GPIO_Mode_OUT) << (6 * 2)); // set PA6 as GPIO
		GPIOA->BRR = GPIO_Pin_6; // PA6 to low
		//GPIO_ResetBits(GPIOA, GPIO_Pin_6);

		TIM_Cmd(TIM3, DISABLE);
		TIM_Cmd(TIM2, DISABLE);
		TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);

		irq_freq = 0;
		dispPutEv( DIS_MEASURE );
		break;
	}
}


