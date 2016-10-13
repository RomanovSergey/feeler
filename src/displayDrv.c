/*
 * displayDrv.c
 *
 *  Created on: Oct 2016
 *      Author: se
 */

#include "stm32f0xx.h"

void initDisplay(void) {
	SPI_InitTypeDef          SPI_InitStruct;

	SPI_Init(SPI2, &SPI_InitStruct);
}

