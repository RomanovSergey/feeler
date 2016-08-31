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

int mainM(uint8_t);
int message_1_M(uint8_t);
int calibAirM(uint8_t );
int calib__0M(uint8_t );
int calib100M(uint8_t );
int calib200M(uint8_t );
int calib300M(uint8_t );
int calib400M(uint8_t );
int calib600M(uint8_t );
int calibDoneM(uint8_t );

#endif /* SRC_MENU_H_ */
