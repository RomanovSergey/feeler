/*
 * menu.c
 *
 *  Created on: июль 2016 г.
 *      Author: se
 */

#include "stm32f0xx.h"
#include "main.h"
#include "menu.h"
#include "uart.h"
#include "micro.h"

inline void clrscr(void) {
	tx.ind = 0;//индекс буфера для отправки в порт сбрасываем в ноль
	toPrint("\033[2J");//clear entire screen
	toPrint("\033[?25l");//Hides the cursor.
	toPrint("\033[H");//Move cursor to upper left corner.
	printRun();//крутящаяся черточка
	toPrint("\r\n");
}

int measureM(void) {
	static uint16_t cnt = 0;

	if ( g.event == b1LongPush ) {//если поймали глобально событие длительного нажатия кнопки
		pmenu = calibrateM;
		return 0;//перерисовывать не надо
	}

	uint32_t val = g.tim_len;
	clrscr();
	toPrint("\r\n Tim_len = ");//=================================
	toPrint("\033[31m");//set red color
	uint32_to_str( val );
	toPrint("\033[0m");//reset normal (color also default)
	toPrint(" y.e. \r\n");

	toPrint(" microns = ");
	uint32_to_str( micro( val ) );
	toPrint(" um \r\n");

	toPrint("\r\n cnt = ");
	uint16_to_5str( cnt++ );
	return 1;//надо перерисовать
}

int calibrateM(void) {
	if ( g.event == b1LongPush ) {
		pmenu = measureM;
		return 0;//перерисовывать не надо
	}
	if ( g.event == b1Push ) {
		pmenu = calib100M;
		return 0;
	}
	clrscr();
	toPrint("Измерте показание на воздухе, нажмите кнопку \r\n");
	return 1;
}

int calib100M(void) {
	if ( g.event == b1LongPush ) {
		pmenu = measureM;
		return 0;//перерисовывать не надо
	}
	if ( g.event == b1Push ) {
		pmenu = calib200M;
		return 0;
	}
	clrscr();
	toPrint("Put feeler on 100 um and push button \r\n");
	return 1;
}

int calib200M(void) {
	if ( g.event == b1LongPush ) {
		pmenu = measureM;
		return 0;//перерисовывать не надо
	}
	if ( g.event == b1Push ) {
		pmenu = calib300M;
		return 0;
	}
	clrscr();
	toPrint("Put feeler on 200 um and push button \r\n");
	return 1;
}

int calib300M(void) {
	if ( g.event == b1LongPush ) {
		pmenu = measureM;
		return 0;//перерисовывать не надо
	}
	if ( g.event == b1Push ) {
		pmenu = measureM;
		return 0;
	}
	clrscr();
	toPrint("Put feeler on 300 um and push button \r\n");
	return 1;
}

