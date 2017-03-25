/*
 * flash.c
 *
 *  Created on: 12 марта 2017 г.
 *      Author: se
 *
 *  Модуль для работы с флэш памятью
 *  Реализует алгоритм записи, чтения данных, а также стирает страницу
 *  при ее заполнении.
 *  Запись и чтение осуществляется пакетами, заданной длины.
 *
 */

#include "stm32f0xx.h"
#include "stm32f0xx_flash.h"
#include "flash.h"

#define PAGE60  0x0800F000
#define PAGE61  0x0800F400
#define PAGE62  0x0800F800
#define PAGE63  0x0800FC00

uint16_t fcalcXOR( const uint16_t *buf )
{
	return 0;
}

///*
// * fFindEmptyAddr() - для отладки
// * находит адрес пустой 16и битной ячейки (0xFFFF)
// * начиная с начала 62й страницы
// *   Return: адрес пустой ячейки
// *   или 0 - адрес не найден
// */
//uint32_t fFindEmptyAddr(void)
//{
//	uint32_t addr = PAGE60;
//	for ( int i = 0; i < 5*3; i++ ) {
//		if ( fread16(addr) == 0xFFFF ) {
//			return addr;
//		}
//		addr += 2;
//	}
//	return 0;
//}
//
///*
// * fwriteInc() - функция для отладки
// * находит пустой элемент, и записывает
// * на его место hw
// *   Return FLASH_Status
// */
//FLASH_Status fwriteInc( uint16_t hw )
//{
//	uint32_t addr = fFindEmptyAddr();
//	if ( addr == 0 ) {
//		return 0;
//	}
//	FLASH_Unlock();
//	FLASH_Status fstat = FLASH_ProgramHalfWord( addr, hw );
//	FLASH_Lock();
//
//	return fstat;
//}
//
///*
// * fFindFilledAddr() - для отладки
// * находит адрес записанной 16и битной ячейки
// * (не равной 0xFFFF или 0х0000)
// * начиная с начала 62й страницы
// *   Return: адрес записанной ячейки
// *   или 0 - адрес не найден
// */
//uint32_t fFindFilledAddr(void)
//{
//	uint32_t addr = PAGE60;
//	uint16_t data;
//	for ( int i = 0; i < 5*3; i++ ) {
//		data = fread16( addr );
//		if ( data == 0xFFFF ) {
//			return 0;
//		}
//		if ( data != 0 ) {
//			return addr;
//		}
//		addr += 2;
//	}
//	return 0;
//}
//
///*
// * fzeroInc() - функция для отладки
// * находит записаный элемент, и записывает
// * на его место 0
// *   Return FLASH_Status
// */
//FLASH_Status fzeroInc( void )
//{
//	uint32_t addr = fFindFilledAddr();
//	if ( addr == 0 ) {
//		return 0;
//	}
//	FLASH_Unlock();
//	FLASH_Status fstat = FLASH_ProgramHalfWord( addr, 0 );
//	FLASH_Lock();
//	return fstat;
//}

//=====================================================
/*
 * Алгоритм записи и чтения информации во флэш.
 *
 *   Минимальная операция записи - 2 байта (16 бит), записать можно только
 * в стертую ячеку (0xFFFF - проверяется флэш контроллером),
 * исключение - если записываемые данные имеют все биты нули (0х0000).
 *   Адрес записи должен быть выровнен по четным адресам.
 * Стирать можно только страницу целиком (1 кБ на данном контроллере), посте стирания
 * все данные страницы переведутся в 0xFF.
 *
 *   В качестве формата записи данных во флэше примем пока следующий вид:
 * START, ID, LEN, DATA[], XOR
 * ,где
 *     START - признак старта данных, 16 бит: 0xABCD (чтобы распознать ошибку)
 *     ID - идентификатор записи, 16 бит, если 0 - то запись стерта
 *     LEN - длина всей записи, начиная от START и заканчивая XOR, в байтах
 *     DATA[] - сами данные, по 16 бит, имеют длину = LEN - 8 байт
 *     XOR - контрольная сумма DATA[] данных
 *
 * - Запись данных во флэше должна быть уникальной, то есть записи с
 * одинаковыми ID повторятся не должны.
 * - Если запись с определенным ID нужно удалить не стирая всю страницу, то вместо
 * поля ID записываются нули, остальные поля записи не трогаются.
 * - Поиск записи по ID происходит по алгоримту: первый байт всегда должен быть
 * стартовым, далее смотрим ID, если ID не наш, увеличиваем адрес на LEN байт и
 * итерация повторяется пока не найдем нужный ID или пока не наткнемся на пустую
 * ячейку или пока не закончится страница памяти.
 * - Если нужно записать новые данные с определенным ID, то сначала удаляем старую
 * запись с данным ID (если она есть), находим первую чистую ячейку (0xFFFF) и если
 * хватает места для записи - записываем.
 */

static const uint32_t PAGE_SIZE = 1024;
static const uint16_t START = 0xABCD;
static const uint16_t MIN_LEN = 10;
static const uint16_t MAX_LEN = 160;

/*
 * Чтение 2х байтного значения с флэш по заданому адресу
 */
uint16_t fread16(uint32_t Address)
{
	return *(__IO uint16_t*)Address;
}

/*
 * Производит поиск записи во флэш с заданным ID
 * Params:
 *   ID - номер ID которую хотим найти
 *   *padr - указатель, куда сохранится адрес записи
 * Return:
 *   0 - ID найден, *addr указывает на старт записи
 *   1 - не нашли ID, дошли до пустой ячейки, paddr на пустую ячейку
 *   2 - нет старта данных, ошибка
 *   3 - длина записи меньше минимально допустимой
 *   4 - длина записи больше максимально допустимой
 *   5 - выход за пределы страницы
 */
int fFindIDaddr( uint16_t ID, uint32_t *padr )
{
	uint16_t data;
	uint16_t len;
	uint16_t id;
	*padr = PAGE60;
	do {
		data = fread16( *padr );
		if ( data == START ) {
			id = fread16( *padr + 2 ); // смотрим ID
			if ( ID == id ) {
				return 0; // нашли!
			} else {
				// смотрим LEN
				len = fread16( *padr + 4 );
				if ( len < MIN_LEN ) {
					return 3; // длина записи меньше минимально допустимой
				} else if ( len > MAX_LEN ) {
					return 4; // длина записи больше максимально допустимой
				} else {
					*padr += len; // перейдем на следующую запись
					if ( (*padr - PAGE60) > PAGE_SIZE ) {
						return 5; // выход за пределы страницы
					}
				}
			}
		} else if ( data == 0xFFFF ) {
			return 1; // дошли до пустой ячейки
		} else {
			return 2; // нет старта данных
		}
	} while ( 1 );
	return -1; // не достижимая инструкция
}

/*
 * Удаляет запись под данным адресом
 *   Return:
 *    0 - Ok
 *    1 - нет старта записи
 *    2 - ID записи не соответствует адресу
 *    3 - ошибка удаления записи
 */
int fdeleteID( uint16_t ID, uint32_t adr ) {
	uint16_t data;
	data = fread16( adr );
	if ( data != START ) {
		return 1;
	}
	adr += 2;
	data = fread16( adr );
	if ( data != ID ) {
		return 2;
	}
	FLASH_Unlock();
	FLASH_Status fstat = FLASH_ProgramHalfWord( adr, 0 );
	FLASH_Lock();
	if ( fstat != FLASH_COMPLETE ) {
		return 3;
	}
	return 0;
}

/*
 * Стирает флэш страницу
 *   Return:
 *     0 - Ok
 *     1 - не правильный адрес
 *     2 - ошибка стирания страницы
 */
int ferasePage( uint32_t adr )
{
	if ( adr != PAGE60 ) {
		return 1;
	}
	FLASH_Unlock();
	FLASH_Status fstat =  FLASH_ErasePage( PAGE60 );
	FLASH_Lock();
	if ( fstat != FLASH_COMPLETE ) {
		return 2;
	}
	return 0;
}

/*
 * Сохраняет запись с уникальным ID на флэш
 * , где
 *  *buf - указатель на 16 битные данные длиной len байт.
 *   len - должно быть четным числом (т.к. данные 16 битные)
 * Return:
 *   0 - Ok
 *   1 - ошибка удаления старой записи
 *   2 - не дошли до пустой ячейки или прочая ошибка
 *   3 - не достаточно свободного места для новой записи
 *   4 - ошибка во время записи
 *   5 - len не является четным числом
 */
int fsaveImage( const uint16_t ID, const uint16_t* const buf, const uint16_t len )
{
	int res;
	FLASH_Status fstat;
	uint32_t adr = PAGE60;

	if ( len%2 != 0) {
		return 5; // len is not even
	}

	res = fFindIDaddr( ID, &adr );
	if ( res == 0 ) { // если запись уже существует
		int r;
		r = fdeleteID( ID, adr ); // удалим старую запись
		if ( r != 0 ) {
			return 1;
		}
		res = fFindIDaddr( ID, &adr ); // снова ищем
	}
	if ( res != 1 ) {
		return 2;
	}
	// теперь adr указывает на пустую ячейку
	// определим объем оставшейся памяти
	uint32_t free = PAGE61 - adr;
	uint16_t LEN = len + 8; // длина всей записи
	if ( free < LEN ) {
		return 3;
	}

	FLASH_Unlock();

	// запишем старт признак записи
	fstat = FLASH_ProgramHalfWord( adr, START );
	if ( fstat != FLASH_COMPLETE ) {
		FLASH_Lock();
		return 4;
	}

	// запишем ID записи
	fstat = FLASH_ProgramHalfWord( adr + 2, ID );
	if ( fstat != FLASH_COMPLETE ) {
		FLASH_Lock();
		return 4;
	}

	// запишем LEN - длину всей записи
	fstat = FLASH_ProgramHalfWord( adr + 4, LEN );
	if ( fstat != FLASH_COMPLETE ) {
		FLASH_ProgramHalfWord( adr + 2, 0 ); // стерём ID
		FLASH_Lock();
		return 4;
	}

	// запишем данные
	const uint16_t *data = buf; // чтобы не инкрементировать buf
	for ( uint16_t i = 0 ; i < len; i += 2 ) {
		fstat = FLASH_ProgramHalfWord( adr + 6 + i, *data );
		if ( fstat != FLASH_COMPLETE ) {
			FLASH_ProgramHalfWord( adr + 2, 0 ); // стерём ID
			FLASH_Lock();
			return 4;
		}
		data++;
	}

	// запишем XOR
	uint16_t xor = fcalcXOR( buf );
	fstat = FLASH_ProgramHalfWord( adr + 6 + len, xor );
	if ( fstat != FLASH_COMPLETE ) {
		FLASH_ProgramHalfWord( adr + 2, 0 ); // стерём ID
		FLASH_Lock();
		return 4;
	}

	FLASH_Lock();
	return 0;
}

/*
 * Загружает данные с указанным ID, где
 *  *buf    - указатель на 16 битные данные
 *   maxLen - максимальный объем буфера buf в байтах
 *   rlen    - считанное количество байт (всегда четное)
 * Return:
 *   0 - Ok, данные в buf записались
 *
 *   1 - не нашли ID, дошли до пустой ячейки, paddr на пустую ячейку
 *   2 - нет старта данных, ошибка
 *   3 - длина записи меньше минимально допустимой
 *   4 - длина записи больше максимально допустимой
 *   5 - выход за пределы страницы
 *
 *   11 - объем буфер buf не достаточен для записи
 *   12 - данные в buf записали, но xor не совпадает
 */
int floadImage( const uint16_t ID, uint16_t* buf, const uint16_t maxLen, uint16_t *rlen )
{
	int res;
	uint32_t adr = PAGE60;
	uint16_t dataLen;

	res = fFindIDaddr( ID, &adr );
	if ( res == 0 ) {
		// нашли запись
		dataLen = fread16( adr + 4 );
		dataLen -= 8;
		if ( dataLen > maxLen ) {
			return 11; // объем буфер buf не достаточен для записи
		}
		adr += 6; // теперь адрес указывает на данные

		// прочитаем данные и запишем их в буфер
		for ( uint16_t i = 0 ; i < dataLen; i += 2 ) {
			*buf = fread16( adr );
			adr += 2;
			buf++;
		}
		*rlen = dataLen;
		uint16_t xor = fcalcXOR( buf );
		if ( xor != fread16( adr ) ) {
			return 12;
		}
		return 0;
	}
	return res;
}

