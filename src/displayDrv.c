/*
 * displayDrv.c
 *
 *  Created on: Oct 2016
 *      Author: se
 */

#include "stm32f0xx.h"

#define RESET_LOW    GPIO_ResetBits(GPIOB,GPIO_Pin_10)  //reset display on
#define RESET_HI     GPIO_SetBits(GPIOB,GPIO_Pin_10)    //reset display off
#define CMD_MODE     GPIO_ResetBits(GPIOB,GPIO_Pin_11)  //command mode
#define DATA_MODE    GPIO_SetBits(GPIOB,GPIO_Pin_11)    //data mode
#define CE_LOW       GPIO_ResetBits(GPIOB,GPIO_Pin_12)  //chip enable on
#define CE_HI        GPIO_SetBits(GPIOB,GPIO_Pin_12)    //chip enable off

union {
uint8_t disp[84*6];// = {0};//буфер дисплея
uint8_t coor[84][6];
} un;
uint8_t dstat = 0;

void display_cmd(uint8_t data) {
	CMD_MODE; // Низкий уровень на линии DC: инструкция
	//pcd8544_send_byte(data);
	CE_LOW; // Низкий уровень на линии SCE
	SPI_SendData8(SPI2, data);
	while ( (SPI2->SR & SPI_I2S_FLAG_BSY) );// !(SPI2->SR & SPI_I2S_FLAG_TXE) ||
	CE_HI; // Высокий уровень на линии SCE
}

void disp_cmds(uint8_t *arr, uint8_t len) {
	CMD_MODE;
	CE_LOW;
	for (int i=0; i<len; i++) {
		while( !(SPI2->SR & SPI_I2S_FLAG_TXE) );
		SPI_SendData8(SPI2, arr[i]);
	}
	while ( (SPI2->SR & SPI_I2S_FLAG_BSY) );
	CE_HI;
}

void initDisplay(void) {
	SPI_InitTypeDef          SPI_InitStruct;
	GPIO_InitTypeDef         GPIO_InitStructure;
	NVIC_InitTypeDef         NVIC_InitStruct;

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
	RESET_LOW;//reset display

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

	SPI_I2S_ITConfig(SPI2, SPI_I2S_IT_TXE, DISABLE);
	SPI_Cmd(SPI2, ENABLE);

	while(!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));//wait until systick timer (1ms)
	while(!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));//wait until systick timer (1ms)
	RESET_HI;

	NVIC_InitStruct.NVIC_IRQChannel = SPI2_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPriority = 2;
	NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
	NVIC_Init(&NVIC_InitStruct);

	uint8_t cmdInit[] = {
			0x21,//расширенный набор команд
			0x80 + 64,//напряжение смещения 56
			0x04,//режим температурной коррекции 0
			0x13,//схема смещения 1:48
			0x22,
			0x0c,//нормальное отображение
	};
	disp_cmds(cmdInit, sizeof (cmdInit));

	//============================================================
	DMA_InitTypeDef DMA_InitStruct;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	DMA_DeInit(DMA1_Channel5);
	DMA_InitStruct.DMA_PeripheralBaseAddr = SPI2_BASE + 0x0c;
	DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)un.disp;
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStruct.DMA_BufferSize = (uint32_t)sizeof( un.disp );
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStruct.DMA_Priority = DMA_Priority_High;
	DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel5, &DMA_InitStruct);

	DMA_Cmd(DMA1_Channel5, DISABLE);
	SPI_I2S_DMACmd( SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);

	NVIC_InitStruct.NVIC_IRQChannel = DMA1_Channel4_5_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
}

/*
 * interrupt handler
 */
void DMA1_Channel4_5_IRQHandler (void) {
	if ( SET == DMA_GetFlagStatus(DMA1_FLAG_TC5) ) {
		dstat = 1;
		//DMA_Cmd(DMA1_Channel5, DISABLE);
		DMA_ClearITPendingBit(DMA1_IT_TC5);
	}
}

/*
 * interrupt handler

void SPI2_IRQHandler(void) {

} */

void display_set() {
	uint8_t data = 0;
	for (int i = 0; i < sizeof( un.disp ); i++) {
		un.disp[i] = data++;
	}
}

void display_clear() {
	for (int i = 0; i < sizeof( un.disp ); i++) {
		un.disp[i] = 0;
	}
}

// Выбирает страницу и горизонтальную позицию для вывода
void display_setpos(uint8_t page, uint8_t x) {
	uint8_t setPos[2];
	setPos[0] = 0x40 | (page & 7);
	setPos[1] = 0x80 | x;
	disp_cmds(setPos, 2);
}

void display_dma_send() {
	display_setpos(0, 0);
	DATA_MODE; // Высокий уровень на линии DC: данные
	CE_LOW; // Низкий уровень на линии SCE
	//dstat = d_dmasend;
	DMA_SetCurrDataCounter(DMA1_Channel5, (uint16_t)sizeof( un.disp ));
	DMA_Cmd(DMA1_Channel5, ENABLE);
}

void point(int x, int y, int color) {
	int yb = y / 8;
	int yo = y % 8;
	uint8_t val = un.coor[x][yb];
	if ( color == 0 ) {
		val &= ~(1 << yo);
	} else {
		val |= (1 << yo);
	}
	un.coor[x][yb] = val;
}

void display(void) {
	static int tim = 0;

	tim++;
	if ( tim == 1000 ) {
		//display_set();
		for (int i=0; i<84; i++) {
			point(i, i>>1, 1);
			point(83-i, 47-(i>>1), 1);
			point(i, 47, 1);
			point(i, 0, 1);
		}
		for (int i = 0; i<48; i++) {
			point(0, i, 1);
			point(83, i, 1);
		}
		display_dma_send();
	}
	if ( tim == 5000 ) {
		display_clear();
		display_dma_send();
		tim = 0;
	}

	if ( /*!(SPI2->SR & SPI_I2S_FLAG_TXE) ||*/ (SPI2->SR & SPI_I2S_FLAG_BSY) ) {
		return;
	}
	if ( 1 == dstat ) {
		DMA_Cmd(DMA1_Channel5, DISABLE);
		CE_HI;
		dstat = 0;
	}
}


