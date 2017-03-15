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

