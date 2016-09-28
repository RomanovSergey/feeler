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
ftoM_t tableFe[FTOMSIZE];//железо
ftoM_t tableAl[FTOMSIZE];//Aluminumum

/*
 * Начальная инициализация калибровочной таблицы в озу
 */
void initCalib(void) {
	for (int i = 0; i < FTOMSIZE; i++) {
		tableFe[i].F = 0;
		tableFe[i].micro = 0xFFFF;//означает воздух (infinity)
	}
	for (int i = 0; i < FTOMSIZE; i++) {
		tableAl[i].F = 0;
		tableAl[i].micro = 0xFFFF;//означает воздух (infinity)
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

	if ( F <= tableFe[0].F ) {
		return 0;//величина без зазора - 0 мкм
	}
	while ( tableFe[i].micro != 0xFFFF ) {
		if ( (F > tableFe[i-1].F)&&(F <= tableFe[i].F) ) {
			F2 = tableFe[i].F;
			M2 = tableFe[i].micro;
			F1 = tableFe[i-1].F;
			M1 = tableFe[i-1].micro;
			M = (F-F1)*(M2-M1)/(F2-F1) + M1;
			return M;
		}
		i++;
	}
	return 0xFFFF;//air
	if ( tableAl[0].micro == 0xFFFF ) {
		return 0xFFFF;//алюминиевая таблица не содержит калибровочных данных
	}
	if ( F <= tableAl[0].F ) {
		return 0xFFFF;//air
	}
	//далее воздуха быть не должно
	i = 1;
	while ( tableAl[i].micro != 0xFFFF ) {
		if ( (F > tableAl[i-1].F)&&(F <= tableAl[i].F) ) {
			F2 = tableAl[i].F;
			M2 = tableAl[i].micro;
			F1 = tableAl[i-1].F;
			M1 = tableAl[i-1].micro;
			M = (F-F1)*(M2-M1)/(F2-F1) + M1;
			return M;
		}
		i++;
	}
	return 0;
}

/*
 * int addFe(uint32_t Freq, uint16_t micro)
 * Функция используется во время калибровки железа
 * параметры
 *   Freq  - измеренная величина
 *   micro - известный зазор между датчиком и металлом
 * return:
 *   1 - успех
 *   0 - ошибка
 */
int addFe(uint32_t Freq, uint16_t micro) {
	int i = 0;

	while ( tableFe[i].F != 0 ) {//ищем первую свободную ячейку для записи в таблицу
		i++;
		if ( i == (FTOMSIZE - 1) ) {//если дошли до предела
			return 0;//error add point (последний элемент дожен быть 0 и 0xFFFF)
		}
	}
	tableFe[i].F  = Freq;
	tableFe[i].micro = micro;

	return 1;//good
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
int addAl(uint32_t Freq, uint16_t micro) {
	int i = 0;

	while ( tableAl[i].F != 0 ) {//ищем первую свободную ячейку для записи в таблицу
		i++;
		if ( i == (FTOMSIZE - 1) ) {//если дошли до предела
			return 0;//error add point (последний элемент дожен быть 0 и 0xFFFF)
		}
	}
	tableAl[i].F  = Freq;
	tableAl[i].micro = micro;

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
/*int addCalibPoint(uint32_t F, uint16_t micro) {

	switch (micro) {
	case 0://показание на железе
		tableFe[0].F  = F;
		tableFe[0].micro = micro;
		return 1;
	case 100:
		tableFe[1].F  = F;
		tableFe[1].micro = micro;
		return (F > tableFe[0].F)? 1 : 0;
	case 200:
		tableFe[2].F  = F;
		tableFe[2].micro = micro;
		return (F > tableFe[1].F)? 1 : 0;
	case 300:
		tableFe[3].F  = F;
		tableFe[3].micro = micro;
		return (F > tableFe[2].F)? 1 : 0;
	case 400:
		tableFe[4].F  = F;
		tableFe[4].micro = micro;
		return (F > tableFe[3].F)? 1 : 0;
	case 600:
		tableFe[5].F  = F;
		tableFe[5].micro = micro;
		return (F > tableFe[4].F)? 1 : 0;
	case 5000://показание на максимуме (дальше отображаем воздух)
		tableFe[6].F  = F;
		tableFe[6].micro = micro;
		tableFe[7].micro = 0xFFFF;//air
		return 1;
	}
	return 0;//ошибка
}*/



