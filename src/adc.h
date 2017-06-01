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
uint16_t adcData(void);
uint16_t adcVda( void );

#endif /* SRC_ADC_H_ */
