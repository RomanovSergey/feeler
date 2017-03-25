/*
 * micrometr.c
 *
 *  Created on: May, 2016
 *      Author: se
 */

#include "stm32f0xx.h"
#include "micro.h"
#include "uart.h"
#include "flash.h"
#include "main.h"

#pragma pack(1)
typedef struct {
	uint32_t  F;  //измеренное значение индуктивности
	uint16_t  micro; //соответствующее значение в микрометрах
} ftoM_t;// F to micrometer
#pragma pack()

//FTOMSIZE - количество точек для замеров и калибровки
#define FTOMSIZE  20

//калибровочная таблица в озу
//индекс 0 - железо; индекс 1 - алюминий
ftoM_t table[2][FTOMSIZE];

/*
 * Начальная инициализация калибровочной таблицы в озу
 */
void initCalib(void) {
	for (int m = 0; m < 2; m++) {
		for (int i = 0; i < FTOMSIZE; i++) {
			table[m][i].F = 0;
			table[m][i].micro = 0xFFFF;//означает воздух (infinity)
		}
	}

	int res;
	uint16_t rlen;
	uint16_t *pTabl;

	pTabl = (uint16_t*)table[0];
	res = floadImage( FID_FE_DEF, pTabl, FTOMSIZE * sizeof(uint32_t) * sizeof(uint16_t), &rlen );
	if ( res == 0 ) {
		urtPrint("Fe loaded, rlen: ");
		urt_uint32_to_str (rlen);
		urtPrint(" bytes \n");
	} else {
		urtPrint("Err load Fe. Result: ");
		urt_uint32_to_str (res);
		urtPrint(" \n");
	}
	pTabl = (uint16_t*)table[1];
	res = floadImage( FID_AL_DEF, pTabl, FTOMSIZE * sizeof(uint32_t) * sizeof(uint16_t), &rlen );
	if ( res == 0 ) {
		urtPrint("Al loaded, rlen=\n");
		urt_uint32_to_str (rlen);
		urtPrint(" bytes \n");
	} else {
		urtPrint("Err load Al. Result: ");
		urt_uint32_to_str (res);
		urtPrint(" \n");
	}
}

int microSaveFe(void)
{
	int res;
	uint16_t *pTabl;
	pTabl = (uint16_t*)table[0];
	res = fsaveImage( FID_FE_DEF, pTabl, FTOMSIZE * sizeof(uint32_t) * sizeof(uint16_t) );
	if ( res == 0 ) {
		return 0;
	}
	urtPrint("Err save Fe.\n");
	urt_uint32_to_str (res);
	urtPrint(" \n");
	return res;
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

	if ( table[0][0].micro != 0xFFFF ) {//если железная таблица не пустая
		if ( F <= table[0][0].F ) {
			return 0;//величина без зазора - 0 мкм
		}
		while ( table[0][i].micro != 0xFFFF ) {//пока таблица не закончилась
			if ( F <= table[0][i].F ) {// (F > table[0][i-1].F)&&(F <= table[0][i].F) ) {
				F2 = table[0][i].F;
				M2 = table[0][i].micro;
				F1 = table[0][i-1].F;
				M1 = table[0][i-1].micro;
				M = (F-F1)*(M2-M1)/(F2-F1) + M1;
				return M;
			}
			i++;
		}
	}

	if ( table[1][0].micro != 0xFFFF ) {//если алюминиевая таблица не пустая
		if ( F >= table[1][0].F ) {
			return 0;//величина без зазора - 0 мкм
		}
		i = 1;
		while ( table[1][i].micro != 0xFFFF ) {//пока таблица не закончилась
			if ( F >= table[1][i].F ) {
				F2 = table[1][i].F;
				M2 = table[1][i].micro;
				F1 = table[1][i-1].F;
				M1 = table[1][i-1].micro;
				M = (F-F1)*(M2-M1)/(F2-F1) + M1;
				return M;
			}
			i++;
		}
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

	while ( table[metall][i].F != 0 ) {//ищем первую свободную ячейку для записи в таблицу
		i++;
		if ( i == (FTOMSIZE - 1) ) {//если дошли до предела
			return 0;//error add point (последний элемент дожен быть 0 и 0xFFFF)
		}
	}
	table[metall][i].F  = Freq;
	table[metall][i].micro = micro;
	return 1;//good
}

