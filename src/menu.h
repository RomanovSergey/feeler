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
	int (*retM)(void);//в какую функцию перейдет меню после отображения сообщения
	const char *message;//нуль терминальная строка - само сообщение
} MESSAGE_T;

int measureM(void);
int MessageM(void);
int calibAirM(void);
int calib100M(void);
int calib200M(void);
int calib300M(void);

#endif /* SRC_MENU_H_ */
