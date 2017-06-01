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
#include "flash2.h"
#include "fonts/img.h"
#include <string.h>
#include "adc.h"

static pdisp_t prev = NULL;

/*
 * Start menu function on Power On
 */
int emptyDisplay(uint8_t event) {
	if ( event == DIS_PAINT ) {
		pdisp = dPowerOn;
		sndPutEv( SND_PERMIT );
		return 0;
	}
	disClear();
	return 1;
}

/*
 * Не существующее меню
 */
int dnotDone(uint8_t ev) {
	switch (ev) {
	case DIS_PUSH_L:
	case DIS_ALARM:
		pwrPutEv( PWR_RESET_ALARM );
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
	case DIS_PUSH_L:
	case DIS_ALARM://сработал будильник
		pwrPutEv( PWR_RESET_ALARM );
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
	disPrint(0,18,"Error!");
	disPrint(3,12,"Ошибка!");
	return 1;
}

/*
 * Отображается при включении питания
 * просит замерить частоту на воздухе
 */
int dPowerOn(uint8_t ev) {
	switch (ev) {
	case DIS_REPAINT:
		mgPutEv( MG_ON );
		break;
	case DIS_PUSH_OK:
		if ( microSetAir( getFreq() ) != 0 ) {
			mgPutEv( MG_OFF );
			pdisp = dmessageError1;
			return 0;
		}
		mgPutEv( MG_OFF );
		pdisp = dworkScreen;
		return 0;
	}
	disClear();
	disPrint(0,0,"Замерте воздух");
	disPrint(1,0,"  нажмите OK");
	disPrint(3,0,"F = ");
	disUINT32_to_str(3,0xFF, getFreq() );
	disPrin(" Гц");
	disPrint(5, 36, "OK");
	return 1;
}

/*
 * Рабочий - отображает измеренное значение толщины
 * по мере поступления новых данных
 */
int dworkScreen(uint8_t ev) {
	static uint8_t measflag = 0;
	switch (ev) {
	case DIS_PUSH_OK:
		pdisp = dmainM;
		mgPutEv( MG_OFF );
		return 0;
	case DIS_PUSH_L:
		mgPutEv( MG_ON );
		return 0;
	case DIS_PULL_L:
		mgPutEv( MG_OFF );
		return 0;
	case DIS_MEASURE:
		measflag ^= 1;
		break;
	case DIS_PUSH_R:
	case DIS_LONGPUSH_OK:
	case DIS_LONGPUSH_L:
	case DIS_LONGPUSH_R:
		return 0;
	}

	uint16_t freq = getFreq();
	uint16_t microValue;
	int res;

	disClear();
	if ( magGetStat() ) {
		disPrint(0, 0, "Измер.");
		if ( measflag ) {
			disPrin("*");
		}
	} else {
		disPrint(0, 0, "Фикс.");
	}

	disPrint(0, 60, adcGetBattary() );
	disUINT32_to_str( 1, 0, adcData() );

	res = micro( freq, &microValue );
	if ( res == 0 ) { // Ferrum
		disUINT16_4digit_to_strFONT2(1,0, microValue);
		disPrint(3, 48, " um");
		disPrint(3, 72, "Fe");
	} else if ( res == 1 ) { // Air
		disPrintFONT2(1,0," Air");
	} else if ( res == 2 ) { // Aluminum
		disUINT16_4digit_to_strFONT2(1,0, microValue);
		disPrint(3, 48, " um");
		disPrint(3, 72, "Al");
	} else if ( res == 3 ) { // No Fe calib data
		disPrint(2, 0, "No Fe calibr.");
	} else if ( res == 4 ) { // No Al calib data
		disPrint(2, 0, "No Al calibr.");
	} else if ( res == 5 ) { // Freq is zero
		disPrint(3, 0, "No Data");
	}
	disUINT32_to_str(4, 0, freq);

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
			pdisp = adcTest; // dnotDone;//dflashDebug;
		} else if ( curs == 2 ) { // Польз. калибр.
			pdisp = duserCalib;
		} else if ( curs == 3 ) { // Просмотр таб.
			pdisp = dimageShtrih;
		} else {
			prev  = dmainM;
			pdisp = dnotDone;
			curs = 0;
		}
		return 0;
	case DIS_PUSH_L:
		pdisp = dworkScreen; // Наверх
		curs = 0;
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
	disPrint(2,6, "ADC test");
	disPrint(3,6, "Польз.калибр.");
	disPrint(4,6, "Просмотр карт"); // look at table
	disPrint( curs + 1, 0, "→");
	return 1;
}

int adcTest(uint8_t ev) {
	static uint8_t measflag = 0;
	switch (ev) {
	case DIS_PUSH_OK:
		pdisp = dmainM;
		mgPutEv( MG_OFF );
		return 0;
	case DIS_PUSH_L:
		mgPutEv( MG_ON );
		return 0;
	case DIS_PULL_L:
		mgPutEv( MG_OFF );
		return 0;
	case DIS_MEASURE:
		measflag ^= 1;
		break;
	case DIS_PUSH_R:
	case DIS_LONGPUSH_OK:
	case DIS_LONGPUSH_L:
	case DIS_LONGPUSH_R:
		return 0;
	}
	disClear();
	disPrint(0,0,"ADC Test");

	disPrint(1,0,"str=");
	disPrin( adcGetBattary() );

	disPrint(2,0,"data=");
	disUINT32_to_str( 2, 0xFF, adcData() );

	disPrint(3,0,"vincal=");
	disUINT32_to_str( 3, 0xFF, adcVda() );

	return 1;
}

/*
 * Пользовательская калибровка
 */
int duserCalib(uint8_t ev) {
	static uint8_t curs = 0;
	switch (ev) {
	case DIS_PUSH_L:
		pdisp = dmainM; // Наверх
		curs = 0;
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

int dcalibFeDone(uint8_t ev) {
	int res;

	switch (ev) {
	case DIS_MEASURE:
		return 0;
	case DIS_PUSH_L:
		//micro_initCalib();
		pdisp = duserCalib;
		return 0;
	case DIS_PUSH_OK:
		res = microSaveFe();
		if ( res == 0 ) {
			pdisp = dcalibDone;
		} else {
			pdisp = dmessageError1;
			prev = duserCalib;
		}
		return 0;
	}
	disClear();
	disPrint(0,0, "Клаибровка Fe:");
	disPrint(1,12, "Сохранить");
	disPrint(2,0, "калибровочные");
	disPrint(3,18,  "данные?");
	disPrint(5,0, "L-Отм 0k-Сохр");
	return 1;
}

int dcalibAlDone(uint8_t ev) {
	int res;

	switch (ev) {
	case DIS_MEASURE:
		return 0;
	case DIS_PUSH_L:
		//micro_initCalib();
		pdisp = duserCalib;
		return 0;
	case DIS_PUSH_OK:
		res = microSaveAl();
		if ( res == 0 ) {
			pdisp = dcalibDone;
		} else {
			pdisp = dmessageError1;
			prev = duserCalib;
		}
		return 0;
	}
	disClear();
	disPrint(0,0, "Клаибровка Al:");
	disPrint(1,12, "Сохранить");
	disPrint(2,0, "калибровочные");
	disPrint(3,18,  "данные?");
	disPrint(5,0, "L-Отм 0k-Сохр");
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
	case DIS_REPAINT: // at first entry to this menu
		index = 0;
		break;
	case DIS_PUSH_OK: //Eb1Click:
		res = addCalibPoint( getFreq(), thickness[index], index, metall );
		if ( res != 0 ) {
			mgPutEv( MG_OFF );
			pdisp = dmessageError1;
			prev  = duserCalib;
			return 0;
		}
		index++;
		if ( index == sizeof(thickness)/(sizeof(uint16_t)) ) {
			index = 0;
			mgPutEv( MG_OFF );
			if ( metall == 0 ) { // if Ferrum
				pdisp = dcalibFeDone;
				return 0;
			}
			pdisp = dcalibAlDone; // if Aluminum
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

int dflashDebug(uint8_t ev)
{
	static uint8_t curs = 0;
	static uint16_t hw = 0x0A01;

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
			fstat = fwriteInc( hw );
			hw++;
			prev = dflashDebug;
			pdisp  = dstatusFlash;
		} else if ( curs == 3 ) { // Zero  inc
			fstat = fzeroInc();
			prev = dflashDebug;
			pdisp  = dstatusFlash;
		} else if ( curs == 4 ) { // Erase page
			fstat = ferasePage();
			prev = dflashDebug;
			pdisp  = dstatusFlash;
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
} */

/*
 * Выводит начало содержимого последней страницы

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
} */

/*
 * пишет статус операции записи - для отладки

int dstatusFlash(uint8_t ev)
{
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
		pwrPutEv( PWR_ALARM_1000 );//заведем будильник отображения данного сообщения
		break;
	}
	char str[28];
	disClear();
	disPrint(0,0,"Status Flash");
	switch (fstat) {
	case FLASH_BUSY:
		strcpy(str, "BUSY");
		break;
	case FLASH_ERROR_WRP:
		strcpy(str, "ERROR_WRP");
		break;
	case FLASH_ERROR_PROGRAM:
		strcpy(str, "ERR_PROGRAM");
		break;
	case FLASH_COMPLETE:
		strcpy(str, "COMPLETE");
		break;
	case FLASH_TIMEOUT:
		strcpy(str, "TIMEOUT");
		break;
	default:
		if ( fstat == 0 ) {
			strcpy(str, "NO SPACE");
		} else {
			strcpy(str, "UNKNOWN");
		}
	}
	disPrint(2,0, str);
	return 1;
} */

/*
 * Рисует картирку
 */
int dimageShtrih(uint8_t ev)
{
	static int count = 0;
	switch (ev) {
	case DIS_PUSH_L:
		pdisp = dmainM;
		break;
	case DIS_PUSH_OK:
		count++;
		if ( count > 1 ) {
			count = 0;
		}
		break;
	}
	disClear();
	if ( count == 0 ) {
		disShowImg( (const uint8_t*)imgShtrih );
	} else {
		disShowImg( (const uint8_t*)imgCar );
	}
	return 1;
}

