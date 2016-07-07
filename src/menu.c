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

int measureDisp(void) {
	static uint16_t cnt = 0;

	if ( g.b1_Lpush == 1 ) {
		g.b1_Lpush = 0;

		ptrDispFunc = calibrateDisp;
	}

	if ( g.tim_done == 1 ) {

		uint32_t val = g.tim_len;
		g.tim_done  = 0;
		tx.ind = 0;//индекс буфера для отправки в порт сбрасываем в ноль
		toPrint("\033[2J");//clear entire screen
		toPrint("\033[?25l");//Hides the cursor.
		toPrint("\033[H");//Move cursor to upper left corner.
		printRun();//крутящаяся черточка
		toPrint("\r\n");

		toPrint("\r\n Tim_len = ");//=================================
		toPrint("\033[31m");//set red color
		uint32_to_str( val );
		toPrint("\033[0m");//reset normal (color also default)
		toPrint(" y.e. \r\n");

		toPrint(" microns = ");
		uint32_to_str( micro( val ) );
		toPrint(" um \r\n");

		toPrint("\r\n\r\n cnt = ");
		uint16_to_5str( cnt++ );
		return 1;
	}
	return 0;
}


int calibrateDisp(void) {
	static uint16_t tim = 0;

	if ( g.b1_Lpush == 1 ) {
		g.b1_Lpush = 0;

		ptrDispFunc = measureDisp;
	}

	tim++;
	if (tim == 100) {
		tim = 0;

		tx.ind = 0;//индекс буфера для отправки в порт сбрасываем в ноль
		toPrint("\033[2J");//clear entire screen
		toPrint("\033[?25l");//Hides the cursor.
		toPrint("\033[H");//Move cursor to upper left corner.
		printRun();//крутящаяся черточка
		toPrint("\r\n");

		toPrint("calibrate is not done yet \r\n");
		return 1;
	}
	return 0;
}

