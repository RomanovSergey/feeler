/*
 * magnetic.h
 *
 *  Created on: May 31, 2016
 *      Author: se
 */

#ifndef MAGNETIC_H_
#define MAGNETIC_H_

#define MG_ON   1
#define MG_OFF  2

int mgPutEv(uint8_t event);

int magGetStat(void);
uint16_t getFreq(void);
void magnetic(void);

#endif /* MAGNETIC_H_ */
