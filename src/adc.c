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
#include "pwr.h"
#include "helpers.h"
#include <string.h>

static volatile int      irq_stat = 0;

static volatile uint16_t irq_vref = 0;
static volatile uint16_t irq_vbat = 0;
static volatile uint16_t irq_temp = 0;
static uint16_t vref = 0;
static uint16_t vbat = 0;
static uint16_t temp = 0;
static uint32_t sum_vref = 0;
static uint32_t sum_vbat = 0;
static uint32_t sum_temp = 0;
static uint16_t vda_v = 330; // calculated vda in voltage

static uint32_t calData = 0; // saves ADC calib data (is need?)


void ADC1_COMP_IRQHandler( void )
{
	if ( SET == ADC_GetITStatus(ADC1, ADC_IT_EOC) ) {
		if ( irq_stat == 0 ) {
			irq_vbat = ADC_GetConversionValue( ADC1 );
			irq_stat++;
		} else if ( irq_stat == 1 ) {
			irq_temp = ADC_GetConversionValue( ADC1 );
			irq_stat++;
		} else if ( irq_stat == 2 ) {
			irq_vref = ADC_GetConversionValue( ADC1 );
			irq_stat++;
		} else {
			ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
		}
	}
}

#define ADC_COUNT 16
void adc( void )
{
	static int tim = 0;
	static int count = 0;

	if ( irq_stat == 3 ) {
		if ( count < ADC_COUNT ) {
			count++;
			sum_vbat += irq_vbat;
			sum_temp += irq_temp;
			sum_vref += irq_vref;
			irq_stat = 0;
			ADC_StartOfConversion( ADC1 );
		} else {
			count = 0;
			irq_stat++;
			vbat = sum_vbat / ADC_COUNT;
			sum_vbat = 0;
			temp = sum_temp / ADC_COUNT;
			sum_temp = 0;
			vref = sum_vref / ADC_COUNT;
			sum_vref = 0;
			dispPutEv( DIS_ADC );
		}
	}

	tim++;
	if ( tim > 500 ) {
		tim = 0;
		irq_stat = 0;
		ADC_StartOfConversion( ADC1 );
	}
}

void adcSaveCalibData(uint32_t cal)
{
	if ( cal == 0 ) {
		urtPrint("ADC calib error\n");
		//pwrPutEv( PWR_POWEROFF );
	}
	calData = cal;
	urtPrint("ADC calib: ");
	urt_uint32_to_str ( calData );
	urtPrint("\n");
}

int32_t adcT(void)
{
#define VDD_CALIB ((uint16_t) (330))

	const uint16_t TEMP110_CAL_ADDR = *(__I uint16_t*)0x1FFFF7C2;
	const uint16_t TEMP30_CAL_ADDR  = *(__I uint16_t*)0x1FFFF7B8;

	int32_t temperature; /* will contain the temperature in degree Celsius */
	temperature = (((int32_t) temp * vda_v / VDD_CALIB) - (int32_t) TEMP30_CAL_ADDR );
	temperature = temperature * (int32_t)(110 - 30);
	temperature = temperature / (int32_t)(TEMP110_CAL_ADDR - TEMP30_CAL_ADDR);
	temperature = temperature + 30;
	return temperature;
}

/*
 * VDDA = 3.3 V x VREFINT_CAL / VREFINT_DATA
 * Return:
 *   VDDA * 100

uint16_t adcVda( void )
{
	const uint16_t vrefint_cal = *(__I uint16_t*)0x1FFFF7BA;
	uint32_t var;

	var = ( 330 * (uint32_t)vrefint_cal ) / vref;
	vda_v = var;
	return (uint16_t)var;
} */

uint16_t adcVbat(void)
{
	return (uint32_t)vbat * vda_v / 4096;
}

char* adcGetBattary( void )
{
	static char str[16] = {0};
	uint16_t vbat = adcVbat();

	if ( vbat > 999 ) {
		strcpy( str, "err" );
		return str;
	}
	itoa( vbat, str );
	memmove( &str[2], &str[1], 2 );
	str[1] = '.';
	str[4] = 0;
	return str;
}
