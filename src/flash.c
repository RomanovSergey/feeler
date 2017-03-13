/*
 * flash.c
 *
 *  Created on: 12 марта 2017 г.
 *      Author: se
 */

#include "stm32f0xx.h"
#include "flash.h"

//FLASH_Status FLASH_ProgramWord(uint32_t Address, uint32_t Data);

/*
 * Чтение 2х байтного значения с флэш по заданому адресу
 * Return: 1 - succes; 0 - fail.
 */
uint16_t fread16(uint32_t Address)
{
	return *(__IO uint16_t*)Address;
}

