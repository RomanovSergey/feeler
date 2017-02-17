/*
 * buttons.c
 *
 *  Created on: апр. 2016 г.
 *      Author: Se
 */

#include "stm32f0xx.h"
#include "main.h"
#include "sound.h"
#include "displayDrv.h"
#include "pwr.h"

#define READ_B1     GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_8)
#define READ_B2     GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_5)
#define READ_B3     GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_7)

//локальная структура состояния кнопки
typedef struct  {
	int16_t  debcount;//счетчик антидребезга
	uint8_t  current;//текущее отфильтрованное состояние кнопки
	uint8_t  state;//машинное состояние (для отличия клика от дабле клика и лонг пуша)
	uint32_t tim;//временная метка клика(ов)
} button_t;

//под каждую кнопку свой объект
button_t B_OK = {
	.debcount = 0,
	.current = 0,
	.state = 0,
	.tim = 0,
};
button_t B_R = {
	.debcount = 0,
	.current = 0,
	.state = 0,
	.tim = 0,
};
button_t B_L = {
	.debcount = 0,
	.current = 0,
	.state = 0,
	.tim = 0,
};
//================================================================================

/*
 * debounce локальная функция антидребезга
 * параметры: указатель на струкуру данных кнопки button_t *b
 * и текущее значение кнопки uint8_t instnce
 *
 * результат работы функции - вычислить текущее логическое
 * состояние кнопки, которое записывается в поле передаваемой
 * структуры b->current
 */
void debounce(button_t *b, uint8_t instance) {
	static const int ANTI_TIME = 20;    //time of debounce
	if ( instance == 0 ) {//if button is pushed ************************
		if ( b->debcount < ANTI_TIME ) {
			b->debcount++;//filter
		} else {
			b->current = 1;
		}
	} else {//if button is pulled **************************************
		if ( b->debcount > 0 ) {
			b->debcount--;//filter
		} else {
			b->current = 0;
		}
	}
}

/*
 * buttonEv() устанавливает флаг при возниковения событий
 * чтобы вызывающий софт генерировал нужные события нужным устрйствам
 * Params:
 *   *b - указатель на структуру кнопки
 *   *push - событие нажатия
 *   *Lpush - событие длительного нажатия
 * Return:
 *   1 - есть новое событие
 *   0 - нет новых событий
 */
int buttonEv(button_t *b, int *push, int *Lpush) {
	static const int LONGPUSH = 2000; //ms время для генер. события длит. нажатия
	*push = 0;
	*Lpush = 0;
	switch ( b->state ) {
	case 0:
		if ( b->current == 1 ) { //первое нажатие кнопки
			b->state = 1;
			b->tim = 0;
			*push = 1;
			return 1;
		}
		break;
	case 1:
		if ( b->current == 0 ) { //первое отпускание кнопки
			b->state = 0;
			b->tim = 0;
		}
		if ( b->tim == LONGPUSH ) {
			b->tim++;
			*Lpush = 1;
			return 1;
		} else if ( b->tim > LONGPUSH ) {
			//таймеру нечего больше считать
			return 0;
		}
		break;
	default:
		b->state = 0;
		b->tim = 0;
	}
	b->tim++;
	return 0;
}

/*
 * this function called from main loop every 1 ms
 */
void buttons(void) {
	static uint32_t timer_ms = 0;
	static const uint32_t LEFT_TIME_MS = 10000UL;
	int wasEvent = 0; // для генер. событ. при длит. отсутствий нажатий
	int pushEv, LpushEv;

	debounce( &B_OK, READ_B1 );
	debounce( &B_R, READ_B2 );
	debounce( &B_L, READ_B3 );

	if ( buttonEv( &B_OK, &pushEv, &LpushEv ) ) {
		wasEvent = 1;
		if ( pushEv ) {
			dispPutEv( DIS_PUSH_OK );
			sndPutEv( SND_BEEP );
		}
		if ( LpushEv ) {
			dispPutEv( DIS_LONGPUSH_OK );
		}
	}

	if ( buttonEv( &B_R, &pushEv, &LpushEv ) ) {
		wasEvent = 1;
		if ( pushEv ) {
			dispPutEv( DIS_PUSH_R );
			sndPutEv( SND_BEEP );
		}
		if ( LpushEv ) {
			dispPutEv( DIS_LONGPUSH_R );
		}
	}

	if ( buttonEv( &B_L, &pushEv, &LpushEv ) ) {
		wasEvent = 1;
		if ( pushEv ) {
			dispPutEv( DIS_PUSH_L );
			sndPutEv( SND_BEEP );
		}
		if ( LpushEv ) {
			dispPutEv( DIS_LONGPUSH_L );
		}
	}
	// отслеживаем длительность простоя кнопок
	if ( wasEvent ) {
		timer_ms = 0;
	} else {
		if ( timer_ms == LEFT_TIME_MS ) {
			timer_ms++;
			//to generate event
			pwrPutEv( PWR_POWEROFF );
		} else {
			timer_ms++;
		}
	}
}


