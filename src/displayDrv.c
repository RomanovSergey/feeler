/*
 * displayDrv.c
 *
 *  Created on: Oct 2016
 *      Author: se
 */

#include "stm32f0xx.h"
#include "fonts/FastFont.h"
#include <string.h>

#define RESET_LOW    GPIO_ResetBits(GPIOB,GPIO_Pin_10)  //reset display on
#define RESET_HI     GPIO_SetBits(GPIOB,GPIO_Pin_10)    //reset display off
#define CMD_MODE     GPIO_ResetBits(GPIOB,GPIO_Pin_11)  //command mode
#define DATA_MODE    GPIO_SetBits(GPIOB,GPIO_Pin_11)    //data mode
#define CE_LOW       GPIO_ResetBits(GPIOB,GPIO_Pin_12)  //chip enable on
#define CE_HI        GPIO_SetBits(GPIOB,GPIO_Pin_12)    //chip enable off

#define DISP_X  84
#define DISP_Y  48
uint8_t coor[DISP_X][DISP_Y / 8];//буфер дисплея 84x48 пикселей (1 бит на пиксель)
uint8_t dstat = 0;

void display_cmd(uint8_t data) {
	CMD_MODE; // Низкий уровень на линии DC: инструкция
	CE_LOW; // Низкий уровень на линии SCE
	SPI_SendData8(SPI2, data);
	while ( (SPI2->SR & SPI_I2S_FLAG_BSY) );
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
	DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)coor;
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStruct.DMA_BufferSize = (uint32_t)(DISP_X * DISP_Y / 8);
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

		DMA_ClearITPendingBit(DMA1_IT_TC5);
	}
}

void display_clear() {
	memset( coor, 0, DISP_X * DISP_Y / 8);
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
	DMA_SetCurrDataCounter( DMA1_Channel5, (uint16_t)(DISP_X * DISP_Y / 8) );
	DMA_Cmd(DMA1_Channel5, ENABLE);
}

void setPixel(int x, int y) {
	int yb = y / 8;
	int yo = y % 8;
	uint8_t val = coor[x][yb];
	val |= (1 << yo);
	coor[x][yb] = val;
}

const uint8_t charA[6] = {
		0x70, 0x1c, 0x13, 0x13, 0x1c, 0x70
};
const uint8_t charB[6] = {
		0x7f, 0x49, 0x49, 0x49, 0x49, 0x36
};
const uint8_t charC[6] = {
		0x1c, 0x22, 0x41, 0x41, 0x41, 0x22
};
/*
 * Печать символа высотой 8 шириной 6 пикселей
 */
void wrChar_6x8(uint8_t x, uint8_t y, const uint8_t *c) {
	if ( (y % 8) == 0 ) {//если соблюдено условие для быстрой печати
		y = y >> 3;
		for ( int dy = 0; dy < 6; dy ++ ) {
			coor[x][y] = c[dy];//вывод в буфер дисплея
			x++;
		}
		return;
	} else {//иначе медленная печать (много битовых операций)
		uint8_t yn = y;
		for ( int dy = 0;  dy < 6;  dy++ ) {
			uint8_t b = c[dy];
			for ( int dx = 0; dx < 8; dx ++ ) {
				if ( b & (1 << dx) ) {
					setPixel( x, yn );
				}
				yn++;
			}
			x++;
			yn = y;
		}
	}
}

/*
 * Печать символа шириной width высотой height
 * x, y - координаты верхнего левого пикселя
 * *с - указатель на шрифт выводимого символа [width] байт
 */
void wrChar_x_8(uint8_t x, uint8_t y, uint8_t width, uint16_t code) {
	const char* img = getImg5x8( code );
	if ( (y%8) == 0 ) {//условие быстрой печати
		y >>= 3;
		for ( int dy = 0;  dy < width;  dy++ ) {
			coor[x][y] = img[dy];
			x++;
		}
	}
}

/*
 * prints печать строки на дисплей
 * Параметры:
 *   numstr - номер строки 0..5
 *   pos - позиция символа 0..13
 *   *s  - указатель на строку
 */
void prints(uint8_t pos, uint8_t numstr, const char* s) {
	uint8_t x;
	uint8_t y;
	uint16_t code;
	if ( pos > 13 ) {
		return;
	} else if ( numstr > 5 ) {
		return;
	}
	x = pos * 6;
	y = numstr * 8;
	while ( *s != 0 ) {
		code = *s;
		if ( code >= 0xD0 ) {
			code = code << 8;
			code |= *(++s);
		}
		wrChar_x_8( x, y, 5, code);
		x += 6;
		s++;
		if ( x >= 84 ) {
			break;
		}
	}
}

void display(void) {
	static int tim = 0;
	tim++;
	if ( tim == 500 ) {
		prints( 0, 0, "Русский текст");
		prints( 0, 1, "кодир-а UTF-8");
		prints( 0, 2, "АБВГД ЕЁ её");
		prints( 0, 3, "Ferrum");
		prints( 0, 4, "Alumin!@#$%^&*");
		prints( 0, 5, "Happy New Year!");
		display_dma_send();
	} else if ( tim == 2500 ) {
		display_clear();
		display_dma_send();
		tim = 0;
	}
	if ( SPI2->SR & SPI_I2S_FLAG_BSY ) {
		return;
	}
	if ( 1 == dstat ) {
		DMA_Cmd(DMA1_Channel5, DISABLE);
		CE_HI;
		dstat = 0;
	}
}


