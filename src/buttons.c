/*
 * buttons.c
 *
 *  Created on: 30 апр. 2016 г.
 *      Author: se
 */

#include "stm32f0xx.h"
#include "main.h"

#define READ_B1     GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0)
#define ANTI_TIME   30

typedef struct  {
	uint16_t count;//кол-во выборок (антидребезг)
	uint8_t  current;//текущее среднее состояние кнопки
	uint8_t  prev;//предыдущее среднее состояние кнопки
	uint8_t* ptrPush;//указ. на глобальную - событие нажатия (из нолика в единицу) сбрасывает обработчик
} button_t;

//пока только одна кнопка
button_t B1 = {
	.count = 0,
	.current = 0,
	.prev = 0,
	.ptrPush = &g.B1_push,
};


void debounce(button_t *b, uint8_t instance);

void buttons(void) {

	debounce(&B1, READ_B1);//читаем и фильтруем кнопку B1
}

/*
 * функция для антидребезга, принимает указатель
 * на структуру кнопки и мгновенное значение кнопки
 */
void debounce(button_t *b, uint8_t instance) {
	if ( instance > 0 ) {//если кнопка нажата
		if ( b->count < ANTI_TIME ) {
			b->count++;//фильтр
		} else {
			b->current = 1;
			if ( b->prev == 0 ) {//если смена состояния
				b->prev = 1;
				if ( b->ptrPush != NULL ) {
					*(b->ptrPush) = 1;//то генерим глобальное событие нажатия
				}
			}
		}
	} else {//если кнопка отпущена
		if ( b->count > 0 ) {
			b->count--;//фильтр
		} else {
			b->current = 0;
			if ( b->prev == 1 ) {//если смена состояния
				b->prev = 0;
				//пока нет задачи генерить событие отпускания
			}
		}
	}
}

