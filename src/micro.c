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

#pragma pack(1)
typedef struct {
	uint16_t  F[FTOMSIZE];     // measured frequency of inductance
	uint16_t  micro[FTOMSIZE]; // corresponding value in micrometers
	uint16_t  max;             // max index number of calibrate points, max FTOMSIZE-1
	uint16_t  air;             // frequency of air at calibrate time for termo correction
} CAL_T;// F to micrometer
#pragma pack()

static CAL_T fe; // calib table for Ferrum
static CAL_T al; // calib table for Aluminum
uint16_t Air;

/*
 * Initial initialization of the calibration table
 */
void micro_initCalib(void)
{
	fe.max = 0;
	for (int i = 0; i < FTOMSIZE; i++) {
		fe.F[i] = 0;
		fe.micro[i] = 0xFFFF; // означает воздух (infinity)
	}
	al.max = 0;
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
 *   to the output value in micrometers
 *
 * The equation of a straight line is used for two points:
 *   (M-M1)/(M2-M1) = (F-F1)/(F2-F1);
 * Then the required value of M is equal to:
 *   M = (F-F1)*(M2-M1)/(F2-F1) + M1;
 *
 * Return:
 *   0 - Ferrum: *micro has result
 *   1 - Air
 *   2 - Aluminum: *micro has result
 *   3 - No Fe calib data
 *   4 - No Al calib data
 */
int micro( uint16_t F, uint16_t *micro )
{
	int32_t M, M1, M2, F1, F2;
	uint8_t hasFe = 1;
	uint8_t hasAl = 1;

	if ( fe.max != 0 ) { // if Ferrum table not empty
		if ( F <= fe.F[0] ) {
			*micro = 0;
			return 0; // Fe
		}
		for (int i = 1; i <= fe.max; i++ ) {
			if ( F <= fe.F[i] ) {
				F2 = fe.F[i];
				M2 = fe.micro[i];
				F1 = fe.F[i-1];
				M1 = fe.micro[i-1];
				M = (F-F1)*(M2-M1)/(F2-F1) + M1;
				*micro = M;
				return 0; // Fe
			}
		}
	} else {
		hasFe = 0; // Ferrum has no calib data
	}

	if ( al.max != 0 ) { // if Aluminum table not empty
		if ( F >= al.F[0] ) { // (very high frequency)
			*micro = 0;
			return 2; // Al
		}
		for (int i = 1; i <= al.max; i++ ) {
			if ( F >= al.F[i] ) {
				F2 = al.F[i];
				M2 = al.micro[i];
				F1 = al.F[i-1];
				M1 = al.micro[i-1];
				M = (F-F1)*(M2-M1)/(F2-F1) + M1;
				*micro = M;
				return 2; // Al
			}
		}
	} else {
		hasAl = 0; // Aluminum has no calib data
	}

	if ( F < (Air - 500) ) {
		if ( hasFe == 0 ) {
			return 3; // No Fe calib data
		}
	}
	if ( F > (Air + 500) ) {
		if ( hasAl == 0 ) {
			return 4; // No Al calib data
		}
	}
	return 1; // Air
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
		fe.max = index;
		return 0; // good
	} else if ( metall == 1 ) { // Al
		al.F[index] = Freq;
		al.micro[index] = micro;
		al.max = index;
		return 0; // good
	}
	return 2;
}

