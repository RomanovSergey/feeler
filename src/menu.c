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


inline void clrscr(void) {
	tx.ind = 0;//индекс буфера для отправки в порт сбрасываем в ноль
	toPrint("\033[2J");//clear entire screen
	toPrint("\033[?25l");//Hides the cursor.
	toPrint("\033[H");//Move cursor to upper left corner.
	printRun();//крутящаяся черточка
	toPrint("\r\n");
}

int showEventM(uint8_t ev) {
	if ( ev == Eb1Click ) {
		clrscr();
		toPrint(" Click \r\n");
		return 1;
	}
	if ( ev == Eb1Double ) {
		clrscr();
		toPrint(" Double \r\n");
		return 1;
	}
	if ( ev == Eb1Long ) {
		clrscr();
		toPrint(" Long Push \r\n");
		return 1;
	}
	if ( ev == Eb1Push ) {
		clrscr();
		toPrint(" Push button \r\n");
		return 1;
	}
	if ( ev == Eb1Pull ) {
		clrscr();
		toPrint(" Pull button \r\n");
		return 1;
	}
//	if ( ev == Emeasure ) {
//		//clrscr();
//		toPrint(" Measure \r\n");
//		return 1;
//	}
	if ( ev == Erepaint ) {
		clrscr();
		toPrint(" Repaint \r\n");
		return 1;
	}
	if ( ev == Ealarm ) {
		clrscr();
		toPrint(" Alarm \r\n");
		return 1;
	}
	return 0;
}

void showVal(uint32_t val) {
	//static uint16_t cnt = 0;
	clrscr();
	toPrint("\r\n Tim_len = ");//=================================
	//toPrint("\033[31m");//set red color
	uint32_to_str( val );
	//toPrint("\033[0m");//reset normal (color also default)
	toPrint(" y.e. \r\n");
	toPrint(" microns = ");
	uint32_to_str( micro( val ) );
	toPrint(" um \r\n");
	//toPrint("\r\n cnt = ");
	//uint16_to_5str( cnt++ );
}

/*
 * Первая функция меню - отображает измеренное значение толщины
 * по мере поступления новых данных
 */
int mainM(uint8_t ev) {
	if ( ev == Eb1Click || ev == Eb1Long ) {//событие нажания кнопки
		return 0;//ниче не делаем
	}
	if ( ev == Eb1Double ) {//если поступило событие длительного нажатия кнопки
		pmenu = calibAirM;//указатель на процедурку калибровки
		return 0;//перерисовывать не надо
	}
	if ( ev == Eb1Push ) {
		pmenu = keepValM;
		return 0;
	}
	showVal( g.tim_len );
	return 1;//надо перерисовать
}

/*
 * Фикирует измеренное значение, пока нажата кнопка
 */
int keepValM(uint8_t ev) {
	static uint32_t keepVal = 0;
	if ( ev == Eb1Pull ) {
		pmenu = mainM;
		keepVal = 0;
		return 0;
	}
	if ( ev == Eb1Long ) {
		return 0;//ниче не делаем
	}
	if ( ev == Erepaint ) {
		keepVal = g.tim_len;
	}
	showVal( keepVal );
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
		int res = addCalibPoint(g.tim_len, 0xFFFF);
		if ( res == 0 ) {//если получили ошибку калибровки
			g.alarm = 5000;//заведем время отображения временного сообщения в мс
			pmenu = message_1_M;
			return 0;
		}
		pmenu = calib__0M;
		return 0;
	}
	clrscr();
	toPrint("Измерте показание на воздухе, нажмите кнопку \r\n");
	uint32_t val = g.tim_len;
	uint32_to_str( val );
	toPrint(" y.e. \r\n");
	return 1;
}

int calib__0M(uint8_t ev) {
	if ( ev == Eb1Long ) {
		pmenu = mainM;
		return 0;//перерисовывать не надо
	}
	if ( ev == Eb1Click ) {
		int res = addCalibPoint(g.tim_len, 0);
		if ( res == 0 ) {//если получили ошибку калибровки
			g.alarm = 5000;//заведем время отображения временного сообщения в мс
			pmenu = message_1_M;
			return 0;
		}
		pmenu = calib100M;
		return 0;
	}
	clrscr();
	toPrint("Измерте показание на образце без зазора и нажмите кнопку \r\n");
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
		int res = addCalibPoint(g.tim_len, 100);
		if ( res == 0 ) {//если получили ошибку калибровки
			g.alarm = 5000;//заведем время отображения временного сообщения в мс
			pmenu = message_1_M;
			return 0;
		}
		pmenu = calib200M;
		return 0;
	}
	clrscr();
	toPrint("Измерте на пластине 100 мкм и нажмите кнопку \r\n");
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
		int res = addCalibPoint(g.tim_len, 200);
		if ( res == 0 ) {//если получили ошибку калибровки
			g.alarm = 5000;//заведем время отображения временного сообщения в мс
			pmenu = message_1_M;
			return 0;
		}
		pmenu = calib300M;
		return 0;
	}
	clrscr();
	toPrint("Измерте на пластине 200 мкм и нажмите кнопку \r\n");
	uint32_t val = g.tim_len;
	uint32_to_str( val );
	toPrint(" y.e. \r\n");
	return 1;
}

int calib300M(uint8_t ev) {
	if ( ev == Eb1Long ) {
		pmenu = mainM;
		return 0;//перерисовывать не надо
	}
	if ( ev == Eb1Click ) {
		int res = addCalibPoint(g.tim_len, 300);
		if ( res == 0 ) {//если получили ошибку калибровки
			g.alarm = 5000;//заведем время отображения временного сообщения в мс
			pmenu = message_1_M;
			return 0;
		}
		pmenu = calib400M;
		return 0;
	}
	clrscr();
	toPrint("Измерте на пластине 300 мкм и нажмите кнопку \r\n");
	uint32_t val = g.tim_len;
	uint32_to_str( val );
	toPrint(" y.e. \r\n");
	return 1;
}

int calib400M(uint8_t ev) {
	if ( ev == Eb1Long ) {
		pmenu = mainM;
		return 0;//перерисовывать не надо
	}
	if ( ev == Eb1Click ) {
		int res = addCalibPoint(g.tim_len, 400);
		if ( res == 0 ) {//если получили ошибку калибровки
			g.alarm = 5000;//заведем время отображения временного сообщения в мс
			pmenu = message_1_M;
			return 0;
		}
		pmenu = calib600M;
		return 0;
	}
	clrscr();
	toPrint("Измерте на пластине 400 мкм и нажмите кнопку \r\n");
	uint32_t val = g.tim_len;
	uint32_to_str( val );
	toPrint(" y.e. \r\n");
	return 1;
}

int calib600M(uint8_t ev) {
	if ( ev == Eb1Long ) {
		pmenu = mainM;
		return 0;//перерисовывать не надо
	}
	if ( ev == Eb1Click ) {
		int res = addCalibPoint(g.tim_len, 600);
		if ( res == 0 ) {//если получили ошибку калибровки
			g.alarm = 5000;//заведем время отображения временного сообщения в мс
			pmenu = message_1_M;
			return 0;
		}
		g.alarm = 5000;
		pmenu = calibDoneM;
		return 0;
	}
	clrscr();
	toPrint("Измерте на пластине 600 мкм и нажмите кнопку \r\n");
	uint32_t val = g.tim_len;
	uint32_to_str( val );
	toPrint(" y.e. \r\n");
	return 1;
}

int calibDoneM(uint8_t ev) {
	if ( ev == Ealarm ) {
		pmenu = mainM;
		return 0;
	}
	if ( ev == Emeasure ) {
		return 0;
	}
	clrscr();
	toPrint("Поздравляю! \r\n");
	toPrint("Процесс калибровки завершен \r\n");
	return 1;
}

