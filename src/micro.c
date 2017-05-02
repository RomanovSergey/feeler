/*
 * micrometr.c
 *
 *  Created on: May, 2016
 *      Author: se
 */

#include "stm32f0xx.h"
#include "micro.h"
#include "uart.h"
#include "flash2.h"
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
			table[m][i].micro = 0xFFFF; // означает воздух (infinity)
		}
	}

	int res;
	uint16_t *pTabl;

	res = flashInit();
	if ( res != 0 ) {
		urtPrint("flashInit error\n");
	}

	pTabl = (uint16_t*)&table[0][0]; // Fe
	res = fread( FID_FE_DEF, pTabl );
	if ( res == 0 ) {
		urtPrint("Fe loaded\n");
	} else {
		urtPrint("Err load Fe. Result: ");
		urt_uint32_to_str (res);
		urtPrint("\n");
	}
	pTabl = (uint16_t*)&table[1][0]; // Al
	res = fread( FID_AL_DEF, pTabl );
	if ( res == 0 ) {
		urtPrint("Al loaded\n");
	} else {
		urtPrint("Err load Al. Result: ");
		urt_uint32_to_str (res);
		urtPrint("\n");
	}
}

int microSaveFe(void)
{
	int res;
	uint16_t *pTabl;
	pTabl = (uint16_t*)table[0];
	res = fwrite( FID_FE_DEF, pTabl );
	if ( res == 0 ) {
		return 0;
	}
	urtPrint("Err save Fe.\n");
	urt_uint32_to_str (res);
	urtPrint(" \n");
	return res;
}

int microSaveAl(void)
{
	int res;
	uint16_t *pTabl;
	pTabl = (uint16_t*)table[1];
	res = fwrite( FID_AL_DEF, pTabl );
	if ( res == 0 ) {
		return 0;
	}
	urtPrint("Err save Al.\n");
	urt_uint32_to_str (res);
	urtPrint(" \n");
	return res;
}

/*
 * Converts the frequency F (inversely proportional to the inductance)
 *   to the output value in micrometers M
 *
 * The equation of a straight line is used for two points:
 *   (M-M1)/(M2-M1) = (F-F1)/(F2-F1);
 * Then the required value of M is equal to:
 *   M = (F-F1)*(M2-M1)/(F2-F1) + M1;
 */
int16_t micro(int32_t F, uint8_t *metall ) {
	int32_t M, M1, M2, F1, F2;
	uint16_t i=1;

	if ( table[0][0].micro != 0xFFFF ) { // если железная таблица не пустая
		if ( F <= table[0][0].F ) {
			return 0;//величина без зазора - 0 мкм
		}
		while ( i < FTOMSIZE ) { // table[0][i].micro != 0xFFFF ) { // пока таблица не закончилась
			if ( F <= table[0][i].F ) { // (F > table[0][i-1].F)&&(F <= table[0][i].F) ) {
				F2 = table[0][i].F;
				M2 = table[0][i].micro;
				F1 = table[0][i-1].F;
				M1 = table[0][i-1].micro;
				M = (F-F1)*(M2-M1)/(F2-F1) + M1;
				*metall = 0; // Fe
				return M;
			}
			i++;
		}
	}

	if ( table[1][0].micro != 0xFFFF ) { // если алюминиевая таблица не пустая
		if ( F >= table[1][0].F ) {
			return 0;//величина без зазора - 0 мкм
		}
		i = 1;
		while ( i < FTOMSIZE ) { // table[1][i].micro != 0xFFFF ) { // пока таблица не закончилась
			if ( F >= table[1][i].F ) {
				F2 = table[1][i].F;
				M2 = table[1][i].micro;
				F1 = table[1][i-1].F;
				M1 = table[1][i-1].micro;
				M = (F-F1)*(M2-M1)/(F2-F1) + M1;
				*metall = 1; // Al
				return M;
			}
			i++;
		}
	}
	*metall = 0xFF; // ?
	return 0xFFFF;  // air
}

/*
 * This function is used during the calibration of Ferrum or Aluminum
 * Params:
 *   Freq   - measured frequency
 *   micro  - Clearance between sensor and metal
 *   metall - if 0 then Ferrum, if 1 then Aluminum
 * Return:
 *   0 - success
 *   1 - error
 */
int addCalibPoint(uint32_t Freq, uint16_t micro, uint8_t index, int metall)
{
	if ( index >= FTOMSIZE ) { // if overflow
		return 1; // error add point (last element must be 0 and 0xFFFF)
	}
	table[metall][index].F  = Freq;
	table[metall][index].micro = micro;
	return 0; // good
}

