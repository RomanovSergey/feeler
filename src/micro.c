/*
 * micrometr.c
 *
 *  Created on: May, 2016
 *      Author: se
 */

#include "stm32f0xx.h"
#include "micro.h"
#include "main.h"

typedef struct {
	uint32_t  F;  //измеренное значение индуктивности
	uint16_t  micro; //соответствующее значение в микрометрах
} ftoM_t;// F to micrometer


//FTOMSIZE - количество точек для замеров и калибровки
#define FTOMSIZE  20

//калибровочная таблица в озу
ftoM_t table[FTOMSIZE];

/*
 * Начальная инициализация калибровочной таблицы в озу
 */
void initCalib(void) {
	for (int i = 0; i < FTOMSIZE; i++) {
		table[i].F = 0;
		table[i].micro = 0xFFFF;//означает воздух (infinity)
	}
}

/*
 * Перобразует частоту F (обр. пропорциональна индуктивности)
 * в выходное значение в микрометрах M
 *
 * Используется уравнение прямой по двум точкам:
 * (M-M1)/(M2-M1) = (F-F1)/(F2-F1);
 * тогда искомая величина M равна:
 * M = (F-F1)*(M2-M1)/(F2-F1) + M1;
 */
int16_t micro(int32_t F) {
	int32_t M, M1, M2, F1, F2;
	uint16_t i=1;

	if ( F <= table[0].F ) {
		return 0;//величина без зазора - 0 мкм
	}
	while (table[i].micro != 0xFFFF) {
		if ( (F > table[i-1].F)&&(F <= table[i].F) ) {
			F2 = table[i].F;
			M2 = table[i].micro;
			F1 = table[i-1].F;
			M1 = table[i-1].micro;
			M = (F-F1)*(M2-M1)/(F2-F1) + M1;
			return M;
		}
		i++;
	}
	return 0xFFFF;
}

/*
 * int addFerrum(uint32_t Freq, uint16_t micro)
 * Функция используется во время калибровки железа
 * параметры
 *   Freq  - измеренная величина
 *   micro - известный зазор между датчиком и металлом
 * return:
 *   1 - успех
 *   0 - ошибка
 */
int addFerrum(uint32_t Freq, uint16_t micro) {
	int i = 0;

	while ( table[i].F != 0 ) {//ищем первую свободную ячейку для записи в таблицу
		i++;
		if ( i == (FTOMSIZE - 1) ) {//если дошли до предела
			return 0;//error add point
		}
	}
	table[i].F  = Freq;
	table[i].micro = micro;

	return 1;//good
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
		table[0].F  = F;
		table[0].micro = micro;
		return 1;
	case 100:
		table[1].F  = F;
		table[1].micro = micro;
		return (F > table[0].F)? 1 : 0;
	case 200:
		table[2].F  = F;
		table[2].micro = micro;
		return (F > table[1].F)? 1 : 0;
	case 300:
		table[3].F  = F;
		table[3].micro = micro;
		return (F > table[2].F)? 1 : 0;
	case 400:
		table[4].F  = F;
		table[4].micro = micro;
		return (F > table[3].F)? 1 : 0;
	case 600:
		table[5].F  = F;
		table[5].micro = micro;
		return (F > table[4].F)? 1 : 0;
	case 5000://показание на максимуме (дальше отображаем воздух)
		table[6].F  = F;
		table[6].micro = micro;
		table[7].micro = 0xFFFF;//air
		return 1;
	}
	return 0;//ошибка
}



