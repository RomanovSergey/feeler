/*
 * buttons.c
 *
 *  Created on: 30 апр. 2016 г.
 *      Author: Se
 */

#include "stm32f0xx.h"
#include "main.h"

#define READ_B1     GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_8)

//локальная структура состояния кнопки
typedef struct  {
	int16_t  debcount;//счетчик антидребезга
	uint8_t  current;//текущее отфильтрованное состояние кнопки
	uint8_t  prev;//предыдущее отфильтрованное состояние кнопки
	uint16_t timPush;//время нажатия кнопки (для события длительного нажатия)
} button_t;

//only one button yet
button_t B1 = {
	.debcount = 0,
	.current = 0,
	.timPush = 0,
	.prev = 0,
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
	static const int ANTI_TIME = 30;    //time of debounce
	if ( instance > 0 ) {//if button is pushed *************************
		if ( b->debcount < ANTI_TIME ) {
			b->debcount++;//filter
		} else {
			b->current = 1;
		}
	} else {//if button is pulled **************************************
		if ( b->debcount > 0 ) {
			b->debcount--;//filter
		} else {
			if ( b->prev == 1 ) {//if state is change
				b->current = 0;
			}
		}
	}
}

/*
 * eventPushPull локальная функция
 * генерить событие при первом нажатии кнопки
 */
void eventPushPull(button_t *b) {
	if ( b->current == 1 ) {//если кнопка нажата
		if ( b->prev == 0 ) {//если кнопку только нажали
			b->prev = 1;
			g.event = b1Push;//сгенерим глоб.событ. нажат. кнопки (сбрасывает обработчик)
		}
	} else {//если кнопка отпущена
		if ( b->prev == 1 ) {//если конопку только что отпустили
			b->prev = 0;
			//пока нет задачи генерить событие по отпусканию кнопки
		}
	}
}

/*
 * longPush локальная функция
 * генерить событие при длительном нажатии кнопки
 */
void eventLongPush(button_t *b) {
	static const int LPUSH_TIME = 2000;  //time for long push button event generation
	if ( b->current == 1 ) {//если кнопка нажата
		if ( b->timPush > LPUSH_TIME ) {//если время нажатия кнопки достаточное
			g.event = b1LongPush;//сгенерим глоб.событ. длит. нажат. кнопки (сбрасывает обработчик)
			b->timPush = 0;//сбрасываем счетчик длительного нажатия
		} else {
			b->timPush++;
		}
	} else {//если кнопка отпущена
		b->timPush = 0;//сбрасываем счетчик длительного нажатия
	}
}

/*
 * this function called from main loop every 1 ms
 */
void buttons(void) {

	debounce( &B1, READ_B1 );//антидребезг B1
	eventPushPull( &B1 );//генерить событие при первом нажатии В1
	eventLongPush( &B1 );//генерить событие при длительном нажатии на В1
}



