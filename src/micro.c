/*
 * micrometr.c
 *
 *  Created on: May 25, 2016
 *      Author: se
 */

#include "stm32f0xx.h"
#include "micro.h"

typedef struct {
	int32_t  Lval;  //измеренное значение индуктивности (L)
	int16_t  micro; //соответствующее значение в микрометрах (D)
} ltom_t;// Lvalue to micrometer

#define LTOMSIZE  7

//калибровочная таблица в озу
ltom_t ltom[LTOMSIZE] = {  //init  Lvalue to micrometer in RAM 40 * ( 4 + 2 ) = 240 байт

		{   4514,  5000 },  // - air

		{   5097,    400 },  //      Ferrum
		{   5181,    320 },  //
		{   5269,    240 },  //
		{   5393,    160 },  //
		{   5466,     80 },  //
		{   5500,      0 },  //
};

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
	return 10000;
}
