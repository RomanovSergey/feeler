/*
 * menu.c
 *
 *  Created on: июль 2016 г.до н.э.
 *      Author: se
 */

#include "stm32f0xx.h"
#include "main.h"
#include "menu.h"
#include "uart.h"
#include "micro.h"

MESSAGE_T mes;

inline void clrscr(void) {
	tx.ind = 0;//индекс буфера для отправки в порт сбрасываем в ноль
	toPrint("\033[2J");//clear entire screen
	toPrint("\033[?25l");//Hides the cursor.
	toPrint("\033[H");//Move cursor to upper left corner.
	printRun();//крутящаяся черточка
	toPrint("\r\n");
}

/*
 * Первая функция меню - отображает измеренное значение толщины
 * по мере поступления новых данных
 */
int mainM(uint8_t ev) {
	static uint16_t cnt = 0;

	if ( ev == Eb1Long ) {//если поступило событие длительного нажатия кнопки
		pmenu = calibAirM;//указатель на процедурку калибровки
		return 0;//перерисовывать не надо
	}
	if ( ev == Eb1Click ) {//событие нажания кнопки
		return 0;//ниче не делаем
	}

	uint32_t val = g.tim_len;
	clrscr();
	toPrint("\r\n Tim_len = ");//=================================
	//toPrint("\033[31m");//set red color
	uint32_to_str( val );
	//toPrint("\033[0m");//reset normal (color also default)
	toPrint(" y.e. \r\n");

	toPrint(" microns = ");
	uint32_to_str( micro( val ) );
	toPrint(" um \r\n");

	toPrint("\r\n cnt = ");
	uint16_to_5str( cnt++ );
	return 1;//надо перерисовать
}

/*
 * Функция для вывода временного сообщения 1
 */
int message_1_M(uint8_t ev) {
	if ( ev == Ealarm ) {
		pmenu = mainM;
		return 0;
	}
	if ( ev == Emeasure ) {
		return 0;
	}
	clrscr();
	toPrint("Error error error!!!\r\n");
	toPrint("Вы сделали что то не хорошее\r\n");
	return 1;
}

/*
 * Начальное меню процедуры калибровки, начиная с замера воздуха
 */
int calibAirM(uint8_t ev) {
	if ( ev == Eb1Long ) {
		pmenu = mainM;
		return 0;//перерисовывать не надо
	}
	if ( ev == Eb1Click ) {
		int res = 0;// addCalibPoint(g.tim_len, 0xFFFF);
		if ( res == 0 ) {//если получили ошибку калибровки
			g.alarm = 3000;//заведем время отображения временного сообщения в мс
			pmenu = message_1_M;
			return 0;
		}
		pmenu = calib100M;
		return 0;
	}
	clrscr();
	toPrint("Измерте показание на воздухе, нажмите кнопку \r\n");
	uint32_t val = g.tim_len;
	uint32_to_str( val );
	toPrint(" y.e. \r\n");
	return 1;
}

int calib100M(uint8_t ev) {
	if ( ev == Eb1Long ) {
		pmenu = mainM;
		return 0;//перерисовывать не надо
	}
	if ( ev == Eb1Click ) {
		pmenu = calib200M;
		return 0;
	}
	clrscr();
	toPrint("Put feeler on 100 um and push button \r\n");
	uint32_t val = g.tim_len;
	uint32_to_str( val );
	toPrint(" y.e. \r\n");
	return 1;
}

int calib200M(uint8_t ev) {
	if ( ev == Eb1Long ) {
		pmenu = mainM;
		return 0;//перерисовывать не надо
	}
	if ( ev == Eb1Click ) {
		pmenu = calib300M;
		return 0;
	}
	clrscr();
	toPrint("Put feeler on 200 um and push button \r\n");
	return 1;
}

int calib300M(uint8_t ev) {
	if ( ev == Eb1Long ) {
		pmenu = mainM;
		return 0;//перерисовывать не надо
	}
	if ( ev == Eb1Click ) {
		pmenu = mainM;
		return 0;
	}
	clrscr();
	toPrint("Put feeler on 300 um and push button \r\n");
	return 1;
}

