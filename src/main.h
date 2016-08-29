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


typedef union {
	uint32_t val;
	struct {
		unsigned b1Click : 1;
		unsigned b1Long  : 1;
		unsigned measure : 1;
		unsigned repaint : 1;
	};
}EV_BITF_T;//битовые поля событий


typedef struct {
	EV_BITF_T  ev;//битовые поля событий

	uint32_t tim_len;//time of lenght N pulses in magnetic measures
	uint32_t tim_done;//flag data timer is ready
} GLOBAL_T;

extern GLOBAL_T g;

#define NULL ((void *)0)

#endif /* INC_MAIN_H_ */
