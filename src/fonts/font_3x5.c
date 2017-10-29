/*
 * font_3x5.c
 *
 *  Created on: 29 окт. 2017 г.
 *      Author: se
 */

#include "stm32f0xx.h"

#define FONs_T static const char

FONs_T fs_0x30[3] = {0x1f, 0x11, 0x1f}; // 0
FONs_T fs_0x31[3] = {0x12, 0x1f, 0x10}; // 1
FONs_T fs_0x32[3] = {0x1d, 0x15, 0x17}; // 2
FONs_T fs_0x33[3] = {0x11, 0x15, 0x1f}; // 3
FONs_T fs_0x34[3] = {0x07, 0x04, 0x1f}; // 4
FONs_T fs_0x35[3] = {0x17, 0x15, 0x1d}; // 5
FONs_T fs_0x36[3] = {0x1f, 0x15, 0x1d}; // 6
FONs_T fs_0x37[3] = {0x01, 0x1d, 0x03}; // 7
FONs_T fs_0x38[3] = {0x1f, 0x15, 0x1f}; // 8
FONs_T fs_0x39[3] = {0x17, 0x15, 0x1f}; // 9
FONs_T fs_0x2E[3] = {0x00, 0x10, 0x00}; // .

typedef struct {
	uint16_t code;
	const char* img;
} TUcode_t;

TUcode_t font3x5[] = {
		{ 0x20, fs_0x2E },
		{ 0x30, fs_0x30 },
		{ 0x31, fs_0x31 },
		{ 0x32, fs_0x32 },
		{ 0x33, fs_0x33 },
		{ 0x34, fs_0x34 },
		{ 0x35, fs_0x35 },
		{ 0x36, fs_0x36 },
		{ 0x37, fs_0x37 },
		{ 0x38, fs_0x38 },
		{ 0x39, fs_0x39 },
};

const char* getFont3x5( const uint16_t key)
{
	int len = sizeof( font3x5 ) / sizeof ( TUcode_t );

	int found = 0;
	int high = len - 1, low = 0;
	int middle = (high + low) / 2;
	while ( !found && high >= low ){
		if ( key == font3x5[middle].code ) {
			found = 1;
			break;
		} else if (key < font3x5[middle].code ) {
			high = middle - 1;
		} else {
			low = middle + 1;
		}
		middle = (high + low) / 2;
	}
	return (found == 1) ? font3x5[middle].img : fs_0x2E ;
}
