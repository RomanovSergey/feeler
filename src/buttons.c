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

#define READ_B1     GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_8)
#define READ_B2     GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_5)
#define READ_B3     GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_7)

//локальная структура состояния кнопки
typedef struct  {
	int16_t  debcount;//счетчик антидребезга
	uint8_t  current;//текущее отфильтрованное состояние кнопки
	uint8_t  state;//машинное состояние (для отличия клика от дабле клика и лонг пуша)
	uint16_t tim;//временная метка клика(ов)
} button_t;

//под каждую кнопку свой объект
button_t B1 = {
	.debcount = 0,
	.current = 0,
	.state = 0,
	.tim = 0,
};
button_t B2 = {
	.debcount = 0,
	.current = 0,
	.state = 0,
	.tim = 0,
};
button_t B3 = {
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
	if ( instance == 0 ) {//if button is pushed *************************
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
 * buttonEvent локальная функция
 * генерить событие клика, двойной клик, нажатие и отпускание,
 * длительное нажатие
 */
void buttonEvent(button_t *b, uint8_t Eclick, uint8_t Edouble,
		uint8_t Elong, uint8_t Epush, uint8_t Epull)
{
	static const uint16_t clickLimit = 180;
	static const int longPush = 1500;

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
			b->tim = 0;
		}
		break;
	case 2:
		if ( b->tim == clickLimit ) {//если кнопку отпустили на слишком долго
			b->state = 0;
			b->tim = 0;
			dispPutEv( Eclick );//click event
			sndPutEv( 1 );
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
			dispPutEv( Edouble );//double click event
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
		if ( b->tim == clickLimit ) {
			b->state = 101;
			b->tim = 0;
			dispPutEv( Epush );//push event
		}
		break;
	case 101:
		if ( b->current == 0 ) {
			b->state = 0;
			b->tim = 0;
			dispPutEv( Epull );//pull event
		}
		if ( b->tim == longPush ) {
			b->tim = 0;
			b->state = 102;
			dispPutEv( Elong );//long push event
		}
		break;
	case 102:
		if ( b->current == 0 ) {
			b->state = 0;
			b->tim = 0;
			dispPutEv( Epull );//pull event
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
	debounce( &B2, READ_B2 );//антидребезг B1
	debounce( &B3, READ_B3 );//антидребезг B1
	buttonEvent( &B1, DIS_CLICK_OK, 0,
			0, 0, 0 );//generate different events on B1 button
	buttonEvent( &B2, DIS_CLICK_L, 0,
			0, 0, 0 );//generate different events on B2 button
	buttonEvent( &B3, DIS_CLICK_R, 0,
			0, 0, 0 );//generate different events on B3 button
}


