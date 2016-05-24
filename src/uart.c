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

#define TX_SIZE 256

struct Tsend{
	uint16_t  ind;//указывает на нулевой символ строки (для след. записи)
	uint8_t   buf[TX_SIZE];
};
static struct Tsend tx;//буфер для отправки по уарт

//прототипы
int8_t copyToBuf(char *str);
void uint32_to_str (uint32_t nmb);
void uint16_to_5str(uint16_t n);
void uint16_to_bin(uint16_t n);
void printRun(void);


/*
 * Периодически вызывается из main.c
 */
void uart(void) {

static uint16_t min = 0xFFFF;
static uint16_t max = 0;
static uint16_t cnt = 0;
static uint32_t sum = 0;

	if (g.ADC_done == 1) {
		g.ADC_done  = 0;

		tx.ind = 0;
		copyToBuf("\033[2J");//clear entire screen
		copyToBuf("\033[?25l");//Hides the cursor.
		copyToBuf("\033[H");//Move cursor to upper left corner.
		printRun();//крутящаяся черточка
		copyToBuf("\r\n");

		copyToBuf("\r\n ADC_value = ");//=================================
		copyToBuf("\033[31m");//set red color
		uint16_to_5str( (uint16_t)(g.ADC_value) );
		copyToBuf("\033[0m");//reset normal (color also default)
		copyToBuf(" adc ");
		uint16_to_bin( (uint16_t)g.ADC_value );
		copyToBuf(" bin \r\n");

		copyToBuf(" ADC_deltaTime = ");
		uint16_to_5str( (uint16_t)g.ADC_deltaTime );
		copyToBuf("\r\n");

		g.ADC_deltaTime *= 1000000;
		uint32_t Lval = g.ADC_deltaTime / g.ADC_value;
		copyToBuf(" Lval = ");
		uint32_to_str( Lval );
		copyToBuf("\r\n");

		if ( min > g.ADC_value ) {
			min = g.ADC_value;
		}
		copyToBuf(" min = ");
		uint16_to_5str( min );

		if ( max < g.ADC_value ) {
			max = g.ADC_value;
		}
		copyToBuf("\r\n max = ");
		uint16_to_5str( max );

		cnt++;
		sum += g.ADC_value;

		copyToBuf("\r\n avg = ");
		uint16_to_5str( (uint16_t)(sum / cnt) );

		copyToBuf("\r\n\r\n cnt = ");
		uint16_to_5str( cnt );


		USART_SendData(USART2, tx.buf[0]);
		tx.ind = 1;
		USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
	}
}

/*
 * Копирует строку str в буфер tx
 * символ конца строки - 0 (не копируется)
 */
int8_t copyToBuf(char *str) {
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
void uint32_to_str (uint32_t nmb)//, char * buf)
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
			//*(buf++) = tmp_str [i-j-1];//перевернем
			tx.buf[tx.ind++] = tmp_str [i-j-1];//перевернем
		}
	}
	//*buf = 0;//null terminator
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
			USART_ClearITPendingBit(USART2, USART_IT_TC);
			USART_ITConfig(USART2, USART_IT_TC, ENABLE);
			USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
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



