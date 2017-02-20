/*
 * displayDrv.c
 *
 *  Created on: Oct 2016
 *      Author: se
 */

#include "stm32f0xx.h"
#include "fonts/fontA.h"
#include "displayDrv.h"
#include "menu.h"
#include "main.h"
#include <string.h>

#define RESET_LOW    GPIO_ResetBits(GPIOB,GPIO_Pin_1)   //reset display on
#define RESET_HI     GPIO_SetBits(GPIOB,GPIO_Pin_1)     //reset display off
#define CMD_MODE     GPIO_ResetBits(GPIOA,GPIO_Pin_15)  //command mode
#define DATA_MODE    GPIO_SetBits(GPIOA,GPIO_Pin_15)    //data mode
#define CE_LOW       GPIO_ResetBits(GPIOB,GPIO_Pin_6)   //chip enable on
#define CE_HI        GPIO_SetBits(GPIOB,GPIO_Pin_6)     //chip enable off

#define DISP_X  84
#define DISP_Y  48
static uint8_t coor[DISP_X][DISP_Y / 8];//буфер дисплея 84x48 пикселей (1 бит на пиксель)
static uint8_t dstat = 0;//dma status
static uint8_t Xcoor = 0;//текущая координата Х для разных функций печати на дисплей
static uint8_t Ycoor = 0;//текущая координата Y для разных функций печати на дисплей

void display_cmd(uint8_t data) {
	CMD_MODE; // Низкий уровень на линии DC: инструкция
	CE_LOW; // Низкий уровень на линии SCE
	SPI_SendData8(SPI1, data);
	while ( (SPI1->SR & SPI_I2S_FLAG_BSY) );
	CE_HI; // Высокий уровень на линии SCE
}

void disp_cmds(uint8_t *arr, uint8_t len) {
	CMD_MODE;
	CE_LOW;
	for (int i=0; i<len; i++) {
		while( !(SPI1->SR & SPI_I2S_FLAG_TXE) );
		SPI_SendData8(SPI1, arr[i]);
	}
	while ( (SPI1->SR & SPI_I2S_FLAG_BSY) );
	CE_HI;
}

void initDisplay(void) {
	SPI_InitTypeDef     SPI_InitStruct;
	GPIO_InitTypeDef    GPIO_InitStructure;
	NVIC_InitTypeDef    NVIC_InitStruct;

	//================================================================
	// SPI1 init for graphic display =================================
	//RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	//GPIO_DeInit(GPIOB);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;//display Reset
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_6;//display Chip Enable
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;//display Data/Command
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	RESET_LOW;//reset display

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;//display spi2_clk
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_5;//display spi2_mosi
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_0);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_0);

	SPI_InitStruct.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
	SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStruct.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;// 3 MHz
	SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_Init(SPI1, &SPI_InitStruct);

	SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_TXE, DISABLE);
	SPI_Cmd(SPI1, ENABLE);
	while(!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));//wait until systick timer (1ms)
	while(!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));//wait until systick timer (1ms)
	RESET_HI;

	NVIC_InitStruct.NVIC_IRQChannel = SPI1_IRQn;
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
	DMA_DeInit(DMA1_Channel3);
	DMA_InitStruct.DMA_PeripheralBaseAddr = SPI1_BASE + 0x0c;
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
	DMA_Init(DMA1_Channel3, &DMA_InitStruct);

	DMA_Cmd(DMA1_Channel3, DISABLE);
	SPI_I2S_DMACmd( SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
	DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);

	NVIC_InitStruct.NVIC_IRQChannel = DMA1_Channel2_3_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);

	dispPutEv( DIS_REPAINT );
}

/*
 * interrupt handler
 */
void DMA1_Channel2_3_IRQHandler (void) {
	if ( SET == DMA_GetFlagStatus(DMA1_FLAG_TC3) ) {
		dstat = 1;
		DMA_ClearITPendingBit(DMA1_IT_TC3);
	}
}

void disClear(void) {
	memset( coor, 0, DISP_X * DISP_Y / 8);
}

// Выбирает страницу и горизонтальную позицию для вывода
void disSetPos(uint8_t page, uint8_t x) {
	uint8_t setPos[2];
	setPos[0] = 0x40 | (page & 7);
	setPos[1] = 0x80 | x;
	disp_cmds(setPos, 2);
}

void disDMAsend() {
	disSetPos(0, 0);
	DATA_MODE; // Высокий уровень на линии DC: данные
	CE_LOW; // Низкий уровень на линии SCE
	DMA_SetCurrDataCounter( DMA1_Channel3, (uint16_t)(DISP_X * DISP_Y / 8) );
	DMA_Cmd(DMA1_Channel3, ENABLE);
}

void setPixel(int x, int y) {
	int yb = y / 8;
	int yo = y % 8;
	uint8_t val = coor[x][yb];
	val |= (1 << yo);
	coor[x][yb] = val;
}

/*
 * Печать символа шириной width высотой 8 p.
 * x, y - координаты верхнего левого пикселя
 * code - код utf-8 выводимого символа [width] байт
 */
void wrChar_x_8(uint8_t x, uint8_t y, uint8_t width, uint16_t code) {
	const char* img = getFont5x8( code );//getImg5x8( code );
	if ( (y%8) == 0 ) {//условие быстрой печати
		y >>= 3;
		for ( int dy = 0;  dy < width;  dy++ ) {
			coor[x][y] = img[dy];
			x++;
		}
	}
}

uint16_t getUCode( const char* str, uint16_t *code ) {
	uint32_t abc = 0;
	uint16_t ret = 0; // на сколько символов передвинуть указатель строки

	if ( *str != 0) { // if not NULL symbol
		if ( (*str & 0x80) == 0) { // if 1 byte
			*code = *str;
			ret++;
			return ret;
		}
		if ( (*str & 0xE0) == 0xC0 ) { // if 2 bytes 0b110..
			abc = *str;
			str++;
			if ( (*str & 0xC0) != 0x80 ) { // error no 0b10.. bits
				return 0;
			}
			abc <<= 8;
			abc |= *str;

			*code = abc & 0x3F;
			abc >>= 2;
			*code |= abc & 0x07C0;

			ret = 2;
			return ret; // error
		}
//		if ( (code & 0xF0) == 0xE0 ) { // if 3 bytes 0b1110..
//
//		}
	}
	*code = 0;
	return 0;
}

void disPrint(uint8_t numstr, uint8_t X, const char* s)
{
	uint16_t code;

	Xcoor = X;
	if ( Xcoor > 77 ) {
		return;
	} else if ( numstr > 5 ) {
		return;
	}
	Ycoor = numstr * 8;

	while (1) {
		s += getUCode( s, &code );
		if ( code == 0 ) { // если конец строки
			break;
		}
		wrChar_x_8( Xcoor, Ycoor, 5, code);
		Xcoor += 6;
		if ( Xcoor >= 84 ) {
			break;
		}
	}
}

/*
 * prints печать строки на дисплей
 * Параметры:
 *   numstr - номер строки 0..5
 *   X - горизонтальная координата 0..84 (до 13 символов)
 *       если x == 0xFF, то берется сохраненная (предыдущая) координата
 *   *s  - указатель на строку
 */
//void dPrint(uint8_t numstr, uint8_t X, const char* s) {
//	uint16_t code;
//
//	Xcoor = X;
//
//	if ( Xcoor > 77 ) {
//		return;
//	} else if ( numstr > 5 ) {
//		return;
//	}
//	Ycoor = numstr * 8;
//	while ( *s != 0 ) {
//		code = *s;
//		if ( code >= 0x80 ) {//utf-8
//			code = code << 8;
//			code |= *(++s);
//		}
//		wrChar_x_8( Xcoor, Ycoor, 5, code);
//		Xcoor += 6;
//		s++;
//		if ( Xcoor >= 84 ) {
//			break;
//		}
//	}
//}

void disPrin(const char* s) {
	uint16_t code;

	if ( Xcoor > 77 ) {
		return;
	} else if ( Ycoor > 47 ) {
		return;
	}
	while ( *s != 0 ) {
		code = *s;
		if ( code >= 0x80 ) {//utf-8
			code = code << 8;
			code |= *(++s);
		}
		wrChar_x_8( Xcoor, Ycoor, 5, code);
		Xcoor += 6;
		s++;
		if ( Xcoor >= 84 ) {
			break;
		}
	}
}

/**
 * Преобразует 32 битное число в строку с нулем на конце
 * Параметры:
 *   numstr - номер строки 0..5
 *   X - горизонтальная координата 0..84 (до 13 символов)
 *       если x == 0xFF, то берется сохраненная (предыдущая) координата
 *   nmb - число для вывода
 */
void disUINT32_to_str (uint8_t numstr, uint8_t X, uint32_t nmb)
{
	char tmp_str [11] = {0,};
	int i = 0, j;
	uint8_t y;

	if ( X != 0xFF ) {
		Xcoor = X;
	}
	if ( Xcoor > 77 ) {
		return;
	} else if ( numstr > 5 ) {
		return;
	}
	y = numstr * 8;

	if (nmb == 0){//если ноль
		wrChar_x_8( Xcoor, y, 5, '0');
		Xcoor += 6;
	}else{
		while (nmb > 0) {
			tmp_str[i++] = (nmb % 10) + '0';
			nmb /=10;
		}
		for (j = 0; j < i; ++j) {
			wrChar_x_8( Xcoor, y, 5, tmp_str [i-j-1]);//перевернем
			Xcoor += 6;
			if ( Xcoor >= 84 ) {
				break;
			}
		}
	}
}

//===========================================================================
//===========================================================================
//для кругового буфера событий
#define DISP_LEN_BITS   4
#define DISP_LEN_BUF    (1<<DISP_LEN_BITS) // 8 или 2^3 или (1<<3)
#define DISP_LEN_MASK   (DISP_LEN_BUF-1)   // bits: 0000 0111
static uint8_t bufEv[DISP_LEN_BUF] = {0};
static uint8_t tail = 0;
static uint8_t head = 0;
/*
 * возвращает 1 если в кольцевом буфере есть свободное место для элемента, иначе 0
 */
static int dispHasFree(void) {
	if ( ((tail + 1) & DISP_LEN_MASK) == head ) {
		return 0;//свободного места нет
	}
	return 1;//есть свободное место
}
/*
 * помещает событие в круговой буфер
 * return 1 - успешно; 0 - нет места в буфере
 */
int dispPutEv(uint8_t event) {
	if (event == 0) {
		return 1;//событие с нулевым кодом пусть не будет для удобства
	}
	if ( dispHasFree() ) {
		bufEv[head] = event;
		head = (1 + head) & DISP_LEN_MASK;//инкремент кругового индекса
		return 1;
	} else {
		return 0;//нет места в буфере
	}
}
/*
 *  извлекает событие из кругового буфера
 *  если 0 - нет событий
 */
uint8_t dispGetEv(void) {
	uint8_t event = 0;
	if (head != tail) {//если в буфере есть данные
		event = bufEv[tail];
		tail = (1 + tail) & DISP_LEN_MASK;//инкремент кругового индекса
	}
	return event;
}
//===========================================================================
//===========================================================================

pdisp_t pdisp = emptyDisplay;

void display(void) {
	uint8_t event;
	static pdisp_t pdold = emptyDisplay;//указатель на предыдущую функцию меню
	int res = 0;

	event = dispGetEv();
	if ( event != 0 ) {
		res = pdisp(event);//отобразим функцию меню на экране (единственное место отображения)
		if ( pdold != pdisp ) {
			pdold = pdisp;
			dispPutEv( DIS_REPAINT );//меню поменялось, надо перерисовать
		}
	}

	if ( res ) {//если есть данные для отрисовки
		disDMAsend();
	}

	if ( SPI1->SR & SPI_I2S_FLAG_BSY ) {
		return;
	}
	if ( 1 == dstat ) {
		DMA_Cmd(DMA1_Channel3, DISABLE);
		CE_HI;
		dstat = 0;
	}
}


