/*
 * sound.c
 *
 *  Created on: 11 февр. 2017 г.
 *      Author: se
 */

#include "stm32f0xx.h"
#include "main.h"
#include "sound.h"

//===========================================================================
//===========================================================================
//для кругового буфера событий
#define SND_LEN_BITS   4
#define SND_LEN_BUF    (1<<SND_LEN_BITS) // 8 или 2^3 или (1<<3)
#define SND_LEN_MASK   (SND_LEN_BUF-1)   // bits: 0000 0111
static uint8_t bufEv[SND_LEN_BUF] = {0};
static uint8_t tail = 0;
static uint8_t head = 0;
/*
 * возвращает 1 если в кольцевом буфере есть свободное место для элемента, иначе 0
 */
static int sndHasFree(void) {
	if ( ((tail + 1) & SND_LEN_MASK) == head ) {
		return 0;//свободного места нет
	}
	return 1;//есть свободное место
}
/*
 * помещает событие в круговой буфер
 * return 1 - успешно; 0 - нет места в буфере
 */
int sndPutEv(uint8_t event) {
	if (event == 0) {
		return 1;//событие с нулевым кодом пусть не будет для удобства
	}
	if ( sndHasFree() ) {
		bufEv[head] = event;
		head = (1 + head) & SND_LEN_MASK;//инкремент кругового индекса
		return 1;
	} else {
		return 0;//нет места в буфере
	}
}
/*
 *  извлекает событие из кругового буфера
 *  если 0 - нет событий
 */
uint8_t sndGetEv(void) {
	uint8_t event = 0;
	if (head != tail) {//если в буфере есть данные
		event = bufEv[tail];
		tail = (1 + tail) & SND_LEN_MASK;//инкремент кругового индекса
	}
	return event;
}
//===========================================================================
//===========================================================================

typedef void (*pSndFun)(void);
static pSndFun pSnd = NULL;

static uint32_t mstime = 0;

void sndBeepState(void) {
	if ( mstime == 0 ) {
		TIM_SetCounter( TIM17, 0);
		TIM_SetAutoreload( TIM17, 1000 );
		TIM_SetCompare1( TIM17, 500 );
		TIM_CtrlPWMOutputs( TIM17, ENABLE );
		TIM_Cmd( TIM17, ENABLE );
	}
	mstime++;
	if ( mstime == 50 ) {
		TIM_CtrlPWMOutputs( TIM17, DISABLE );
		TIM_Cmd( TIM17, DISABLE );
		BEEP_OFF;
		pSnd = NULL;
	}
}

/*
 * вызывается из main раз в 1 мс
 */
void sound(void) {

	uint8_t event = 0;
	event = sndGetEv();

	if ( event == 1 ) {
		mstime = 0;
		pSnd = sndBeepState;
	}

	if ( pSnd == NULL ) {
		return;
	}
	pSnd();
}
