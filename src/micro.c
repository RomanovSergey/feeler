/*
 * micrometr.c
 *
 *  Created on: May 25, 2016
 *      Author: se
 */

#include "stm32f0xx.h"
#include "micro.h"

typedef struct {
	uint32_t  Lval;  //измеренное значение индуктивности (L)
	uint16_t  micro; //соответствующее значение в микрометрах (D)
} ltom_t;// Lvalue to micrometer

#define LTOMSIZE  40

//калибровочная таблица в озу
ltom_t ltom[LTOMSIZE] = {  //init  Lvalue to micrometer in RAM 40 * ( 4 + 2 ) = 240 байт
		{      0,      0 },  //  0 - always begins from 0
		{   1000,    100 },  //  1
		{   2000,    200 },  //  2
		{   3000,    300 },  //  3
		{   4000,    400 },  //  4      Aluminium
		{   5000,    500 },  //  5
		{   6000,    600 },  //  6
		{   7000,    700 },  //  7
		{  10000, 0xFFFF },  //  8
		{  15000, 0xFFFF },  //  9 - air
		{  20000, 0xFFFF },  // 10
		{  25000,    900 },  // 11
		{  30000,    800 },  // 12
		{  35000,    700 },  // 12
		{  40000,    600 },  // 12
		{  45000,    500 },  // 12      Ferrum
		{  50000,    400 },  // 12
		{  55000,    300 },  // 12
		{  60000,    200 },  // 12
		{  65000,    100 },  // 12
		{  70000,      0 },  // 12
		{  75000,      0 },  // 12
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
uint16_t micro(uint32_t L) {
	uint32_t D, D1, D2, L1, L2;
	uint16_t i;

	if ( L == 0 ) {
		return 0xFFFF;//ну это же ошибка будет
	}
	for ( i = 1; i < LTOMSIZE; i++ ) {//найдем ближайшие точки
		if ( L < ltom[i].Lval ) {
			L2 = ltom[i].Lval;
			D2 = ltom[i].micro;
			L1 = ltom[i-1].Lval;
			D2 = ltom[i-1].micro;
			break;//нашли 2 ближайшие точки на прямой
		}
	}
	D = (L-L1)*(D2-D1)/(L2-L1) + D1;
	return D;
}
