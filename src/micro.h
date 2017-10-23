/*
 * micrometer.h
 *
 *  Created on: May 25, 2016
 *      Author: se
 */

#ifndef MICROMETER_H_
#define MICROMETER_H_

int micro( uint16_t F, uint16_t *micro );
void micro_initCalib(void);
int microSaveFe(void);
int microSaveAl(void);
//int microSetAir( uint16_t air );
int addCalibPoint(uint32_t Freq, uint16_t micro, uint8_t index, int metall);

#endif /* MICROMETER_H_ */
