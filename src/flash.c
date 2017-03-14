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

void fFindEmptyAddr(void)
{

}
