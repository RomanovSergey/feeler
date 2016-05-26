/*
 * micrometr.c
 *
 *  Created on: May 25, 2016
 *      Author: se
 */

#include "stm32f0xx.h"
#include "micro.h"

typedef struct {
	uint32_t  Lval;  //измеренное значение индуктивности
	uint16_t  micro; //соответствующее значение в микрометрах
} ltom_t;// Lvalue to micrometer

#define LTOMSIZE  40

ltom_t ltom[LTOMSIZE] = {  // 40 * ( 4 + 2 ) = 240 байт
		{      0,      0 },  //  0 - always begins from 0
		{   1000,    100 },  //  1
		{   2000,    200 },  //  2
		{   3000,    300 },  //  3   Aluminium
		{   4000,    400 },  //  4
		{   5000,    500 },  //  5
		{   6000,    600 },  //  6
		{   7000,    700 },  //  7
		{  10000, 0xFFFF },  //  8
		{  15000, 0xFFFF },  //  9 - air
		{  20000, 0xFFFF },  // 10
		{  25000,    900 },  // 11   Ferrum
		{  30000,    800 },  // 12
		{  35000,    700 },  // 12
		{  40000,    600 },  // 12
		{  45000,    500 },  // 12
		{  50000,    400 },  // 12
		{  55000,    300 },  // 12
		{  60000,    200 },  // 12
		{  65000,    100 },  // 12
		{  70000,      0 },  // 12
		{  75000,      0 },  // 12
};

/*
 * Перобразует величину Lvalue (пропорциональна индуктивности)
 * в выходное значение в микрометрах
 */
uint16_t micro(uint32_t Lvalue) {
	int i;
	if ( Lvalue == 0 ) {
		return 0xFFFF;
	}
	for ( i = 1; i < LTOMSIZE; i++ ) {
		if ( Lvalue < ltom[i].Lval ) {

		}
	}
	return 0xFFFF;
}
