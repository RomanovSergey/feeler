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

static int      irq_stat = 0;
static uint16_t irq_vref = 0;
static uint16_t vref = 0;
static uint16_t irq_vbat = 0;
static uint16_t vbat = 0;

static uint32_t calData = 0; // saves ADC calib data (is need?)


void ADC1_COMP_IRQHandler( void )
{
	if ( SET == ADC_GetITStatus(ADC1, ADC_IT_EOC) ) {
		if ( irq_stat == 0 ) {
			irq_vref = ADC_GetConversionValue( ADC1 );
			ADC_ChannelConfig(ADC1, ADC_Channel_3, ADC_SampleTime_71_5Cycles); // vbat
			irq_stat = 1;
			ADC_StartOfConversion( ADC1 );
		} else if ( irq_stat == 1 ) {
			irq_vbat = ADC_GetConversionValue( ADC1 );
			ADC_ITConfig( ADC1, ADC_IT_EOC, DISABLE );
			irq_stat = 2;
		}
		ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
	}
}

void adc( void )
{
	static uint16_t cnt = 0;

	if ( irq_stat == 2 ) {
		irq_stat = 3;
		vbat = irq_vbat;
		vref = irq_vref;
		dispPutEv( DIS_ADC );
	}

	cnt++;
	if ( cnt > 500 ) {
		cnt = 0;
		irq_stat = 0;
		ADC_ChannelConfig( ADC1, ADC_Channel_17, ADC_SampleTime_71_5Cycles ); // vref
		ADC_StartOfConversion( ADC1 );
		ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
	}
}

void adcSaveCalibData(uint32_t cal)
{
	calData = cal;
	urtPrint("ADC calib: ");
	urt_uint32_to_str ( calData );
	urtPrint("\n");
}

uint16_t adcGetCalib(void)
{
	return calData;
}

uint16_t adcRaw(void)
{
	return vbat;
}

uint16_t adcVbat(void)
{
	return (uint32_t)vbat * 325 / 4096;
}

char* adcGetBattary( void )
{
	static char battaryStr[5];
	battaryStr[0] = '0';
	battaryStr[1] = '.';
	battaryStr[2] = '9';
	battaryStr[3] = '5';
	battaryStr[4] = 0;
	return battaryStr;
}

/*
 * VDDA = 3.3 V x VREFINT_CAL / VREFINT_DATA
 * Return:
 *   VDDA * 100
 */
uint16_t adcVda( void )
{
	const uint16_t vrefint_cal = *(__I uint16_t*)0x1FFFF7BA; // 3280;
	uint32_t var;

	var = ( 330 * (uint32_t)vrefint_cal ) / vref;

	return (uint16_t)var;
}

