/*
 * micrometr.c
 *
 *  Created on: May, 2016
 *      Author: se
 */

#include "stm32f0xx.h"
#include "micro.h"
#include "main.h"

typedef struct {
	uint32_t  F;  //измеренное значение индуктивности
	uint16_t  micro; //соответствующее значение в микрометрах
} ftoM_t;// F to micrometer


//FTOMSIZE - количество точек для замеров и калибровки
#define FTOMSIZE  20

//калибровочная таблица в озу
ftoM_t tableFe[FTOMSIZE];//железо
ftoM_t tableAl[FTOMSIZE];//Aluminumum

/*
 * Начальная инициализация калибровочной таблицы в озу
 */
void initCalib(void) {
	for (int i = 0; i < FTOMSIZE; i++) {
		tableFe[i].F = 0;
		tableFe[i].micro = 0xFFFF;//означает воздух (infinity)
	}
	for (int i = 0; i < FTOMSIZE; i++) {
		tableAl[i].F = 0;
		tableAl[i].micro = 0xFFFF;//означает воздух (infinity)
	}
}

/*
 * Перобразует частоту F (обр. пропорциональна индуктивности)
 * в выходное значение в микрометрах M
 *
 * Используется уравнение прямой по двум точкам:
 * (M-M1)/(M2-M1) = (F-F1)/(F2-F1);
 * тогда искомая величина M равна:
 * M = (F-F1)*(M2-M1)/(F2-F1) + M1;
 */
int16_t micro(int32_t F) {
	int32_t M, M1, M2, F1, F2;
	uint16_t i=1;

	if ( tableFe[0].micro != 0xFFFF ) {//если железная таблица не пустая
		if ( F <= tableFe[0].F ) {
			return 0;//величина без зазора - 0 мкм
		}
		while ( tableFe[i].micro != 0xFFFF ) {
			if ( (F > tableFe[i-1].F)&&(F <= tableFe[i].F) ) {
				F2 = tableFe[i].F;
				M2 = tableFe[i].micro;
				F1 = tableFe[i-1].F;
				M1 = tableFe[i-1].micro;
				M = (F-F1)*(M2-M1)/(F2-F1) + M1;
				return M;
			}
			i++;
		}
	}
	if ( tableAl[0].micro != 0xFFFF ) {//если алюминиевая таблица не пустая
		if ( F <= tableAl[0].F ) {
			return 0xFFFF;//air
		}
		i = 1;
		while ( tableAl[i].micro != 0xFFFF ) {
			if ( (F > tableAl[i-1].F)&&(F <= tableAl[i].F) ) {
				F2 = tableAl[i].F;
				M2 = tableAl[i].micro;
				F1 = tableAl[i-1].F;
				M1 = tableAl[i-1].micro;
				M = (F-F1)*(M2-M1)/(F2-F1) + M1;
				return M;
			}
			i++;
		}
		return 0;
	}
	return 0xFFFF;//air
}

/*
 * int addCalibPoint(uint32_t Freq, uint16_t micro, int metall)
 * Функция используется во время калибровки железа или алюминия
 * параметры
 *   Freq  - измеренная величина
 *   micro - известный зазор между датчиком и металлом
 *   metall - если 0 то железо, если 1 то алюминий
 * return:
 *   1 - успех
 *   0 - ошибка
 */
int addCalibPoint(uint32_t Freq, uint16_t micro, int metall) {
	int i = 0;

	while ( tableFe[i].F != 0 ) {//ищем первую свободную ячейку для записи в таблицу
		i++;
		if ( i == (FTOMSIZE - 1) ) {//если дошли до предела
			return 0;//error add point (последний элемент дожен быть 0 и 0xFFFF)
		}
	}
	if ( metall == 0 ) {
		tableFe[i].F  = Freq;
		tableFe[i].micro = micro;
	} else {
		tableAl[i].F  = Freq;
		tableAl[i].micro = micro;
	}

	return 1;//good
}

