/*
 * helpers.h
 *
 *  Created on: 26 окт. 2017 г.
 *      Author: se
 */

#ifndef SRC_HELPERS_H_
#define SRC_HELPERS_H_

void itoa( int n, char s[] );
char* itostr( int n );
char* u16to4str( uint16_t n );
char* u32to5str( uint32_t n );

int decompressImg84x48( const uint8_t *ipic, uint8_t *opic, int iMAX );

#endif /* SRC_HELPERS_H_ */
