/*
 * helpers.c
 *
 *  Created on: 26 окт. 2017 г.
 *      Author: se
 */

#include "stm32f0xx.h"
#include <string.h>

/* reverse:  переворачиваем строку s на месте */
static void reverse(char s[])
{
	int i, j;
	char c;

	for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

/* itoa:  конвертируем n в символы в s */
void itoa(int n, char s[])
{
	int i, sign;

	if ( (sign = n) < 0 ) { n = -n; }
	i = 0;
	do {
		s[i++] = n % 10 + '0';
	} while ( (n /= 10) > 0 );
	if ( sign < 0 ) { s[i++] = '-'; }
	s[i] = '\0';
	reverse( s );
}

char* itostr( int n )
{
	static char s[11] = {0};

	itoa( n, s );
	return s;
}

char* u16to4str( uint16_t n )
{
	static char s[11] = {0};
	int i = 0;

	if ( n > 9999 ) { // only 4 digits allow
		strcpy( s, " Err");
		return s;
	}
	do {
		s[i++] = n % 10 + '0';
	} while ( (n /= 10) > 0 );

	while ( i < 4 ) {
		s[i++] = '0'; // дополним спереди нули
	}
	s[i] = '\0';
	reverse( s );
	return s;
}

char* u32to5str( uint32_t n )
{
	static char s[11] = {0};
	int i = 0;

	if ( n > 99999 ) { // only 5 digits allow
		strcpy( s, " Err");
		return s;
	}
	do {
		s[i++] = n % 10 + '0';
	} while ( (n /= 10) > 0 );

	while ( i < 5 ) {
		s[i++] = '0'; // дополним спереди нули
	}
	s[i] = '\0';
	reverse( s );
	return s;
}


#define CMD_ZER   0x40
#define CMD_ONE   0x80
#define CMD_FOL   0xc0

#define PICSIZE  504
/*
 * Decompress compressed image 84x48
 * Params:
 *   ipic - pointer to compressed input array of image
 *   opic - pointer ot output array of decompressed image
 *   OMAX - size of input array of image
 * Return:
 *   0 - good
 *  <0 - error
 */
int decompressImg84x48( const uint8_t *ipic, uint8_t *opic, int iMAX )
{
    int      iind = 0; // input index array
    int      oind = 0; // output index array
    int      cnt;      // intermediate helper counter

    if ( (ipic == NULL) || (opic == NULL) ) { return -1; }

    while ( (iind < iMAX) && (oind < PICSIZE) ) {
        if ( (ipic[iind] & 0xC0) == CMD_ZER ) { // reduce 0x00, max 64 bytes
            cnt = ( ipic[iind] & 0x3F ) + 1;
            for ( int n = 0; n < cnt; n++ ) {
                if ( oind >= PICSIZE ) { return -2; }
                opic[oind++] = 0x00;
            }
            iind++;
        } else if ( (ipic[iind] & 0xC0) == CMD_ONE ) { // reduce 0xFF, max 64 bytes
            cnt = ( ipic[iind] & 0x3F ) + 1;
            for ( int n = 0; n < cnt; n++ ) {
                if ( oind >= PICSIZE ) { return -3; }
                opic[oind++] = 0xFF;
            }
            iind++;
        } else if ( (ipic[iind] & 0xC0) == CMD_FOL ) { // follows other bytes - not reduces
            cnt = ( ipic[iind] & 0x3F ) + 1;
            if ( (iind + cnt + 1) > iMAX ) { return -4; }
            for ( int n = 0; n < cnt; n++ ) {
                if ( oind >= PICSIZE ) { return -5; }
                opic[oind++] = ipic[iind + n + 1];
            }
            iind += (cnt + 1);
        } else { // error: no cmd found
            return -6;
        }
    }

    if ( iind == iMAX ) {
        return 0;
    } else {
        return 10;
    }
    return 11;
}


