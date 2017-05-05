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

//FTOMSIZE - количество точек для замеров и калибровки
#define FTOMSIZE  20

//#pragma pack(1)
//typedef struct {
//	uint32_t  F;     // measured value of inductance
//	uint16_t  micro; // corresponding value in micrometers
//} ftoM_t;// F to micrometer
//#pragma pack()

//калибровочная таблица в озу
//индекс 0 - железо; индекс 1 - алюминий
//ftoM_t table[2][FTOMSIZE];

#pragma pack(1)
typedef struct {
	uint16_t  points;          // numbers of calibrate points, max FTOMSIZE
	uint32_t  F[FTOMSIZE];     // measured value of inductance
	uint16_t  micro[FTOMSIZE]; // corresponding value in micrometers
} CAL_T;// F to micrometer
#pragma pack()

static CAL_T fe; // calib table for Ferrum
static CAL_T al; // calib table for Aluminum

/*
 * Начальная инициализация калибровочной таблицы в озу
 */
void micro_initCalib(void)
{
	fe.points = 0;
	for (int i = 0; i < FTOMSIZE; i++) {
		fe.F[i] = 0;
		fe.micro[i] = 0xFFFF; // означает воздух (infinity)
	}
	al.points = 0;
	for (int i = 0; i < FTOMSIZE; i++) {
		al.F[i] = 0;
		al.micro[i] = 0xFFFF; // означает воздух (infinity)
	}

	int res;
	uint16_t *pTabl;

	res = flashInit();
	if ( res != 0 ) {
		urtPrint("flashInit error\n");
	}

	pTabl = (uint16_t*)&fe; // Fe
	res = fread( FID_FE_DEF, pTabl );
	if ( res == 0 ) {
		urtPrint("Fe loaded\n");
	} else {
		urtPrint("Err load Fe. Result: ");
		urt_uint32_to_str (res);
		urtPrint("\n");
	}
	pTabl = (uint16_t*)&al; // Al
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
	pTabl = (uint16_t*)&fe;
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
	pTabl = (uint16_t*)&al;
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

	if ( fe.points != 0 ) { // if Ferrum table not empty
		if ( F <= fe.F[0] ) {
			*metall = 0; // Fe
			return 0;
		}
		for (int i = 1; i < fe.points; i++ ) {
			if ( F <= fe.F[i] ) { // (F > table[0][i-1].F)&&(F <= table[0][i].F) ) {
				F2 = fe.F[i];
				M2 = fe.micro[i];
				F1 = fe.F[i-1];
				M1 = fe.micro[i-1];
				M = (F-F1)*(M2-M1)/(F2-F1) + M1;
				*metall = 0; // Fe
				return M;
			}
		}
	}

	if ( al.points != 0 ) { // if Aluminum table not empty
		for (int i = 1; i < al.points; i++ ) {
			if ( F >= al.F[i] ) { // (F > table[0][i-1].F)&&(F <= table[0][i].F) ) {
				F2 = al.F[i];
				M2 = al.micro[i];
				F1 = al.F[i-1];
				M1 = al.micro[i-1];
				M = (F-F1)*(M2-M1)/(F2-F1) + M1;
				*metall = 1; // Al
				return M;
			}
			*metall = 1; // Al
			return 0;
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
 *   1 - out of index
 *   2 - metall unrecognized
 */
int addCalibPoint(uint32_t Freq, uint16_t micro, uint8_t index, int metall)
{
	if ( index >= FTOMSIZE ) { // if overflow
		return 1; // error add point (last element must be 0 and 0xFFFF)
	}
	if ( metall == 0 ) { // Fe
		fe.F[index] = Freq;
		fe.micro[index] = micro;
		fe.points = index;
		return 0; // good
	} else if ( metall == 1 ) { // Al
		al.F[index] = Freq;
		al.micro[index] = micro;
		al.points = index;
		return 0; // good
	}
	return 2;
}

