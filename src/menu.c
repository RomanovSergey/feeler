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
#include "magnetic.h"
#include "displayDrv.h"
#include "pwr.h"
#include "sound.h"

static pdisp_t prev = NULL;

/*
 * Не существующее меню
 */
int dnotDone(uint8_t ev) {
	switch (ev) {
	case DIS_ALARM:
		if ( prev != NULL ) {
			pdisp = prev;
			prev = NULL;
		} else {
			pdisp = dworkScreen;
		}
		return 0;
	case DIS_MEASURE:
		return 0;
	case DIS_REPAINT:
		pwrPutEv( PWR_ALARM_3000 );//заведем будильник отображения данного сообщения
		break;
	}

	disClear();
	disPrint(0, 27, "Sorry");
	disPrint(1, 6,  "Данный пункт");
	disPrint(2, 9,  "меню еще не");
	disPrint(3, 12, "существует");
	return 1;
}

/*
 * Функция для вывода временного сообщения 1
 */
int dmessageError1(uint8_t ev) {
	switch (ev) {
	case DIS_ALARM://сработал будильник
		if ( prev != NULL ) {
			pdisp = prev;
			prev = NULL;
		} else {
			pdisp = dworkScreen;
		}
		return 0;
	case DIS_MEASURE:
		return 0;
	case DIS_REPAINT:
		pwrPutEv( PWR_ALARM_3000 );//заведем будильник отображения данного сообщения
		break;
	}
	disClear();
	disPrint(0,0,"Error error!!!");
	disPrint(1,0,"Вы сделали");
	disPrint(2,0,"что то не");
	disPrint(3,0,"хорошее");
	return 1;
}

int emptyDisplay(uint8_t event) {
	if ( event == DIS_PAINT ) {
		pdisp = dworkScreen;
		sndPutEv( SND_PERMIT );
		return 0;
	}
	disClear();
	return 1;
}

/*
 * Отображается при включении питания
 * просит замерить частоту на воздухе
 */
int dPowerOn(uint8_t ev) {
	switch (ev) {
	case DIS_PUSH_OK:
		g.air = getFreq();
		pdisp = dworkScreen;
		return 0;
	}
	disClear();
	disPrint(0,0,"Замерте воздух");
	disPrint(1,0,"  нажмите OK");
	disPrint(2,0,"F = ");
	disUINT32_to_str(2,0xFF, getFreq() );
	disPrin(" y.e.");
	return 1;
}

void dshowV(uint32_t val) { //из dworkScreen()
	disClear();
	disPrint(0, 0, "ИЗМЕРЕНИЕ");

	disUINT32_to_strFONT2(1, 0, val); // freq

	/*uint16_t microValue = micro( val );
	if ( microValue == 0xFFFF ) {
		disPrint(4,0,"Air");
	} else {
		disUINT32_to_str(3, 24, microValue );
		disPrin(" um");
	}*/
}

/*
 * Рабочий - отображает измеренное значение толщины
 * по мере поступления новых данных
 */
int dworkScreen(uint8_t ev) {
	switch (ev) {
	case DIS_PUSH_OK:
		pdisp = dmainM;//на главное меню
		sndPutEv( SND_BEEP );
		mgPutEv( MG_OFF );
		return 0;

	case DIS_PUSH_L:
		mgPutEv( MG_ON );
		break;
	case DIS_PULL_L:
		mgPutEv( MG_OFF );
		break;
	case DIS_LONGPUSH_OK:
	case DIS_PUSH_R:
	case DIS_LONGPUSH_L:
	case DIS_LONGPUSH_R:
		return 0;
	}
	dshowV( getFreq() );
	return 1;//надо перерисовать
}

/*
 * Главное меню
 */
int dmainM(uint8_t ev) {
	static int8_t curs = 0;
	switch (ev) {
	case DIS_PUSH_OK:
		if ( curs == 0 ) {        // Наверх
			pdisp = dworkScreen;
			sndPutEv( SND_BEEP );
			curs = 0;
		} else if ( curs == 1 ) { // Выбор калибровки
			prev  = dmainM;
			pdisp = dnotDone;
		} else if ( curs == 2 ) { // Польз. калибр.
			pdisp = duserCalib;
		} else if ( curs == 3 ) { // Просмотр таб.
			prev  = dmainM;
			pdisp = dnotDone;
		} else {
			prev  = dmainM;
			pdisp = dnotDone;
			curs = 0;
		}
		return 0;
	case DIS_PUSH_L:
		if ( curs > 0 ) {
			curs--;
		} else {
			curs = 3;
		}
		break;
	case DIS_PUSH_R:
		curs++;
		if ( curs == 4 ) {
			curs = 0;
		}
		break;
	case DIS_MEASURE:
	case DIS_LONGPUSH_L:
	case DIS_LONGPUSH_R:
		return 0;
	}
	disClear();
	disPrint(0,0,"Главное меню");
	disPrint(1,6, "Наверх");
	disPrint(2,6, "Выбор калибр.");
	disPrint(3,6, "Польз.калибр.");
	disPrint(4,6, "Просмотр таб.");
	disPrint( curs + 1, 0, "→");
	return 1;
}

/*
 * Пользовательская калибровка
 */
int duserCalib(uint8_t ev) {
	static uint8_t curs = 0;
	switch (ev) {
	case DIS_PUSH_L:
		if ( curs > 0 ) {
			curs--;
		} else {
			curs = 2;
		}
		break;
	case DIS_PUSH_R:
		curs++;
		if ( curs > 2 ) {
			curs = 0;
		}
		break;
	case DIS_PUSH_OK:
		if ( curs == 0 ) {
			pdisp = dmainM;
			curs = 0;
		} else if ( curs == 1 ) {
			prev  = duserCalib;
			pdisp = dnotDone;
		} else if ( curs == 2 ) {
			prev  = duserCalib;
			pdisp = dnotDone;
		} else {
			prev  = duserCalib;
			pdisp = dmessageError1;
			curs = 1;
		}
		return 0;
	}
	disClear();
	disPrint(0,0,"Клаибровка");
	disPrint(1,6,"Наверх");
	disPrint(2,6,"Железо");
	disPrint(3,6,"Алюминий");
	disPrint( curs + 1, 0, "→");
	return 1;
}

//===========================================================================
//===========================================================================
//
//inline void clrscr(void) {
//	tx.ind = 0;//индекс буфера для отправки в порт сбрасываем в ноль
//	toPrint("\033[2J");//clear entire screen
//	toPrint("\033[?25l");//Hides the cursor.
//	toPrint("\033[H");//Move cursor to upper left corner.
//	printRun();//крутящаяся черточка
//	toPrint("\r\n");
//}
//
///*
// * Отображается при включении питания
// * просит замерить частоту на воздухе
// */
//int powerOn(uint8_t ev) {
//	switch (ev) {
//	case Eb1Long:
//	case Eb1Pull:
//		return 0;//do not paint
//	case Eb1Double:
//	case Eb1Push:
//	case Eb1Click:
//		g.air = getFreq();
//		pmenu = workScreenM;
//		return 0;
//	}
//	clrscr();
//	toPrint("Замерте показание на воздухе\r\n");
//	toPrint(" и кликните на кнопку.\r\n");
//	toPrint("\r\n Freq = ");
//	uint32_to_str( getFreq() );
//	toPrint(" y.e. \r\n");
//	return 1;
//}
//
//void showVal(uint32_t val) {
//	clrscr();
//	toPrint("\r\n Freq = ");
//	uint32_to_str( val );
//	toPrint(" y.e. \r\n");
//	toPrint(" microns = ");
//	uint16_t microValue = micro( val );
//	if ( microValue == 0xFFFF ) {
//		toPrint("Air \r\n");
//	} else {
//		uint32_to_str( microValue );
//		toPrint(" um \r\n");
//	}
//}
//
///*
// * Рабочий - отображает измеренное значение толщины
// * по мере поступления новых данных
// */
//int workScreenM(uint8_t ev) {
//	switch (ev) {
//	case Eb1Click:
//	case Eb1Long:
//		return 0;
//	case Eb1Double:
//		pmenu = mainM;//на главное меню
//		return 0;
//	case Eb1Push:
//		pmenu = keepValM;
//		return 0;
//	}
//
//	showVal( getFreq() );
//	return 1;//надо перерисовать
//}
//
///*
// * Фикирует измеренное значение, пока нажата кнопка
// */
//int keepValM(uint8_t ev) {
//	static uint32_t keepVal = 0;
//	switch (ev) {
//	case Eb1Pull:
//		pmenu = workScreenM;
//		keepVal = 0;
//		return 0;
//	case Eb1Long:
//		return 0;
//	case Erepaint:
//		keepVal = getFreq();
//		break;
//	}
//
//	showVal( keepVal );
//	return 1;//надо перерисовать
//}
//
///*
// * Главное меню
// */
//int mainM(uint8_t ev) {
//	static uint8_t curs = 1;
//	switch (ev) {
//	case Eb1Long:
//		pmenu = workScreenM;
//		curs = 1;
//		return 0;
//	case Eb1Double:
//		if ( curs == 1 ) {
//			pmenu = notDoneM;
//			curs = 1;
//		} else if ( curs == 2 ) {
//			pmenu = userCalibM;
//		} else if ( curs == 3 ) {
//			pmenu = notDoneM;
//			curs = 1;
//		} else {
//			curs = 1;
//		}
//		return 0;
//	case Eb1Click:
//		curs++;
//		if ( curs == 4 ) {
//			curs = 1;
//		}
//		break;
//	}
//
//	clrscr();
//	toPrint("Главное меню\r\n");
//	curs==1 ? toPrint(">") : toPrint(" ") ; toPrint("Выбор рабочей калибровки\r\n");
//	curs==2 ? toPrint(">") : toPrint(" ") ; toPrint("Пользовательская калибровка\r\n");
//	curs==3 ? toPrint(">") : toPrint(" ") ; toPrint("Просмотр таблиц\r\n");
//	return 1;
//}
//
///*
// * Функция для вывода временного сообщения 1
// */
//int messageError1M(uint8_t ev) {
//	switch (ev) {
//	case Ealarm://сработал будильник
//		pmenu = workScreenM;
//		return 0;
//	case Emeasure:
//		return 0;
//	case Erepaint:
//		g.alarm = 3000;//заведем будильник в мс
//		break;
//	}
//	clrscr();
//	toPrint("Error error error!!!\r\n");
//	toPrint("Вы сделали что то не хорошее\r\n");
//	return 1;
//}
//
///*
// * Пользовательская калибровка
// */
//int userCalibM(uint8_t ev) {
//	static uint8_t curs = 1;
//	switch (ev) {
//	case Eb1Long:
//		pmenu = mainM;
//		curs = 1;
//		return 0;
//	case Eb1Click:
//		curs++;
//		if ( curs > 2 ) {
//			curs = 1;
//		}
//		break;
//	case Eb1Double:
//		if ( curs == 1 ) {
//			pmenu = calibFeM;
//		} else if ( curs == 2 ) {
//			pmenu = calibAlM;
//		} else {
//			pmenu = messageError1M;
//			curs = 1;
//		}
//		return 0;
//	}
//	clrscr();
//	toPrint("Пользовательская клаибровка\r\n");
//	curs==1 ? toPrint(">") : toPrint(" ") ; toPrint("Железо\r\n");
//	curs==2 ? toPrint(">") : toPrint(" ") ; toPrint("Алюминий\r\n");
//	return 1;
//}
//
//int calib(uint8_t ev, int metall) {
//	static const uint16_t thickness[] = {0,100,200,300,500,1000,2000,3000};
//	static int index = 0;
//	int res = 0;
//
//	if ( metall != 0 && metall != 1 ) {
//		pmenu = messageError1M;
//		return 0;
//	}
//	switch (ev) {
//	case Eb1Long:
//		pmenu = userCalibM;
//		return 0;
//	case Erepaint:
//		index = 0;
//		break;
//	case Eb1Click:
//		res = addCalibPoint( getFreq(), thickness[index], metall );
//		if ( res == 0 ) {//если получили ошибку калибровки
//			pmenu = messageError1M;
//			return 0;
//		}
//		index++;
//		if ( index == sizeof(thickness)/(sizeof(uint16_t)) ) {
//			index = 0;
//			pmenu = calibDoneM;
//			return 0;
//		}
//		break;
//	}
//	clrscr();
//	if ( metall == 0 ) {
//		toPrint("Калибровка по железу\r\n");
//	} else {
//		toPrint("Калибровка по алюминию\r\n");
//	}
//	toPrint("Измерте зазор ");
//	uint32_to_str( thickness[index] );
//	toPrint(" мкм, кликните.\r\n");
//	uint32_t val = getFreq();
//	uint32_to_str( val );
//	toPrint(" y.e. \r\n");
//	return 1;
//}
//
///*
// * Калибровка по железу
// */
//int calibFeM(uint8_t ev) {
//	return calib(ev, 0);
//}
//
///*
// * Калибровка по алюминию
// */
//int calibAlM(uint8_t ev) {
//	return calib(ev, 1);
//}
//
//int calibDoneM(uint8_t ev) {
//	switch (ev) {
//	case Erepaint:
//		g.alarm = 3000;//заведем время отображения данного сообщения в мс
//		break;
//	case Ealarm:
//		pmenu = userCalibM;
//		return 0;
//	case Emeasure:
//		return 0;
//	}
//
//	clrscr();
//	toPrint("Поздравляю! \r\n");
//	toPrint("Процесс калибровки завершен \r\n");
//	return 1;
//}
//
//int notDoneM(uint8_t ev) {
//	switch (ev) {
//	case Ealarm:
//		pmenu = workScreenM;
//		return 0;
//	case Emeasure:
//		return 0;
//	case Erepaint:
//		g.alarm = 3000;//заведем время отображения данного сообщения в мс
//		break;
//	}
//
//	clrscr();
//	toPrint("Sorry \r\n");
//	toPrint("Но данный пункт меню еще не существует. \r\n");
//	return 1;
//}
//
//int showEventM(uint8_t ev) {
//	if ( ev == Eb1Click ) {
//		clrscr();
//		toPrint(" Click \r\n");
//		return 1;
//	}
//	if ( ev == Eb1Double ) {
//		clrscr();
//		toPrint(" Double \r\n");
//		return 1;
//	}
//	if ( ev == Eb1Long ) {
//		clrscr();
//		toPrint(" Long Push \r\n");
//		return 1;
//	}
//	if ( ev == Eb1Push ) {
//		clrscr();
//		toPrint(" Push button \r\n");
//		return 1;
//	}
//	if ( ev == Eb1Pull ) {
//		clrscr();
//		toPrint(" Pull button \r\n");
//		return 1;
//	}
//	if ( ev == Erepaint ) {
//		clrscr();
//		toPrint(" Repaint \r\n");
//		return 1;
//	}
//	if ( ev == Ealarm ) {
//		clrscr();
//		toPrint(" Alarm \r\n");
//		return 1;
//	}
//	return 0;
//}
