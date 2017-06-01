/*
 * adc.c
 *
 *  Created on: 29 мая 2017 г.
 *      Author: se
 */

#include "stm32f0xx.h"
#include "adc.h"
#include "uart.h"
#include "displayDrv.h"

static int      irq_ready = 0;
static uint16_t irq_adc_data = 0;
static uint16_t adc_data = 0;

static uint32_t calData = 0; // saves ADC calib data (is need?)


void ADC1_COMP_IRQHandler( void )
{
	//while ( RESET == ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) );
	if ( SET == ADC_GetITStatus(ADC1, ADC_IT_EOC) ) {
		ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
		irq_adc_data = ADC_GetConversionValue(ADC1);
		irq_ready = 1;
	}
}

void adc( void )
{
	static uint16_t cnt = 0;

	if ( irq_ready == 1 ) {
		irq_ready = 0;
		adc_data = irq_adc_data;
		dispPutEv( DIS_MEASURE );
	}

	cnt++;
	if ( cnt > 500 ) {
		cnt = 0;
		ADC_StartOfConversion(ADC1);
	}
}

void adcSaveCalibData(uint32_t cal)
{
	calData = cal;
	urtPrint("ADC calib: ");
	urt_uint32_to_str ( calData );
	urtPrint("\n");
}

uint16_t adcData(void)
{
	return adc_data;
}

char* adcGetBattary( void )
{
	static char battaryStr[5];
	battaryStr[0] = '2';
	battaryStr[1] = '.';
	battaryStr[2] = '9';
	battaryStr[3] = '5';
	battaryStr[4] = 0;
	return battaryStr;
}

uint16_t adcVda( void )
{
	uint16_t vrefint_cal; // VREFINT calibration value
	vrefint_cal = *(__IO uint16_t*)0x1FFFF7BA;
	return vrefint_cal;
}
