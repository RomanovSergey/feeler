/*
 * uart.c
 *
 *  Created on: 1 мая 2016 г.
 *      Author: se
 *
 *  Эскейп коды взял отсюда:
 *  https://en.wikipedia.org/wiki/ANSI_escape_code#Sequence_elements
 */

#include "stm32f0xx.h"
#include "main.h"
#include "micro.h"

#define TX_SIZE 256

struct Tsend{
	uint16_t  ind;//указывает на нулевой символ строки (для след. записи)
	uint8_t   buf[TX_SIZE];
};
static struct Tsend tx;//буфер для отправки по уарт

//прототипы
int8_t toPrint(char *str);
void uint32_to_str (uint32_t nmb);
void uint16_to_5str(uint16_t n);
void uint16_to_bin(uint16_t n);
void printRun(void);

/*
 * Периодически вызывается из main.c
 */
void uart(void) {
static uint16_t cnt = 0;

	if (g.tim_done == 1) {
		g.tim_done  = 0;

		tx.ind = 0;
		toPrint("\033[2J");//clear entire screen
		toPrint("\033[?25l");//Hides the cursor.
		toPrint("\033[H");//Move cursor to upper left corner.
		printRun();//крутящаяся черточка
		toPrint("\r\n");

		toPrint("\r\n Tim_len = ");//=================================
		toPrint("\033[31m");//set red color
		uint32_to_str( g.tim_len );
		toPrint("\033[0m");//reset normal (color also default)
		toPrint(" y.e. \r\n");

		toPrint("\r\n\r\n cnt = ");
		uint16_to_5str( cnt++ );

		USART_SendData(USART2, tx.buf[0]);
		tx.ind = 1;
		USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
	}
}

/*
void uart(void) {

static uint16_t min = 0xFFFF;
static uint16_t max = 0;
static uint16_t cnt = 0;
static uint32_t sum = 0;

	if (g.ADC_done == 1) {
		g.ADC_done  = 0;

		tx.ind = 0;
		toPrint("\033[2J");//clear entire screen
		toPrint("\033[?25l");//Hides the cursor.
		toPrint("\033[H");//Move cursor to upper left corner.
		printRun();//крутящаяся черточка
		toPrint("\r\n");

		toPrint("\r\n ADC_value = ");//=================================
		toPrint("\033[31m");//set red color
		uint16_to_5str( (uint16_t)(g.ADC_value) );
		toPrint("\033[0m");//reset normal (color also default)
		toPrint(" adc ");
		uint16_to_bin( (uint16_t)g.ADC_value );
		toPrint(" bin \r\n");

		toPrint(" ADC_deltaTime = ");
		uint16_to_5str( (uint16_t)g.ADC_deltaTime );
		toPrint("\r\n");

		g.ADC_deltaTime *= 1000000;
		uint32_t Lval = g.ADC_deltaTime / g.ADC_value;
		toPrint(" Lval   = ");
		uint32_to_str( Lval );
		toPrint("\r\n");

		toPrint(" micron = ");
		uint16_to_5str( micro(Lval) );
		toPrint("\r\n");

		if ( min > g.ADC_value ) {
			min = g.ADC_value;
		}
		toPrint(" min = ");
		uint16_to_5str( min );

		if ( max < g.ADC_value ) {
			max = g.ADC_value;
		}
		toPrint("\r\n max = ");
		uint16_to_5str( max );

		cnt++;
		sum += g.ADC_value;

		toPrint("\r\n avg = ");
		uint16_to_5str( (uint16_t)(sum / cnt) );

		toPrint("\r\n\r\n cnt = ");
		uint16_to_5str( cnt );


		USART_SendData(USART2, tx.buf[0]);
		tx.ind = 1;
		USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
	}
}
*/

/*
 * Копирует строку str в буфер tx
 * символ конца строки - 0 (не копируется)
 */
int8_t toPrint(char *str) {
	uint16_t i = 0;

	while (str[i] != 0) {
		if (tx.ind < TX_SIZE) {
			tx.buf[tx.ind] = (uint8_t)str[i];
			tx.ind++;
			i++;
		} else {
			return -1;//buffer overflow
		}
	}
	tx.buf[tx.ind] = 0;
	return 0;//all ok
}

//крутящаяся черточка
void printRun(void) {
	static const uint8_t s_run[]={'\\','|','/','|'};
	static uint8_t run = 0;
	tx.buf[tx.ind++] = s_run[run];//напечатать меняющийся символ
	tx.buf[tx.ind] = 0;
	run++;
	if ( (run + 1) > sizeof(s_run) )
	{run = 0;}
}

/**
 * Преобразует 32 битное число в строку с нулем на конце
 */
void uint32_to_str (uint32_t nmb)
{
	char tmp_str [11] = {0,};
	int i = 0, j;
	if (nmb == 0){//если ноль
		//*(buf++) = '0';
		tx.buf[tx.ind++] = '0';
	}else{
		while (nmb > 0) {
			tmp_str[i++] = (nmb % 10) + '0';
			nmb /=10;
		}
		for (j = 0; j < i; ++j) {
			tx.buf[tx.ind++] = tmp_str [i-j-1];//перевернем
		}
	}
	tx.buf[tx.ind] = 0;//null terminator
}

/**
 * Преобразует 16 битное число в 5и символьную строку + ноль на конце
 */
void uint16_to_bin(uint16_t n)
{
	for (int i = 15; i >= 0; i--) {
		tx.buf[tx.ind++] = ( n & (1<<i) )?'1':'0';
	}
	tx.buf[tx.ind] = 0;
}

/**
 * Преобразует 16 битное число в 5и символьную строку + ноль на конце
 */
void uint16_to_5str(uint16_t n)
{
	tx.buf[tx.ind] = '0';
	tx.buf[tx.ind + 1] = '0';
	tx.buf[tx.ind + 2] = '0';
	tx.buf[tx.ind + 3] = '0';
	tx.buf[tx.ind + 4] = '0';
	tx.buf[tx.ind + 5] = 0;
	while (n > 9999){
		n -= 10000;
		tx.buf[tx.ind]++;
	}
	while (n > 999){
		n -= 1000;
		tx.buf[tx.ind + 1]++;
	}
	while (n > 99){
		n -= 100;
		tx.buf[tx.ind + 2]++;
	}
	while (n > 9){
		n -= 10;
		tx.buf[tx.ind + 3]++;
	}
	tx.buf[tx.ind + 4] += n;
	tx.ind += 5;
}

/*
 * Обработчик прерывания
 */
void USART2_IRQHandler(void) {

	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
		USART_SendData(USART2, USART_ReceiveData(USART2));
	}

	if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET) {
		if (tx.buf[tx.ind] == 0) {
			if ( g.buf[0] == 0 ) {//в отладочном буфере нет данных
				USART_ClearITPendingBit(USART2, USART_IT_TC);
				USART_ITConfig(USART2, USART_IT_TC, ENABLE);
				USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
			} else {//если в отладочном буфере есть данные
				int i = 0;
				do {//копируем данные из отладочного буфера в tx
					tx.buf[i] = g.buf[i];
					i++;
				} while ( tx.buf[i] != 0 );
				g.buf[0] = 0;//в отладочном буфере больше нет данных
				g.ind = 0;
				USART_SendData(USART2, tx.buf[0]);
				tx.ind = 1;
			}
		} else if (tx.ind < TX_SIZE) {
			USART_SendData(USART2, tx.buf[tx.ind++]);
		} else {
			USART_ClearITPendingBit(USART2, USART_IT_TC);
			USART_ITConfig(USART2, USART_IT_TC, ENABLE);
			USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
		}
	}
	if(USART_GetITStatus(USART2, USART_IT_TC) != RESET) {
		USART_ClearFlag(USART2, USART_FLAG_TC);
		USART_ITConfig(USART2, USART_IT_TC, DISABLE);
	}
}



