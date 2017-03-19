/*
 * displayDrv.h
 *
 *  Created on: Oct 13, 2016
 *      Author: se
 */

#ifndef DISPLAYDRV_H_
#define DISPLAYDRV_H_

#define DISRESET_LOW    GPIO_ResetBits(GPIOB,GPIO_Pin_1)   //reset display on
#define DISRESET_HI     GPIO_SetBits(GPIOB,GPIO_Pin_1)     //reset display off

#define DIS_PAINT        1
#define DIS_REPAINT      2
#define DIS_ALARM        3
#define DIS_PUSH_OK      10
#define DIS_LONGPUSH_OK  11
#define DIS_PULL_OK      12
#define DIS_PUSH_L       20
#define DIS_LONGPUSH_L   21
#define DIS_PULL_L       22
#define DIS_PUSH_R       30
#define DIS_LONGPUSH_R   31
#define DIS_MEASURE      50

typedef int (*pdisp_t)(uint8_t event);
extern pdisp_t pdisp;

void initDisplay(void);
void display(void);//вызывается из main

void disClear(void);
void disPrint(uint8_t numstr, uint8_t X, const char* s);
void disPrin(const char* s);
void disUINT32_to_str (uint8_t numstr, uint8_t X, uint32_t nmb);
void disUINT32_to_strFONT2 (uint8_t numstr, uint8_t X, uint32_t nmb);
void disHexHalfWord (uint8_t numstr, uint8_t X, uint16_t nmb);
void disShowImg( const uint8_t *img );

int dispPutEv(uint8_t event);

#endif /* DISPLAYDRV_H_ */
