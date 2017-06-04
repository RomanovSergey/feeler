/*
 * adc.h
 *
 *  Created on: 29 мая 2017 г.
 *      Author: se
 */

#ifndef SRC_ADC_H_
#define SRC_ADC_H_

void adc( void );
void adcSaveCalibData( uint32_t cal );
char* adcGetBattary( void );
uint16_t adcGetCalib(void);
uint16_t adcRaw(void);
uint16_t adcVbat(void);
//uint16_t adcVref(void);
uint16_t adcVda( void );
//uint16_t adcVcal( void );

#endif /* SRC_ADC_H_ */
