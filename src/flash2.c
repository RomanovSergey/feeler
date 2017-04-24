/*
 * flash2.c
 *
 *  Created on: Apr 24, 2017
 *      Author: se
 */

#include "stm32f0xx.h"
#include "stm32f0xx_flash.h"
#include "flash.h"
#include "uart.h"

#define PAGE60     0x0800F000
#define PAGE61     0x0800F400
#define PAGE62     0x0800F800
#define PAGE63     0x0800FC00
#define PAGE_SIZE  (PAGE61-PAGE60)

static const uint32_t adrBLOCK_A     = PAGE60;
static const uint32_t adrBLOCK_B     = PAGE62;
static const uint32_t BLOCK_SIZE  = PAGE_SIZE * 2;

static const uint16_t BLOCK_EMPTY = 0xFFFF; // empty block
static const uint16_t BLOCK_CURR  = 0xBBBB; // current block
static const uint16_t BLOCK_SHIFT = 0x0000; // in the process of switching

static const uint8_t  ADDLEN = 4; // IDLEN(2) + CS(2)

// Errors codes
#define FRES_OK             0  // ok, successful operation
#define FERR_WRITE_HW       1  // error while write half word data to flash
#define FERR_UNREACABLE   -1  // unreachable instruction


//static uint16_t flashBuf[256];

/*
 * calculates check summ
 */
uint16_t fcalcCS( const uint16_t *buf, uint8_t len )
{
	uint16_t cs = 0;
	for ( int i = 0; i < len; i++ ) {
		cs ^= buf[i];
	}
	return cs;
}

/*
 * Reads 2 bytes from flash by Address
 */
inline uint16_t fread16(uint32_t Address)
{
	return *(__IO uint16_t*)Address;
}

/*
 * Writes half word (hw) to flash at specified address (adr)
 * Return:
 *   FRES_OK - good
 *   FERR_WRITE_HW - error while write half word data to flash
 */
int fwriteHW( uint32_t adr, uint16_t hw )
{
	FLASH_Status fstat;
	FLASH_Unlock();
	fstat = FLASH_ProgramHalfWord( adr, hw ); // в статус блока записываются 0x0000
	FLASH_Lock();
	if ( fstat != FLASH_COMPLETE ) {
		return FERR_WRITE_HW; // error while write half word data to flash
	}
	return FRES_OK;
}

/*
 * To copy all active records from overflow block to empty block.
 *   adrCur - points to current block
 *   adrEmp - points to empty block
 * Return:
 *   code error
 */
int fmvBlock( uint32_t adrCur, uint32_t adrEmp, uint32_t *newAdr )
{
	return 0;
}

/*
 * Specifies *adr to the free cell,
 * enough for record with len data
 * Return:
 *   FRES_OK - good, *adr will point to free cell with enough space
 *   code error
 */
int fGetFreeSpace( uint32_t *address, uint8_t hwlen )
{
	int res;
	uint32_t endAdr;
	uint16_t data;
	uint32_t adr;
	uint32_t adrCur;
	uint32_t adrEmp;

	data = fread16( adrBLOCK_A );
	if ( data != 0xFFFF ) {
		adrCur = adrBLOCK_A;
		adrEmp = adrBLOCK_B;
	} else {
		adrCur = adrBLOCK_B;
		adrEmp = adrBLOCK_A;
	}
	adr = adrCur;
	endAdr = adrCur + BLOCK_SIZE - 2; // at end must be free space

	while (1) {
		data = fread16( adr );
		if ( data != 0xFFFF ) {
			adr += (uint8_t)(data & 0x00FF)*2 + ADDLEN;
			// ToDo: check adr out of block range
			if ( adr > endAdr) {
				res = fmvBlock( adrCur, adrEmp, &adr );
				if ( res != 0 ) {
					return res;
				}
			}
			continue;
		}
		// to calculate len
		if ( (adr + ADDLEN + hwlen*2) > endAdr ) {
			// not enough space

			return FERR_UNREACABLE;
		}
		// space is enough
		*address = adr;
		return FRES_OK;
	}
	return FERR_UNREACABLE;
}

// =======================================================================================
// ============================== interface functions ====================================

/*
 * Writes data to flash.
 * Params:
 *  IDLEN - hi byte - record's ID, lo byte - lenght of hw data (bytes * 2)
 *  *buf - points to the 16 bits data with lenght len bytes.
 * Return:
 *   0 - ok
 *   other - error
 */
int fwrite( const uint16_t IDLEN, const uint16_t* const buf )
{
	int res;
	uint32_t adr;
	uint8_t hwlen;

	adr = 0;
	hwlen = (uint8_t)(IDLEN & 0x00FF);

	// Specifies adr on the free cell and calculates
	// whether there is enough free space for writing
	res = fGetFreeSpace( &adr, hwlen );
	if ( res != 0 ) {
		return res;
	}

	// write IDLEN
	res = fwriteHW( adr, IDLEN );
	if ( res != FRES_OK ) {
		return res;
	}
	adr += 2;

	// write data
	for ( uint16_t i = 0 ; i < hwlen; i++ ) {
		res = fwriteHW( adr, buf[i] );
		if ( res != FRES_OK ) {
			return res;
		}
		adr += 2;
	}

	// calculate and write check summ
	uint16_t cs = fcalcCS( buf, hwlen );
	res = fwriteHW( adr, cs );
	if ( res != FRES_OK ) {
		return res;
	}
	return FRES_OK;
}

/*
 * Read data from flash.
 */
int fread( const uint16_t IDLEN, const uint16_t* const buf )
{

	return 1;
}

