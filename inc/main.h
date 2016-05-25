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
	uint32_t ADC_calib;//получили при инициализации калибровку ацп (вдруг пригодится)
	uint32_t ADC_value;//ну типа измереное значение ацп (ну типа усредненое чтоли)
	uint32_t ADC_deltaTime;//здесь время за какое ток нарастает от нуля до ADC_value
	uint32_t ADC_done;//алгоритм завершен данные готовы (ADC_value, ADC_deltaTime)

	uint8_t  B1_push;//событие нажатия B1 кнопки (сбрасыватся обработчиком)

	uint16_t  ind;//указывает на нулевой символ строки (для след. записи)
	uint8_t   buf[256];//отладочный буфер (потом удалить)

} GLOBAL;

extern GLOBAL g;

#define NULL ((void *)0)

#endif /* INC_MAIN_H_ */
