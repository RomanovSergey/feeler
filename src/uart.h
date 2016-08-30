/*
 * uart.h
 *
 *  Created on: 1 мая 2016 г.
 *      Author: se
 */

#ifndef INC_UART_H_
#define INC_UART_H_

#define TX_SIZE 256
struct Tsend{
	uint16_t  ind;//указывает на нулевой символ строки (для след. записи)
	uint8_t   buf[TX_SIZE];
};

extern struct Tsend tx;


extern int(*pmenu)(uint8_t);

void uart(void);

int8_t toPrint(const char *str);
void uint32_to_str (uint32_t nmb);
void uint16_to_5str(uint16_t n);
void uint16_to_bin(uint16_t n);
void printRun(void);


#endif /* INC_UART_H_ */
