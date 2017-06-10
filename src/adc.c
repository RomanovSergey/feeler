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

static volatile int      irq_stat = 0;

static volatile uint16_t irq_vref = 0;
static volatile uint16_t irq_vbat = 0;
static volatile uint16_t irq_temp = 0;
static uint16_t vref = 0;
static uint16_t vbat = 0;
static uint16_t temp = 0;

static uint32_t calData = 0; // saves ADC calib data (is need?)


void ADC1_COMP_IRQHandler( void )
{
	if ( SET == ADC_GetITStatus(ADC1, ADC_IT_EOC) ) {
		if ( irq_stat == 0 ) {
			irq_vref = ADC_GetConversionValue( ADC1 );
			irq_stat++;
		} else if ( irq_stat == 1 ) {
			irq_vbat = ADC_GetConversionValue( ADC1 );
			irq_stat++;
		} else if ( irq_stat == 2 ) {
			irq_temp = ADC_GetConversionValue( ADC1 );
			irq_stat++;
		}
		ADC_ITConfig( ADC1, ADC_IT_EOC, DISABLE );
		ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
		ADC_StopOfConversion( ADC1 );
	}
}

void adc( void )
{
	static int cnt = 0;

	switch ( irq_stat ) {
	case 0:
		ADC1->CHSELR = 0;
		ADC_ChannelConfig(ADC1, ADC_Channel_17, ADC_SampleTime_239_5Cycles); // for vref
		ADC_ITConfig( ADC1, ADC_IT_EOC, ENABLE );
		ADC_StartOfConversion( ADC1 );
		break;
	case 1:
		ADC1->CHSELR = 0;
		ADC_ChannelConfig(ADC1, ADC_Channel_3, ADC_SampleTime_71_5Cycles); // for vbat
		ADC_ITConfig( ADC1, ADC_IT_EOC, ENABLE );
		ADC_StartOfConversion( ADC1 );
		break;
	case 2:
		ADC1->CHSELR = 0;
		ADC_ChannelConfig(ADC1, ADC_Channel_16, ADC_SampleTime_239_5Cycles); // for temperature
		ADC_ITConfig( ADC1, ADC_IT_EOC, ENABLE );
		ADC_StartOfConversion( ADC1 );
		break;
	case 3:
		vbat = irq_vbat;
		vref = irq_vref;
		temp = irq_temp;
		dispPutEv( DIS_ADC );
		irq_stat++;
		break;
	}

	cnt++;
	if ( cnt > 500 ) {
		cnt = 0;
		irq_stat = 0;
	}
}

void adcSaveCalibData(uint32_t cal)
{
	if ( cal == 0 ) {
		urtPrint("ADC calib error\n");
		pwrPutEv( PWR_POWEROFF );
	}
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

int32_t adcT(void)
{
//#define TEMP110_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FFFF7C2))
//#define TEMP30_CAL_ADDR ((uint16_t*) ((uint32_t) 0x1FFFF7B8))
#define VDD_CALIB ((uint16_t) (330))
#define VDD_APPLI ((uint16_t) (330))

	const uint16_t TEMP110_CAL_ADDR = *(__I uint16_t*)0x1FFFF7C2;
	const uint16_t TEMP30_CAL_ADDR  = *(__I uint16_t*)0x1FFFF7B8;

//	int32_t T;
//	T = (110 - 30) * (temp - TEMP30_CAL_ADDR) / (TEMP110_CAL_ADDR - TEMP30_CAL_ADDR) + 30;
//	return T;

	int32_t temperature; /* will contain the temperature in degree Celsius */
	temperature = (((int32_t) temp * VDD_APPLI / VDD_CALIB) - (int32_t) TEMP30_CAL_ADDR );
	temperature = temperature * (int32_t)(110 - 30);
	temperature = temperature / (int32_t)(TEMP110_CAL_ADDR - TEMP30_CAL_ADDR);
	temperature = temperature + 30;
	return temperature;
}

/*
 * VDDA = 3.3 V x VREFINT_CAL / VREFINT_DATA
 * Return:
 *   VDDA * 100
 */
uint16_t adcVda( void )
{
	const uint16_t vrefint_cal = *(__I uint16_t*)0x1FFFF7BA;
	uint32_t var;

	var = ( 330 * (uint32_t)vrefint_cal ) / vref;
	return (uint16_t)var;
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

