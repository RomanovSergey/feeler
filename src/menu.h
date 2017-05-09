/*
 * menu.h
 *
 *  Created on: июль 2016 г.
 *      Author: se
 */

#ifndef SRC_MENU_H_
#define SRC_MENU_H_

int dnotDone(uint8_t ev);
int dmessageError1(uint8_t ev);
int emptyDisplay(uint8_t event);
int dPowerOn(uint8_t ev);
int dworkScreen(uint8_t event);
int dmainM(uint8_t ev);
int duserCalib(uint8_t ev);

int dcalibDone(uint8_t ev);
int dcalibFeDone(uint8_t ev);
int dcalibAlDone(uint8_t ev);
int dcalibFe(uint8_t ev);
int dcalibAl(uint8_t ev);

int dflashDebug(uint8_t ev);
int dflashShow(uint8_t ev);
int dstatusFlash(uint8_t ev);

int dimageShtrih(uint8_t ev);

#endif /* SRC_MENU_H_ */
