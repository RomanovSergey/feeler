/*
 * main.h
 *
 *  Created on: 5 мая 2016 г.
 *      Author: se
 */

#ifndef INC_MAIN_H_
#define INC_MAIN_H_


#define GREEN_ON      GPIO_SetBits(GPIOC,GPIO_Pin_9)
#define GREEN_OFF     GPIO_ResetBits(GPIOC,GPIO_Pin_9);

#define BLUE_ON       GPIO_SetBits(GPIOC,GPIO_Pin_8);
#define BLUE_OFF      GPIO_ResetBits(GPIOC,GPIO_Pin_8);

typedef struct {
	uint32_t ADC_calib;
	uint32_t ADC_value;
	uint32_t ADC_done;

	uint8_t  B1_push;//событие нажатия B1 кнопки (сбрасыватся обработчиком)
} GLOBAL;

extern GLOBAL g;

#define NULL ((void *)0)

#endif /* INC_MAIN_H_ */
