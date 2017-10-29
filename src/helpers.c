/*
 * helpers.c
 *
 *  Created on: 26 окт. 2017 г.
 *      Author: se
 */

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

	if ((sign = n) < 0)  /* записываем знак */
		n = -n;          /* делаем n положительным числом */
	i = 0;
	do {       /* генерируем цифры в обратном порядке */
		s[i++] = n % 10 + '0';   /* берем следующую цифру */
	} while ((n /= 10) > 0);     /* удаляем */
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);
}

char* itostr( int n )
{
	static char s[11] = {0};
	int i, sign;

	if ((sign = n) < 0)  /* записываем знак */
		n = -n;          /* делаем n положительным числом */
	i = 0;
	do {       /* генерируем цифры в обратном порядке */
		s[i++] = n % 10 + '0';   /* берем следующую цифру */
	} while ((n /= 10) > 0);     /* удаляем */
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);
	return s;
}


/*void disHexHalfWord (uint8_t numstr, uint8_t X, uint16_t nmb)
{
	uint8_t str[10];

	if ( X != 0xFF ) {
		Xcoor = X;
	}
	if ( Xcoor > 77 ) {
		return;
	} else if ( numstr > 5 ) {
		return;
	}
	Ycoor = numstr * 8;

	char_to_strHex(nmb >> 8,   &str[0]);
	char_to_strHex(nmb & 0xFF, &str[2]);

	for ( int i = 0; i < 4; i++ ) {
		wrChar_5_8( Xcoor, Ycoor, str[i] );
		Xcoor += 6;
	}
}*/

/*void disUINT32_to_strFONT2 (uint8_t numstr, uint8_t X, uint32_t nmb)
{
	char tmp_str [11] = {0,};
	int i = 0, j;
	uint8_t y;

	if ( X != 0xFF ) {
		Xcoor = X;
	}
	if ( Xcoor > 77 ) {
		return;
	} else if ( numstr > 2 ) {
		return;
	}
	y = numstr * 16;

	if (nmb == 0){//если ноль
		wrChar_10_16( Xcoor, y, '0');
		Xcoor += 12;
	}else{
		while (nmb > 0) {
			tmp_str[i++] = (nmb % 10) + '0';
			nmb /=10;
		}
		for (j = 0; j < i; ++j) {
			wrChar_10_16( Xcoor, y, tmp_str [i-j-1]);//перевернем
			Xcoor += 12;
			if ( Xcoor >= 84 ) {
				break;
			}
		}
	}
}
*/
