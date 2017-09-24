/*
 * adc.h
 *
 *  Created on: 29 мая 2017 г.
 *      Author: se
 */

#ifndef SRC_ADC_H_
#define SRC_ADC_H_

typedef struct {
	uint16_t freq;
	uint16_t volt;
} adcFind_t;

void adcReset();
void adcGetLC( adcFind_t* val );

//void adc( void );
//void adcSaveCalibData( uint32_t cal );
//char* adcGetBattary( void );
//uint16_t adcVbat(void);
//uint16_t adcVda( void );
//int32_t adcT(void);

#endif /* SRC_ADC_H_ */
