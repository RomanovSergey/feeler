/*
 * adc.c
 *
 *  Created on: 29 мая 2017 г.
 *      Author: se
 */

#include "stm32f0xx.h"
#include "adc.h"
#include "uart.h"

static uint32_t calData = 0; // saves ADC calib data (is need?)
static char battaryStr[5];

void adcSaveCalibData(uint32_t cal)
{
	calData = cal;
	urtPrint("ADC calib: ");
	urt_uint32_to_str ( calData );
	urtPrint("\n");
}

char* adcGetBattary( void )
{
	battaryStr[0] = '2';
	battaryStr[1] = '.';
	battaryStr[2] = '9';
	battaryStr[3] = '5';
	battaryStr[4] = 0;
	return battaryStr;
}
