/*
 * main.h
 *
 *  Created on: 5 мая 2016 г.
 *      Author: se
 */

#ifndef INC_MAIN_H_
#define INC_MAIN_H_


#define BL1_ON      GPIO_SetBits(GPIOA,GPIO_Pin_11)
#define BL1_OFF     GPIO_ResetBits(GPIOA,GPIO_Pin_11)

#define BL2_ON      GPIO_SetBits(GPIOA,GPIO_Pin_12)
#define BL2_OFF     GPIO_ResetBits(GPIOA,GPIO_Pin_12)

#define PWR_ON      GPIO_SetBits(GPIOB,GPIO_Pin_0)
#define PWR_OFF     GPIO_ResetBits(GPIOB,GPIO_Pin_0)

#define BEEP_ON      GPIO_SetBits(GPIOB,GPIO_Pin_7)
#define BEEP_OFF     GPIO_ResetBits(GPIOB,GPIO_Pin_7)


typedef struct {
	uint32_t air;//air's frequency on power on
	//uint32_t alarm;//будильник для отсчета времени отображения временного меню
} GLOBAL_T;

extern GLOBAL_T g;

#define NULL ((void *)0)

int put_event(uint8_t event);
uint8_t get_event(void);

#endif /* INC_MAIN_H_ */
