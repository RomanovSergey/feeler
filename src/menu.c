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

void showVal(uint32_t val) {
	//static uint16_t cnt = 0;
	clrscr();
	toPrint("\r\n Tim_len = ");//=================================
	//toPrint("\033[31m");//set red color
	uint32_to_str( val );
	//toPrint("\033[0m");//reset normal (color also default)
	toPrint(" y.e. \r\n");
	toPrint(" microns = ");
	uint16_t microValue = micro( val );
	if ( microValue == 0xFFFF ) {
		toPrint("Air \r\n");
	} else {
		uint32_to_str( microValue );
		toPrint(" um \r\n");
	}
	//toPrint("\r\n cnt = ");
	//uint16_to_5str( cnt++ );
}

/*
 * Рабочий - отображает измеренное значение толщины
 * по мере поступления новых данных
 */
int workScreenM(uint8_t ev) {
	switch (ev) {
	case Eb1Click:
	case Eb1Long:
		return 0;//ignore
	case Eb1Double:
		pmenu = mainM;//указатель на процедурку калибровки
		return 0;//перерисовывать не надо
	case Eb1Push:
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
	switch (ev) {
	case Eb1Pull:
		pmenu = workScreenM;
		keepVal = 0;
		return 0;
	case Eb1Long:
		return 0;
	case Erepaint:
		keepVal = g.tim_len;
		break;
	}

	showVal( keepVal );
	return 1;//надо перерисовать
}

/*
 * Главное меню
 */
int mainM(uint8_t ev) {
	static uint8_t curs = 1;
	switch (ev) {
	case Eb1Long:
		pmenu = workScreenM;
		return 0;
	case Eb1Double:
		if ( curs == 1 ) {
			pmenu = notDoneM;
		} else if ( curs == 2 ) {
			pmenu = calib__0M;//указатель на процедурку калибровки
		} else if ( curs == 3 ) {
			pmenu = notDoneM;
		} else {
			pmenu = messageError1M;
		}
		return 0;
	case Eb1Click:
		curs++;
		if ( curs == 4 ) {
			curs = 1;
		}
		break;
	case Erepaint:
		curs = 1;
		break;
	}

	clrscr();
	toPrint("Главное меню\r\n");
	curs==1 ? toPrint(">") : toPrint(" ") ; toPrint("Выбор рабочей калибровки\r\n");
	curs==2 ? toPrint(">") : toPrint(" ") ; toPrint("Пользовательская калибровка\r\n");
	curs==3 ? toPrint(">") : toPrint(" ") ; toPrint("Просмотр таблиц\r\n");
	return 1;
}

/*
 * Функция для вывода временного сообщения 1
 */
int messageError1M(uint8_t ev) {
	switch (ev) {
	case Ealarm:
		pmenu = workScreenM;
		return 0;
	case Emeasure:
		return 0;
	case Erepaint:
		g.alarm = 4000;//заведем время отображения данного сообщения в мс
		break;
	}
	clrscr();
	toPrint("Error error error!!!\r\n");
	toPrint("Вы сделали что то не хорошее\r\n");
	return 1;
}

int calib__0M(uint8_t ev) {
	switch (ev) {
	case Eb1Long:
		pmenu = workScreenM;
		return 0;
	case Eb1Click: {
		int res = addCalibPoint(g.tim_len, 0);
		if ( res == 0 ) {//если получили ошибку калибровки
			pmenu = messageError1M;
			return 0;
		}
		pmenu = calib100M; }
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
	switch (ev) {
	case Eb1Long:
		pmenu = workScreenM;
		return 0;
	case Eb1Click: {
		int res = addCalibPoint(g.tim_len, 100);
		if ( res == 0 ) {//если получили ошибку калибровки
			pmenu = messageError1M;
			return 0;
		}
		pmenu = calib200M; }
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
	switch (ev) {
	case Eb1Long:
		pmenu = workScreenM;
		return 0;
	case Eb1Click: {
		int res = addCalibPoint(g.tim_len, 200);
		if ( res == 0 ) {//если получили ошибку калибровки
			pmenu = messageError1M;
			return 0;
		}
		pmenu = calib300M; }
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
	switch (ev) {
	case Eb1Long:
		pmenu = workScreenM;
		return 0;
	case Eb1Click: {
		int res = addCalibPoint(g.tim_len, 300);
		if ( res == 0 ) {//если получили ошибку калибровки
			pmenu = messageError1M;
			return 0;
		}
		pmenu = calib400M; }
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
	switch (ev) {
	case Eb1Long:
		pmenu = workScreenM;
		return 0;
	case Eb1Click: {
		int res = addCalibPoint(g.tim_len, 400);
		if ( res == 0 ) {//если получили ошибку калибровки
			pmenu = messageError1M;
			return 0;
		}
		pmenu = calib600M; }
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
	switch (ev) {
	case Eb1Long:
		pmenu = workScreenM;
		return 0;
	case Eb1Click: {
		int res = addCalibPoint(g.tim_len, 600);
		if ( res == 0 ) {//если получили ошибку калибровки
			pmenu = messageError1M;
			return 0;
		}
		pmenu = calibMaxM; }
		return 0;
	}
	clrscr();
	toPrint("Измерте на пластине 600 мкм и нажмите кнопку \r\n");
	uint32_t val = g.tim_len;
	uint32_to_str( val );
	toPrint(" y.e. \r\n");
	return 1;
}

int calibMaxM(uint8_t ev) {
	switch (ev) {
	case Eb1Long:
		pmenu = workScreenM;
		return 0;
	case Eb1Click: {
		int res = addCalibPoint(g.tim_len, 5000);
		if ( res == 0 ) {//если получили ошибку калибровки
			pmenu = messageError1M;
			return 0;
		}
		pmenu = calibDoneM; }
		return 0;
	}
	clrscr();
	toPrint("Измерте на пластине 5000 мкм и нажмите кнопку \r\n");
	uint32_t val = g.tim_len;
	uint32_to_str( val );
	toPrint(" y.e. \r\n");
	return 1;
}

int calibDoneM(uint8_t ev) {
	switch (ev) {
	case Erepaint:
		g.alarm = 5000;//заведем время отображения данного сообщения в мс
		break;
	case Ealarm:
		pmenu = workScreenM;
		return 0;
	case Emeasure:
		return 0;
	}

	clrscr();
	toPrint("Поздравляю! \r\n");
	toPrint("Процесс калибровки завершен \r\n");
	return 1;
}

int notDoneM(uint8_t ev) {
	switch (ev) {
	case Ealarm:
		pmenu = workScreenM;
		return 0;
	case Emeasure:
		return 0;
	case Erepaint:
		g.alarm = 3000;//заведем время отображения данного сообщения в мс
		break;
	}

	clrscr();
	toPrint("Sorry \r\n");
	toPrint("Но данный пункт меню еще не существует. \r\n");
	return 1;
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
