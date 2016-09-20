/*
 * micrometr.c
 *
 *  Created on: May, 2016
 *      Author: se
 */

#include "stm32f0xx.h"
#include "micro.h"

typedef struct {
	int32_t  F;  //измеренное значение индуктивности
	int16_t  micro; //соответствующее значение в микрометрах
} ftom_t;// F to micrometer

/*
 * FTOMSIZE - количество точек для замеров и калибровки
 */
#define FTOMSIZE  7

//калибровочная таблица в озу
ftom_t ftom[FTOMSIZE] = { {0,0} };  //init F to micrometer in RAM


/*
 * Перобразует величину F (пропорциональна индуктивности)
 * в выходное значение в микрометрах M
 *
 * Используется уравнение прямой по двум точкам:
 * (M-M1)/(M2-M1) = (F-F1)/(F2-F1);
 * тогда искомая величина M равна:
 * M = (F-F1)*(M2-M1)/(F2-F1) + M1;
 */
int16_t micro(int32_t F) {
	int32_t M, M1, M2, F1, F2;
	uint16_t i;

	if ( F <= ftom[0].F ) {
		return 0;
	}
	for ( i = 1; i < FTOMSIZE; i++ ) {//найдем ближайшие точки
		if ( (F > ftom[i-1].F)&&(F <= ftom[i].F) ) {
			F2 = ftom[i].F;
			M2 = ftom[i].micro;
			F1 = ftom[i-1].F;
			M1 = ftom[i-1].micro;
			M = (F-F1)*(M2-M1)/(F2-F1) + M1;
			return M;
		}
	}
	return 0xFFFF;//Air
}

/*
 * Функция используется во время калибровки
 * параметры
 *   F  - измеренная величина
 *   micro - известный зазор между датчиком и металлом
 *
 * return:
 *   1 - успех
 *   0 - ошибка
 */
int addCalibPoint(uint32_t F, uint16_t micro) {
	switch (micro) {
	case 0://показание на железе
		ftom[0].F  = F;
		ftom[0].micro = micro;
		return 1;
	case 100:
		ftom[1].F  = F;
		ftom[1].micro = micro;
		return (F > ftom[0].F)? 1 : 0;
	case 200:
		ftom[2].F  = F;
		ftom[2].micro = micro;
		return (F > ftom[1].F)? 1 : 0;
	case 300:
		ftom[3].F  = F;
		ftom[3].micro = micro;
		return (F > ftom[2].F)? 1 : 0;
	case 400:
		ftom[4].F  = F;
		ftom[4].micro = micro;
		return (F > ftom[3].F)? 1 : 0;
	case 600:
		ftom[5].F  = F;
		ftom[5].micro = micro;
		return (F > ftom[4].F)? 1 : 0;
	case 5000://показание на максимуме (дальше отображаем воздух)
		ftom[6].F  = F;
		ftom[6].micro = micro;
		return 1;
	}
	return 0;//ошибка
}



