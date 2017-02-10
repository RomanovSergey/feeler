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
#include "menu.h"

struct Tsend tx;//буфер для отправки по уарт
int (*pmenu)(uint8_t) = powerOn;//указатель на функцию меню

/*
 * Периодически вызывается из main.c
 */
void uart(void) {
	uint8_t event;
	static int (*pmold)(uint8_t) = powerOn;//указатель на предыдущую функцию меню
	int res = 0;

	event = get_event();
	if ( event != 0 ) {
		res = pmenu(event);//отобразим функцию меню на экране (единственное место отображения)
		if ( pmold != pmenu ) {
			pmold = pmenu;
			put_event( Erepaint );//меню поменялось, надо перерисовать
		}
	}

	if ( res ) {//если есть данные для отрисовки
		USART_SendData(USART1, tx.buf[0]);
		tx.ind = 1;
		USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
	}
}

/*
 * Копирует строку str в буфер tx
 * символ конца строки - 0 (не копируется)
 */
int8_t toPrint(const char *str) {
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
	static const uint8_t s_run[]={'\\','|','/','-'};
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
void USART1_IRQHandler(void) {

	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
		USART_SendData(USART1, USART_ReceiveData(USART1));
	}

	if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {
		if (tx.buf[tx.ind] == 0) {//нулевой символ? (конец данных)
			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
			USART_ClearITPendingBit(USART1, USART_IT_TC);
			USART_ITConfig(USART1, USART_IT_TC, ENABLE);
		} else if (tx.ind < TX_SIZE) {//не дошли до конца буфера? и не поймали нулевой символ
			USART_SendData(USART1, tx.buf[tx.ind++]);//отправим символ в порт
		} else {//значит уже дошли до конца буфера
			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
			USART_ClearITPendingBit(USART1, USART_IT_TC);
			USART_ITConfig(USART1, USART_IT_TC, ENABLE);
		}
	}

	if(USART_GetITStatus(USART1, USART_IT_TC) != RESET) {//последний символ отправлен
		USART_ClearFlag(USART1, USART_FLAG_TC);
		USART_ITConfig(USART1, USART_IT_TC, DISABLE);
	}
}



