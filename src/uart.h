/*
 * uart.h
 *
 *  Created on: 1 мая 2016 г.
 *      Author: se
 */

#ifndef INC_UART_H_
#define INC_UART_H_

void uart(void);

int8_t urtPrint(const char *str);
void urt_uint32_to_str (uint32_t nmb);
void urt_uint16_to_5str(uint16_t n);
void urt_uint16_to_bin(uint16_t n);
void urt_uint32_to_hex(uint32_t nmb);

#endif /* INC_UART_H_ */
