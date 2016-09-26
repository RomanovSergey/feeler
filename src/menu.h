/*
 * menu.h
 *
 *  Created on: июль 2016 г.
 *      Author: se
 */

#ifndef SRC_MENU_H_
#define SRC_MENU_H_

typedef struct {
	uint32_t  tim;//время отображения сообщения
	int (*retM)(uint8_t);//в какую функцию перейдет меню после отображения сообщения
	const char *message;//нуль терминальная строка - само сообщение
} MESSAGE_T;

int showEventM(uint8_t ev);

int keepValM(uint8_t ev);
int messageError1M(uint8_t);

int powerOn(uint8_t );
int workScreenM(uint8_t);
int mainM(uint8_t);

int calib__0M(uint8_t );
int calib100M(uint8_t );
int calib200M(uint8_t );
int calib300M(uint8_t );
int calib400M(uint8_t );
int calib600M(uint8_t );
int calibMaxM(uint8_t );

int calibDoneM(uint8_t );
int notDoneM(uint8_t );

#endif /* SRC_MENU_H_ */
