/*
 * adc.c
 *
 *  Created on: 3 мая 2016 г.
 *      Author: se
 */

#include "stm32f0xx.h"
#include "main.h"

/*
 * Копирует строку str в глобальный отладочный буфер
 * символ конца строки - 0 (не копируется)
 */
int8_t debug(char *str) {
	uint16_t i = 0;

	while (str[i] != 0) {
		if (g.ind < 256) {
			g.buf[g.ind] = (uint8_t)str[i];
			g.ind++;
			i++;
		} else {
			return -1;//buffer overflow
		}
	}
	g.buf[g.ind] = 0;
	return 0;//all ok
}
