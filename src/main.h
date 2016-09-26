/*
 * main.h
 *
 *  Created on: 5 мая 2016 г.
 *      Author: se
 */

#ifndef INC_MAIN_H_
#define INC_MAIN_H_


#define GREEN_ON      GPIO_SetBits(GPIOC,GPIO_Pin_9)
#define GREEN_OFF     GPIO_ResetBits(GPIOC,GPIO_Pin_9)

#define BLUE_ON       GPIO_SetBits(GPIOC,GPIO_Pin_8)
#define BLUE_OFF      GPIO_ResetBits(GPIOC,GPIO_Pin_8)

//коды событий (здесь нельзя использовать событие с кодом 0)
#define Eb1Click  1
#define Eb1Double 2
#define Eb1Long   3
#define Eb1Push   4
#define Eb1Pull   5
#define Emeasure  6
#define Erepaint  7
#define Ealarm    8

typedef struct {
	uint32_t tim_len;//time of lenght N pulses in magnetic measures
	uint32_t tim_done;//flag data timer is ready
	uint32_t air;//air's frequency on power on

	uint32_t alarm;//будильник для отсчета времени отображения временного меню
} GLOBAL_T;

extern GLOBAL_T g;

#define NULL ((void *)0)

int put_event(uint8_t event);
uint8_t get_event(void);

#endif /* INC_MAIN_H_ */
