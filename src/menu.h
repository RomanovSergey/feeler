/*
 * menu.h
 *
 *  Created on: июль 2016 г.
 *      Author: se
 */

#ifndef SRC_MENU_H_
#define SRC_MENU_H_

int emptyDisplay(uint8_t event);
int dPowerOn(uint8_t ev);
int dworkScreen(uint8_t event);
int dmainM(uint8_t ev);


//===========================================================================
//===========================================================================


//typedef struct {
//	uint32_t  tim;//время отображения сообщения
//	int (*retM)(uint8_t);//в какую функцию перейдет меню после отображения сообщения
//	const char *message;//нуль терминальная строка - само сообщение
//} MESSAGE_T;
//
//int showEventM(uint8_t ev);
//
//int keepValM(uint8_t ev);
//int messageError1M(uint8_t);
//
//int powerOn(uint8_t );
//int workScreenM(uint8_t );
//  int mainM(uint8_t );
//    int userCalibM(uint8_t );
//      int calibFeM(uint8_t );
//      int calibAlM(uint8_t );
//        int calibDoneM(uint8_t );
//    int notDoneM(uint8_t );

#endif /* SRC_MENU_H_ */
