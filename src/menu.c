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

int measureDisp(void) {
	static uint16_t cnt = 0;

	if ( g.event == b1LongPush ) {//если поймали глобально событие длительного нажатия кнопки
		ptrDispFunc = calibrateDisp;
		g.event = repaint;
		return 0;//перерисовывать не надо
	}

	if ( g.event == measure || g.event == repaint ) {
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
		g.event = noEvent;
		return 1;
	}
	g.event = noEvent;//сбросим прочие не интересные события
	return 0;
}

int calibrateDisp(void) {
	if ( g.event == b1LongPush ) {
		ptrDispFunc = measureDisp;
		g.event = repaint;
		return 0;//перерисовывать не надо
	}
	if ( g.event == measure || g.event == repaint ) {
		clrscr();
		toPrint("Put feeler on the air and then push button \r\n");
		g.event = noEvent;
		return 1;
	}
	if ( g.event == b1Push ) {
		ptrDispFunc = calib100;
		g.event = repaint;
		return 0;
	}
	g.event = noEvent;
	return 0;
}

int calib100(void) {
	if ( g.event == b1LongPush ) {
		ptrDispFunc = measureDisp;
		g.event = repaint;
		return 0;//перерисовывать не надо
	}
	if ( g.event == measure || g.event == repaint ) {
		clrscr();
		toPrint("Put feeler on 100 um and push button \r\n");
		g.event = noEvent;
		return 1;
	}
	if ( g.event == b1Push ) {
		ptrDispFunc = calib200;
		g.event = repaint;
		return 0;
	}
	g.event = noEvent;
	return 0;
}

int calib200(void) {
	if ( g.event == b1LongPush ) {
		ptrDispFunc = measureDisp;
		g.event = repaint;
		return 0;//перерисовывать не надо
	}
	if ( g.event == measure || g.event == repaint ) {
		clrscr();
		toPrint("Put feeler on 200 um and push button \r\n");
		g.event = noEvent;
		return 1;
	}
	if ( g.event == b1Push ) {
		ptrDispFunc = calib300;
		g.event = repaint;
		return 0;
	}
	g.event = noEvent;
	return 0;
}

int calib300(void) {
	if ( g.event == b1LongPush ) {
		ptrDispFunc = measureDisp;
		g.event = repaint;
		return 0;//перерисовывать не надо
	}
	if ( g.event == measure || g.event == repaint ) {
		clrscr();
		toPrint("Put feeler on 300 um and push button \r\n");
		g.event = noEvent;
		return 1;
	}
	if ( g.event == b1Push ) {
		ptrDispFunc = measureDisp;
		g.event = repaint;
		return 0;
	}
	g.event = noEvent;
	return 0;
}

