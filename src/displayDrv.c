/*
 * displayDrv.c
 *
 *  Created on: Oct 2016
 *      Author: se
 */

#include "stm32f0xx.h"

#define ResetDisplayOn   GPIO_ResetBits(GPIOB,GPIO_Pin_10)  //reset display on
#define ResetDisplayOff  GPIO_SetBits(GPIOB,GPIO_Pin_10)    //reset display off
#define DispComm         GPIO_ResetBits(GPIOB,GPIO_Pin_11)  //data mode
#define DispData         GPIO_SetBits(GPIOB,GPIO_Pin_11)    //command mode
#define DispChipEOn      GPIO_ResetBits(GPIOB,GPIO_Pin_12)  //chip enable on
#define DispChipEOff     GPIO_SetBits(GPIOB,GPIO_Pin_12)    //chip enable off

uint8_t disp[84*6] = {0};

void pcd8544_send_byte(uint8_t data) {
	DispChipEOn; // Низкий уровень на линии SCE
	SPI_SendData8(SPI2, data);
	while ( SET == SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) );
	//while(!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));//wait until systick timer (1ms)
	DispChipEOff; // Высокий уровень на линии SCE
}

void display_cmd(uint8_t data) {
	DispComm; // Низкий уровень на линии DC: инструкция
	pcd8544_send_byte(data);
}

void display_data(uint8_t data) {
	DispData; // Высокий уровень на линии DC: данные
	pcd8544_send_byte(data);
}

void initDisplay(void) {
	SPI_InitTypeDef          SPI_InitStruct;
	GPIO_InitTypeDef         GPIO_InitStructure;

	//================================================================
	// SPI2 init for graphic display =================================
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	GPIO_DeInit(GPIOB);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;//display Reset
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_11;//display Data/Command
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_12;//display Chip Enable
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	ResetDisplayOn;//reset display

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;//display spi2_clk
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_15;//display spi2_mosi
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_0);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_0);

	SPI_InitStruct.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
	SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStruct.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;// 3 MHz
	SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_Init(SPI2, &SPI_InitStruct);


	SPI_Cmd(SPI2, ENABLE);

	while(!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));//wait until systick timer (1ms)
	while(!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));//wait until systick timer (1ms)
	ResetDisplayOff;

	display_cmd(0x21);	// расширенный набор команд
	display_cmd(0x80 + 64);	// напряжение смещения 56

	display_cmd(0x04);	// Режим температурной коррекции 0
	display_cmd(0x13);	// схема смещения 1:48
	display_cmd(0x20);
	display_cmd(0x0c);	// Нормальное отображение

	//============================================================
	DMA_InitTypeDef DMA_InitStruct;

	DMA_DeInit(DMA1_Channel5);

	DMA_InitStruct.DMA_PeripheralBaseAddr = SPI2_BASE + 0x0c;
	DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)disp;
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStruct.DMA_BufferSize = (uint32_t)sizeof( disp );
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStruct.DMA_Priority = DMA_Priority_High;
	DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel5, &DMA_InitStruct);

	DMA_Cmd(DMA1_Channel5, ENABLE);
}

// Выбирает страницу и горизонтальную позицию для вывода
void display_setpos(uint8_t page, uint8_t x) {
  display_cmd(0x40 | (page & 7));
  display_cmd(0x80 | x);
}

void display_set(uint8_t data) {
	for (int i = 0; i < sizeof( disp ); i++) {
		disp[i] = data;
	}
	display_setpos(0, 0);

	DispData; // Высокий уровень на линии DC: данные
	DispChipEOn; // Низкий уровень на линии SCE
	SPI_I2S_DMACmd( SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
	//DispChipEOff; // Высокий уровень на линии SCE

//	for (int i = 0; i < sizeof( disp ); i++) {
//		display_data( disp[i] );
//	}

	//display_setpos(0, 0);
}

void display(void) {
	static int tim = 0;
	tim++;
	if ( tim == 1000 ) {
		display_set( 0xc0 );
	}
	if ( tim == 2000 ) {
		display_set( 0 );
		tim = 0;
	}
}


