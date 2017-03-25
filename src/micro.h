/*
 * micrometer.h
 *
 *  Created on: May 25, 2016
 *      Author: se
 */

#ifndef MICROMETER_H_
#define MICROMETER_H_

int16_t micro(int32_t );
void initCalib(void);
int microSaveFe(void);
int addCalibPoint(uint32_t Freq, uint16_t micro, int metall);

#endif /* MICROMETER_H_ */
