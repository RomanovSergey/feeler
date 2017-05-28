/*
 * adc.c
 *
 *  Created on: 29 мая 2017 г.
 *      Author: se
 */

#include "stm32f0xx.h"
#include "adc.h"

static uint16_t calData = 0;

void adcSaveCalibData(uint32_t cal)
{
	calData = cal;
}

