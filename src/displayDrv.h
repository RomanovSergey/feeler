/*
 * displayDrv.h
 *
 *  Created on: Oct 13, 2016
 *      Author: se
 */

#ifndef DISPLAYDRV_H_
#define DISPLAYDRV_H_

#define DIS_REPAINT   1
#define DIS_CLICK_OK  10
#define DIS_CLICK_L   11
#define DIS_CLICK_R   12
#define DIS_MEASURE   30

typedef int (*pdisp_t)(uint8_t event);


void initDisplay(void);
void display(void);//вызывается из main

void disClear(void);
void disPrint(uint8_t numstr, uint8_t X, const char* s);
void disUINT32_to_str (uint8_t numstr, uint8_t X, uint32_t nmb);

int dispPutEv(uint8_t event);

#endif /* DISPLAYDRV_H_ */
