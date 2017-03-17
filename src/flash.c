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

//FLASH_Status FLASH_ProgramWord(uint32_t Address, uint32_t Data);

#define PAGE62  0x0800F800

/*
 * Чтение 2х байтного значения с флэш по заданому адресу
 * Return: 1 - succes; 0 - fail.
 */
uint16_t fread16(uint32_t Address)
{
	return *(__IO uint16_t*)Address;
}

/*
 * fFindEmptyAddr() - для отладки
 * находит адрес пустой 16и битной ячейки (0xFFFF)
 * начиная с начала 62й страницы
 *   Return: адрес пустой ячейки
 *   или 0 - адрес не найден
 */
uint32_t fFindEmptyAddr(void)
{
	uint32_t addr = PAGE62;
	for ( int i = 0; i < 5*3; i++ ) {
		if ( fread16(addr) == 0xFFFF ) {
			return addr;
		}
		addr += 2;
	}
	return 0;
}

/*
 * fwriteInc() - функция для отладки
 * находит пустой элемент, и записывает
 * на его место hw
 *   Return FLASH_Status
 */
FLASH_Status fwriteInc( uint16_t hw )
{
	uint32_t addr = fFindEmptyAddr();
	if ( addr == 0 ) {
		return 0;
	}
	FLASH_Unlock();
	FLASH_Status fstat = FLASH_ProgramHalfWord( addr, hw );
	FLASH_Lock();

	return fstat;
}

/*
 * fFindFilledAddr() - для отладки
 * находит адрес записанной 16и битной ячейки
 * (не равной 0xFFFF или 0х0000)
 * начиная с начала 62й страницы
 *   Return: адрес записанной ячейки
 *   или 0 - адрес не найден
 */
uint32_t fFindFilledAddr(void)
{
	uint32_t addr = PAGE62;
	uint16_t data;
	for ( int i = 0; i < 5*3; i++ ) {
		data = fread16( addr );
		if ( data == 0xFFFF ) {
			return 0;
		}
		if ( data != 0 ) {
			return addr;
		}
		addr += 2;
	}
	return 0;
}

/*
 * fzeroInc() - функция для отладки
 * находит записаный элемент, и записывает
 * на его место 0
 *   Return FLASH_Status
 */
FLASH_Status fzeroInc( void )
{
	uint32_t addr = fFindFilledAddr();
	if ( addr == 0 ) {
		return 0;
	}
	FLASH_Unlock();
	FLASH_Status fstat = FLASH_ProgramHalfWord( addr, 0 );
	FLASH_Lock();
	return fstat;
}

FLASH_Status ferasePage( void )
{
	FLASH_Unlock();
	FLASH_Status fstat =  FLASH_ErasePage( PAGE62 );
	FLASH_Lock();
	return fstat;
}

//=====================================================
/*
 * Алгоритм записи и чтения информации во флэш.
 *
 * Минимальная операция записи - 2 байта (16 бит), записать можно только
 * в стертую ячеку (0xFFFF - проверяется флэш контроллером),
 * исключение - если записываемые данные имеют все биты нули (0х0000).
 * Адрес записи должен быть выровнен по четным адресам.
 * Стирать можно только страницу целиком (1 кБ на данном контроллере), посте стирания
 * все данные страницы переведутся в 0xFF.
 *
 * В качестве формата записи данных во флэше примем пока следующий вид:
 * START, ID, LEN, DATA[], XOR
 * ,где
 *     START - признак старта данных, 16 бит: 0xABCD (чтобы распознать ошибку)
 *     ID - идентификатор записи, 16 бит, если 0 - то запись стерта
 *     LEN - длина всей записи, начиная от START и заканчивая XOR, в байтах
 *     DATA[] - сами данные, по 16 бит, имеют длину = LEN - 8
 *     XOR - контрольная сумма
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

typedef struct {
	uint16_t   id;  // айди записи
	uint16_t   len; // длина записи = id + len + ptr + xor (вся запись)
	uint16_t*  ptr;
	uint16_t   xor;
} ImageInfo_t;

static const uint16_t START = 0xABCD;

/*
 * Находит запись во флэш с заданным ID
 * Если вернул 0, то ID не найден
 */
uint32_t fFindIDaddr( uint16_t ID )
{
	uint32_t addr = PAGE62;
	uint16_t data;
	do {
		data = fread16( addr );
		if ( data == START ) {


			return addr;
		}
		uint16_t len = fread16( addr + 4 );
		addr += len;
	} while ( 1 );
	return 0;
}

/*
 * Сохраняет образ на флэш
 */
void fsaveImage( uint8_t ID, uint8_t* buf, uint8_t len )
{

}
