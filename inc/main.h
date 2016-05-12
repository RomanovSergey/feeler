/*
 * main.h
 *
 *  Created on: 5 мая 2016 г.
 *      Author: se
 */

#ifndef INC_MAIN_H_
#define INC_MAIN_H_

#define MAGNETIC_ON   GPIO_SetBits(GPIOA,GPIO_Pin_8)
#define MAGNETIC_OFF  GPIO_ResetBits(GPIOA,GPIO_Pin_8)

typedef struct {
	uint32_t ADC_calib;
	uint32_t ADC_value;
	uint32_t ADC_count;
} GLOBAL;

extern GLOBAL g;

#endif /* INC_MAIN_H_ */
