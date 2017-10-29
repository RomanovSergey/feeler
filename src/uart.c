/*
 * uart.c
 *
 *  Created on: май 2016 г.
 *      Author: se
 *
 *  Эскейп коды взял отсюда:
 *  https://en.wikipedia.org/wiki/ANSI_escape_code#Sequence_elements
 */

#include "stm32f0xx.h"
#include "main.h"
#include "micro.h"
#include "uart.h"
#include "displayDrv.h"
#include "menu.h"

//===========================================================================
//===========================================================================
//для кругового буфера uart
#define URT_LEN_BITS   8
#define URT_LEN_BUF    (1<<URT_LEN_BITS)
#define URT_LEN_MASK   (URT_LEN_BUF-1)
static uint8_t urtbufEv[URT_LEN_BUF] = {0};
static uint16_t urttail = 0;
static uint16_t urthead = 0;
/*
 * возвращает 1 если в кольцевом буфере есть свободное место для элемента, иначе 0
 */
static int urtHasFree(void) {
	if ( ((urthead + 1) & URT_LEN_MASK) == urttail ) {
		return 0;//свободного места нет
	}
	return 1;//есть свободное место
}
/*
 * помещает byte в круговой буфер
 * return 1 - успешно; 0 - нет места в буфере
 */
int urtPut(uint8_t d) {
	if (d == 0) {
		return 1;//событие с нулевым кодом пусть не будет для удобства
	}
	if ( urtHasFree() ) {
		urtbufEv[urthead] = d;
		urthead = (1 + urthead) & URT_LEN_MASK;//инкремент кругового индекса
		return 1;
	} else {
		return 0;//нет места в буфере
	}
}
/*
 *  извлекает byte из кругового буфера
 *  если -1 - нет данных
 */
int urtGet(void) {
	int d = -1;
	if (urthead != urttail) {//если в буфере есть данные
		d = urtbufEv[urttail];
		urttail = (1 + urttail) & URT_LEN_MASK;//инкремент кругового индекса
	}
	return d;
}
//===========================================================================
//===========================================================================

static int uwork = 0;

/*
 * Периодически вызывается из main.c
 */
void uart(void) {
	int data;

	if ( uwork ) {
		return;
	}

	data = urtGet();
	if ( data != -1 ) {//если есть данные для отрисовки
		uwork = 1;
		USART_SendData(USART1, data);
		USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
	}
}

/*
 * Копирует строку str в буфер tx
 * символ конца строки - 0 (не копируется)
 */
int8_t urtPrint(const char *str) {
	uint16_t i = 0;

	while (str[i] != 0) {
		if ( urtPut( str[i] ) == 0 ) {
			return -1;//buffer overflow
		}
		i++;
	}
	return 0;//all ok
}

/**
 * Преобразует 32 битное число в строку с нулем на конце
 */
void urt_uint32_to_str (uint32_t nmb)
{
	char tmp_str [11] = {0,};
	int i = 0, j;
	if (nmb == 0){//если ноль
		urtPut( '0' );
	}else{
		while (nmb > 0) {
			tmp_str[i++] = (nmb % 10) + '0';
			nmb /=10;
		}
		for (j = 0; j < i; ++j) {
			urtPut( tmp_str [i-j-1] ); //перевернем
		}
	}
}

/**
 * Преобразует 16 битное число в 5и символьную строку
 */
void urt_uint16_to_bin(uint16_t n)
{
	for (int i = 15; i >= 0; i--) {
		urtPut( ( n & (1<<i) )?'1':'0' );
	}
	//urtPut( 0 ); //null terminator
}

/**
 * Convert char to hex string
 */
void char_to_strHex( uint8_t V, uint8_t *d )
{
	if ( (V >> 4) < 10 ) {
		*d++ = '0' + (V >> 4);
	} else {
		*d++ = (V >> 4) - 10 + 'A';
	}
	if ( (V & 0x0F) < 10 ) {
		*d++ = '0' + (V & 0x0F);
	} else {
		*d++ = (V & 0x0F) - 10 + 'A';
	}
}

void urt_uint32_to_hex(uint32_t nmb)
{
	uint8_t str[10];

	char_to_strHex( nmb >> 24, &str[0] );
	char_to_strHex( nmb >> 16, &str[2] );
	char_to_strHex( nmb >> 8,  &str[4] );
	char_to_strHex( nmb,       &str[6] );

	for ( int i = 0; i < 8; i++ ) {
		urtPut( str[i] );
	}
}

/**
 * Преобразует 16 битное число в 5и символьную строку + ноль на конце
 */
void urt_uint16_to_5str(uint16_t n)
{
	uint8_t dtis = '0';
	uint8_t tis = '0';
	uint8_t sot = '0';
	uint8_t des = '0';
	uint8_t edn = '0';

	while (n > 9999){
		n -= 10000;
		dtis++;
	}
	while (n > 999){
		n -= 1000;
		tis++;
	}
	while (n > 99){
		n -= 100;
		sot++;
	}
	while (n > 9){
		n -= 10;
		des++;
	}
	edn += n;
	urtPut( dtis );
	urtPut( tis );
	urtPut( sot );
	urtPut( des );
	urtPut( edn );
}

/*
 * Обработчик прерывания
 */
void USART1_IRQHandler(void) {

	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) { // recieve
		USART_SendData(USART1, USART_ReceiveData(USART1));
	}

	if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {
		int data = urtGet();
		if ( data == -1 ) { // конец данных?
			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
			USART_ClearITPendingBit(USART1, USART_IT_TC);
			USART_ITConfig(USART1, USART_IT_TC, ENABLE);
		} else {
			USART_SendData( USART1, data ); //отправим символ в порт
		}
	}

	if(USART_GetITStatus(USART1, USART_IT_TC) != RESET) { //последний символ отправлен
		USART_ClearFlag(USART1, USART_FLAG_TC);
		USART_ITConfig(USART1, USART_IT_TC, DISABLE);
		uwork = 0;
	}
}



