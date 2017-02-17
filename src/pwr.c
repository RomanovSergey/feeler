/*
 * pwr.c
 *
 *  Created on: Feb 17, 2017
 *      Author: se
 */

#include "stm32f0xx.h"
#include "main.h"
#include "displayDrv.h"
#include "pwr.h"

//===========================================================================
//===========================================================================
//для кругового буфера событий модуля power
#define PWR_LEN_BITS   3
#define PWR_LEN_BUF    (1<<PWR_LEN_BITS) // 8 или 2^3 или (1<<3)
#define PWR_LEN_MASK   (PWR_LEN_BUF-1)   // bits: 0000 0111
static uint8_t pwrbufEv[PWR_LEN_BUF] = {0};
static uint8_t pwrtail = 0;
static uint8_t pwrhead = 0;
/*
 * возвращает 1 если в кольцевом буфере есть свободное место для элемента, иначе 0
 */
static int pwrHasFree(void) {
	if ( ((pwrtail + 1) & PWR_LEN_MASK) == pwrhead ) {
		return 0;//свободного места нет
	}
	return 1;//есть свободное место
}
/*
 * помещает событие в круговой буфер
 * return 1 - успешно; 0 - нет места в буфере
 */
int pwrPutEv(uint8_t event) {
	if (event == 0) {
		return 1;//событие с нулевым кодом пусть не будет для удобства
	}
	if ( pwrHasFree() ) {
		pwrbufEv[pwrhead] = event;
		pwrhead = (1 + pwrhead) & PWR_LEN_MASK;//инкремент кругового индекса
		return 1;
	} else {
		return 0;//нет места в буфере
	}
}
/*
 *  извлекает событие из кругового буфера
 *  если 0 - нет событий
 */
uint8_t pwrGetEv(void) {
	uint8_t event = 0;
	if (pwrhead != pwrtail) {//если в буфере есть данные
		event = pwrbufEv[pwrtail];
		pwrtail = (1 + pwrtail) & PWR_LEN_MASK;//инкремент кругового индекса
	}
	return event;
}
//===========================================================================
//===========================================================================

void power(void)
{
	static const uint16_t CTIM = 2000;
	static int startTime = 0;

	if ( startTime > CTIM ) {
		static uint32_t alarm = 0;
		uint8_t event;
		event = pwrGetEv();
		if ( event == PWR_POWEROFF ) {
			PWR_OFF;
		} else if ( event == PWR_ALARM_3000 ) {
			alarm = 3000;
		}

		if ( alarm != 0 ) {
			alarm--;
			if ( alarm == 0 ) {
				dispPutEv( DIS_ALARM );
			}
		}
	} else if ( startTime < CTIM ) {
		startTime++;
	} else if ( startTime == CTIM ) {
		startTime++;
		PWR_ON;
		BL1_ON;
		BL2_ON;
		dispPutEv( DIS_PAINT );
	}
}
