/*
 * micrometr.c
 *
 *  Created on: May, 2016
 *      Author: se
 */

#include "stm32f0xx.h"
#include "micro.h"

typedef struct {
	int32_t  Lval;  //измеренное значение индуктивности (L)
	int16_t  micro; //соответствующее значение в микрометрах (D)
} ltom_t;// Lvalue to micrometer

/*
 * LTOMSIZE - количество точек для замеров и калибровки
 */
#define LTOMSIZE  7

//калибровочная таблица в озу
ltom_t ltom[LTOMSIZE] = { {0,0} };  //init  Lvalue to micrometer in RAM


/*
 * Перобразует величину L (пропорциональна индуктивности)
 * в выходное значение в микрометрах D
 *
 * Используется уравнение прямой по двум точкам:
 * (D-D1)/(D2-D1) = (L-L1)/(L2-L1);
 * тогда искомая величина D равна:
 * D = (L-L1)*(D2-D1)/(L2-L1) + D1;
 */
int16_t micro(int32_t L) {
	int32_t D, D1, D2, L1, L2;
	uint16_t i;

	if ( L == 0 ) {
		return 0xFFFF;//ну это же ошибка будет
	}
	for ( i = 1; i < LTOMSIZE; i++ ) {//найдем ближайшие точки
		if ( (L > ltom[i-1].Lval)&&(L <= ltom[i].Lval) ) {
			L2 = ltom[i].Lval;
			D2 = ltom[i].micro;
			L1 = ltom[i-1].Lval;
			D1 = ltom[i-1].micro;
			D = (L-L1)*(D2-D1)/(L2-L1) + D1;
			return D;//нашли 2 ближайшие точки на прямой
		}
	}
	return 0xFFFF;
}

/*
 * Функция используется во время калибровки
 * параметры
 *   lval  - измеренная величина
 *   micro - известный зазор между датчиком и металлом
 *
 * return:
 *   1 - успех
 *   0 - ошибка
 */
int addCalibPoint(uint32_t lval, uint16_t micro) {
	switch (micro) {
	case 0://показание на железе
		ltom[0].Lval  = lval;
		ltom[0].micro = micro;
		return 1;
	case 100:
		ltom[1].Lval  = lval;
		ltom[1].micro = micro;
		return (lval > ltom[0].Lval)? 1 : 0;
	case 200:
		ltom[2].Lval  = lval;
		ltom[2].micro = micro;
		return (lval > ltom[1].Lval)? 1 : 0;
	case 300:
		ltom[3].Lval  = lval;
		ltom[3].micro = micro;
		return (lval > ltom[2].Lval)? 1 : 0;
	case 400:
		ltom[4].Lval  = lval;
		ltom[4].micro = micro;
		return (lval > ltom[3].Lval)? 1 : 0;
	case 600:
		ltom[5].Lval  = lval;
		ltom[5].micro = micro;
		return (lval > ltom[4].Lval)? 1 : 0;
	case 0xFFFF://показание на воздухе
		ltom[6].Lval  = lval;
		ltom[6].micro = micro;
		return 1;
	}
	return 0;//ошибка
}



