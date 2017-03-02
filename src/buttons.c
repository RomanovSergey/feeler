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
#include "buttons.h"

pBut_t pButton = butNo;

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
	if ( instance == 0 ) { //if button is pushed ************************
		if ( b->debcount < ANTI_TIME ) {
			b->debcount++;//filter
		} else {
			b->current = 1;
		}
	} else { //if button is pulled **************************************
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
 *   *Long - событие длительного нажатия
 *   *pull - событие отпускания кнопки
 * Return:
 *   1 - есть новое событие
 *   0 - нет новых событий
 */
int buttonEv(button_t *b, int *push, int *Long, int *pull)
{
	static const int LONGPUSH = 1000; //ms время для генер. события длит. нажатия
	*push = 0;
	*Long = 0;
	*pull = 0;

	switch ( b->state ) {
	case 0: // ждем первое нажатие кнопки
		if ( b->current == 1 ) { //первое нажатие кнопки
			b->state = 1;
			b->tim = 0;
			*push = 1;
			return 1;
		}
		return 0;
	case 1: // ждем первое отпускание кнопки или длительное нажатие
		if ( b->current == 0 ) { //первое отпускание кнопки
			b->state = 0;
			b->tim = 0;
			*pull = 1;
			return 1;
		}
		if ( b->tim == LONGPUSH ) { //длительное нажатие кнопки
			b->tim = 0;
			b->state = 2;
			*Long = 1;
			return 1;
		}
		break;
	case 2: // здесь состояние дилтельного нажатия, ждем отпускания
		if ( b->current == 0 ) { //первое отпускание кнопки
			b->state = 0;
			b->tim = 0;
			*pull = 1;
			return 1;
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
 * ждем pwr.c пока включится мнимое питание
 */
void butNo(void) {
	return;
}

/*
 * Ждем, пока все кнопку влючения питания отпустят
 */
void butWait(void) {
	if ( B_OK.current != 0 ) {
		return;
	}
	pButton = butProcess;
}

void butProcess(void)
{
	int pushEv;
	int LongEv;
	int pullEv;
	int wasEvent = 0; // для генер. событ. при длит. отсутствий нажатий

	static uint32_t timer_ms = 0;
	static const uint32_t LEFT_TIME_MS = 20000UL;

	if ( buttonEv( &B_OK, &pushEv, &LongEv, &pullEv ) ) {
		wasEvent = 1;
		if ( pushEv ) {
			dispPutEv( DIS_PUSH_OK );
		} else if ( LongEv ) {
			pwrPutEv( PWR_POWEROFF );
		} else if ( pullEv ) {
			dispPutEv( DIS_PULL_OK );
		}
	}

	if ( buttonEv( &B_R, &pushEv, &LongEv, &pullEv ) ) {
		wasEvent = 1;
		if ( pushEv ) {
			dispPutEv( DIS_PUSH_R );
		} else if ( LongEv ) {
			dispPutEv( DIS_LONGPUSH_R );
		}
	}

	if ( buttonEv( &B_L, &pushEv, &LongEv, &pullEv ) ) {
		wasEvent = 1;
		if ( pushEv ) {
			dispPutEv( DIS_PUSH_L );
		} else if ( LongEv ) {
			dispPutEv( DIS_LONGPUSH_L );
		} else if ( pullEv ) {
			dispPutEv( DIS_PULL_L );
		}
	}

	// если длительно кнопки не нажимать, то выключим питание
	if ( wasEvent ) {
		timer_ms = 0;
	} else {
		if ( timer_ms == LEFT_TIME_MS ) {
			timer_ms = 0;
			pwrPutEv( PWR_OVERTIME );
		} else {
			timer_ms++;
		}
	}
}

/*
 * this function called from main loop every 1 ms
 */
void buttons(void)
{
	debounce( &B_OK, READ_B1 );
	debounce( &B_R, READ_B2 );
	debounce( &B_L, READ_B3 );

	pButton(); //указатель на функцию
}


