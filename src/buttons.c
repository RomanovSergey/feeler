/*
 * buttons.c
 *
 *  Created on: апр. 2016 г.
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
	uint8_t  state;//машинное состояние (для отличия клика от дабле клика и лонг пуша)
	uint16_t tim;//временная метка клика(ов)
	uint16_t timPush;//время нажатия кнопки (для события длительного нажатия)
} button_t;

//only one button yet
button_t B1 = {
	.debcount = 0,
	.current = 0,
	.prev = 0,
	.state = 0,
	.tim = 0,
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
			put_event(Eb1Click);//событие нажат. кнопки (сбрасывает обработчик)
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
			//g.event = b1LongPush;//сгенерим глоб.событ. длит. нажат. кнопки (сбрасывает обработчик)
			put_event(Eb1Long);//событие длительного нажатия
			b->timPush = 0;//сбрасываем счетчик длительного нажатия
		} else {
			b->timPush++;
		}
	} else {//если кнопка отпущена
		b->timPush = 0;//сбрасываем счетчик длительного нажатия
	}
}

/*
 * click локальная функция
 * генерить событие клика при одном кратком нажатии кнопки
 */
void click(button_t *b) {
	static const uint16_t clickLimit = 200;
	static const int longPush = 2000;

	b->tim++;
	switch ( b->state ) {
	case 0:
		if ( b->current == 1 ) {//первое нажатие кнопки
			b->state = 1;
			b->tim = 0;
		}
		break;
	case 1:
		if ( b->current == 0 ) {//первое отпускание кнопки
			b->state = 2;
			b->tim = 0;
		}
		if ( b->tim == clickLimit ) {
			b->state = 100;
		}
		break;
	case 2:
		if ( b->tim == clickLimit ) {//если кнопку отпустили на слишком долго
			b->state = 0;
			b->tim = 0;
			put_event( Eb1Click );
		}
		if ( b->current == 1 ) {//пока клонит к двойному щелчку
			b->tim = 0;
			b->state = 3;
		}
		break;
	case 3:
		if ( b->current == 0 ) {//второй раз отпустили кнопку
			b->tim = 0;
			b->state = 4;
		}
		if ( b->tim == clickLimit ) {//слишком долго держат нажатой второй раз кнопку
			b->state = 10;
			b->tim = 0;
		}
		break;
	case 4:
		if ( b->current == 1 ) {//слишком шустро нажимать кнопки не следует
			b->tim = 0;
			b->state = 10;
		}
		if ( b->tim == clickLimit ) {
			b->state = 0;
			b->tim = 0;
			put_event( Eb1Double );//событие двойного нажатия
		}
		break;

	case 10://просто ждем отпускания кнопки без генерации событий
		if ( b->current == 0 ) {//ну наконец то отпустили кнопку
			b->tim = 0;
			b->state = 0;
		}
		break;

	case 100:
		if ( b->current == 0 ) {//отпустили кнопку раньше Long push
			b->tim = 0;
			b->state = 0;
		}
		if ( b->tim == longPush ) {
			b->tim = 0;
			b->state = 10;
			put_event( Eb1Long );
		}
		break;
	default:
		b->state = 0;
		b->tim = 0;
	}
}

/*
 * this function called from main loop every 1 ms
 */
void buttons(void) {

	debounce( &B1, READ_B1 );//антидребезг B1
	click( &B1 );//генерить событие при первом нажатии В1
	//eventLongPush( &B1 );//генерить событие при длительном нажатии на В1
}
//надо сделать
//click( &B1 );
//doubleClick( &B1 );
//longPush( &B1 );


