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
#include "flash.h"

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
		pwrPutEv( PWR_ALARM_3000 ); //заведем будильник отображения данного сообщения
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
		//g.air = getFreq();
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
	if ( magGetStat() ) {
		disPrin(" *");
	}
	disUINT32_to_strFONT2(1, 0, val); // freq

	uint16_t microValue = micro( val );
	if ( microValue == 0xFFFF ) {
		disPrint(4,0,"Air");
	} else {
		disUINT32_to_str(5, 0, microValue );
		disPrin(" um");
	}/**/
}

/*
 * Рабочий - отображает измеренное значение толщины
 * по мере поступления новых данных
 */
int dworkScreen(uint8_t ev) {
	switch (ev) {
	case DIS_PUSH_OK:
		pdisp = dmainM;//на главное меню
		mgPutEv( MG_OFF );
		return 0;
	case DIS_PUSH_L:
		mgPutEv( MG_ON );
		return 0;
	case DIS_PULL_L:
		mgPutEv( MG_OFF );
		return 0;
	case DIS_PUSH_R:
	case DIS_LONGPUSH_OK:
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
			curs = 0;
		} else if ( curs == 1 ) { // Выбор калибровки
			prev  = dmainM;
			pdisp = dflashDebug;//dnotDone;
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
		if ( curs == 0 ) { // Наверх
			pdisp = dmainM;
			curs = 0;
		} else if ( curs == 1 ) { // Железо
			pdisp  = dcalibFe;
			mgPutEv( MG_ON );
		} else if ( curs == 2 ) { // Алюминий
			pdisp  = dcalibAl;
			mgPutEv( MG_ON );
		} else {
			prev  = duserCalib;
			pdisp = dmessageError1;
			curs = 0;
		}
		return 0;
	}
	disClear();
	disPrint(0,0,"Клаибровка");
	disPrint(1,6,  "Наверх");
	disPrint(2,6,  "Железо");
	disPrint(3,6,  "Алюминий");
	disPrint( curs + 1, 0, "→");
	return 1;
}

int dcalibDone(uint8_t ev) {
	switch (ev) {
	case DIS_REPAINT:
		pwrPutEv( PWR_ALARM_3000 ); //заведем будильник отображения данного сообщения
		break;
	case DIS_ALARM://сработал будильник
		if ( prev != NULL ) {
			pdisp = prev;
			prev = NULL;
		} else {
			pdisp = duserCalib;
		}
		return 0;
	case DIS_MEASURE:
		return 0;
	}
	disClear();
	disPrint(0,0,"Клаибровка:");
	disPrint(1,12,"Процесс");
	disPrint(2,12,"завершен");
	disPrint(3,12,"успешно.");
	return 1;
}

int calib(uint8_t ev, int metall) {
	static const uint16_t thickness[] = {0,100,200,300,500,1000,2000,3000};
	static int index = 0;
	int res = 0;

	if ( metall != 0 && metall != 1 ) {
		mgPutEv( MG_OFF );
		pdisp = dmessageError1;
		prev  = duserCalib;
		return 0;
	}
	switch (ev) {
	case DIS_PUSH_L:
		mgPutEv( MG_OFF );
		pdisp = duserCalib;
		return 0;
	case DIS_REPAINT:
		index = 0;
		break;
	case DIS_PUSH_OK: //Eb1Click:
		res = addCalibPoint( getFreq(), thickness[index], metall );
		if ( res == 0 ) {//если получили ошибку калибровки
			mgPutEv( MG_OFF );
			pdisp = dmessageError1;
			prev  = duserCalib;
			return 0;
		}
		index++;
		if ( index == sizeof(thickness)/(sizeof(uint16_t)) ) {
			index = 0;
			mgPutEv( MG_OFF );
			pdisp = dcalibDone;
			return 0;
		}
		break;
	}
	disClear();
	if ( metall == 0 ) {
		disPrint(0,0,"Клаибровка Fe");
	} else {
		disPrint(0,0,"Клаибровка Al");
	}
	disPrint(1,0,"Измерте зазор:");
	disUINT32_to_str(2,0, thickness[index] ); //uint32_to_str( thickness[index] );
	disPrin(" мкм"); //toPrint(" мкм, кликните.\r\n");

	disPrint(3,0,"F=");
	uint32_t val = getFreq();
	disUINT32_to_str(3, 0xFF, val); //uint32_to_str( val );
	//toPrint(" y.e. \r\n");
	return 1;
}

/*
 * Калибровка по железу
 */
int dcalibFe(uint8_t ev) {
	return calib(ev, 0);
}

/*
 * Калибровка по алюминию
 */
int dcalibAl(uint8_t ev) {
	return calib(ev, 1);
}

/*
 * Эксперименты с флэш памятью
 */
int dflashDebug(uint8_t ev)
{
	static uint8_t curs = 0;

	switch (ev) {
	case DIS_PUSH_L:
		if ( curs > 0 ) {
			curs--;
		} else {
			curs = 4;
		}
		break;
	case DIS_PUSH_R:
		curs++;
		if ( curs > 4 ) {
			curs = 0;
		}
		break;
	case DIS_PUSH_OK:
		if ( curs == 0 ) { // Наверх
			pdisp = dmainM;
			curs = 0;
		} else if ( curs == 1 ) { // Show
			pdisp  = dflashShow;
		} else if ( curs == 2 ) { // Write inc
			//pdisp  = ;
		} else {
			//prev  = duserCalib;
			//pdisp = dmessageError1;
			//curs = 0;
		}
		return 0;
	}
	disClear();
	disPrint(0,0,"Flash Debug");
	disPrint(1,6,  "Наверх");
	disPrint(2,6,  "Show");
	disPrint(3,6,  "Write inc");
	disPrint(4,6,  "Zero  inc");
	disPrint(5,6,  "Erase page");
	disPrint( curs + 1, 0, "→");
	return 1;
}

/*
 * Выводит начало содержимого последней страницы
 */
int dflashShow(uint8_t ev)
{
	switch (ev) {
	case DIS_PUSH_L:
		pdisp = dflashDebug;
		break;
	}
	disClear();
	disPrint(0,0,"Show Flash");
	{
		uint32_t addr = 0x0800F800;
		uint16_t val;
		for ( int i = 1; i < 6; i++ ) { // 5 строчек
			for ( int k = 0; k < 3; k++) {
				val = fread16( addr );
				addr += 2;
				disHexHalfWord( i, k*5*6, val );
			}
		}
	}
	return 1;
}

//===========================================================================
//===========================================================================
///*
// * Фикирует измеренное значение, пока нажата кнопка
// */
//int dkeepVal(uint8_t ev) {
//	static uint32_t keepVal = 0;
//	switch (ev) {
//	case DIS_PULL_L:
//		pdisp = workScreenM;
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

