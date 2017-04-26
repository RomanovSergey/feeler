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

static const uint8_t  ADDLEN = 4; // IDLEN(2) + CS(2)

// Errors codes
#define FRES_OK             0  // ok, successful operation
#define FERR_WRITE_HW       1  // error while write half word data to flash
#define FERR_ID             2  // record's ID is not allowable
#define FERR_UNREACABLE    -1  // unreachable instruction

#define FNUMREC   5  // numbers of ID, from 1 to FNUMREC

// inits on power on and keeps actual information of records in flash
typedef struct {
	uint32_t  curBlock;   // current block - where to write and read records
	uint32_t  ersBlock;   // erased block - where to move on overflow current block
	uint32_t  endAdr;     // last allowable addres of current block
	uint32_t  cell;       // start address erased cells of current block
	uint16_t  freeSpace;  // available free space in current block
	uint32_t  arec[FNUMREC]; // index - is ID, keep last address, if 0 - no record
} FlashManager;

static FlashManager fm;

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
 * called in power on, to initialise fm struct
 * Return:
 *   FERR_ID
 */
int flashInit(void)
{
	uint16_t data;
	uint32_t adr;
	uint8_t  id;

	// init fm sturct =========================
	fm.curBlock  = 0;
	fm.ersBlock  = 0;
	fm.cell      = 0;
	fm.freeSpace = 0;
	fm.endAdr    = 0;
	for ( int i = 0; i < FNUMREC; i++ ) {
		fm.arec[i] = 0;
	}
	// ========================================

	data = fread16( adrBLOCK_A );
	if ( data != 0xFFFF ) {
		fm.curBlock = adrBLOCK_A;
		fm.ersBlock = adrBLOCK_B;
	} else {
		fm.curBlock = adrBLOCK_B;
		fm.ersBlock = adrBLOCK_A;
	}

	fm.endAdr = fm.curBlock + BLOCK_SIZE - 2; // at end must be free space
	adr = fm.curBlock;
	while (1) {
		data = fread16( adr );
		if ( data != 0xFFFF ) {
			id  = (uint8_t)(data >> 8);
			if ( id > FNUMREC ) {
				urtPrint("flashInit: err: id\n");
				return FERR_ID;
			}
			adr += (uint8_t)(data & 0x00FF)*2 + ADDLEN;
			if ( adr > fm.endAdr) {
				// need to move block
				fm.cell = adr;
				fm.freeSpace = fm.endAdr - fm.cell;
				break;
			}
			continue;
		}
		fm.cell = adr;
		fm.freeSpace = fm.endAdr - fm.cell;
		break;
	}
	return FRES_OK;
}

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

