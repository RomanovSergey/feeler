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




