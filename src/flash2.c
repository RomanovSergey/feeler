/*
 * flash2.c
 *
 *  Created on: Apr 24, 2017
 *      Author: se
 */

#include "stm32f0xx.h"
#include "stm32f0xx_flash.h"
#include "flash2.h"
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
#define FERR_OVER_BLOCK     3  // record is out of block
#define FERR_PAGE_ADR       4  // page address is not correct
#define FERR_PAGE_ERASE     5  // error during page erasing
#define FERR_CS             6  // controll summ error
#define FERR_UNREACABLE    -1  // unreachable instruction

// inits on power on and keeps actual information of records in flash
typedef struct {
	uint32_t  curBlock;   // current block - where to write and read records
	uint32_t  ersBlock;   // erased block - where to move on overflow current block
	uint32_t  endAdr;     // last allowable addres of current block
	uint32_t  cell;       // start address erased cells of current block
	uint32_t  arec[FNUMREC]; // index - is ID, keep last address, if 0 - no record
} FlashManager;

static FlashManager fm;

/*
 * Check IDLEN for correct
 * Return:
 *   FRES_OK - *idnum and *hwdlen has result
 *   FERR_ID
 */
int fcheckId( uint16_t IDLEN, uint8_t *idnum, uint8_t *hwdlen )
{
	uint8_t id;

	id = (uint8_t)(IDLEN >> 8);
	if ( id == 0 ) {
		urtPrint("fcheckId: err: id=0\n");
		return FERR_ID;
	}
	if ( id > FNUMREC ) {
		urtPrint("fcheckId: err: id>FNUMREC\n");
		return FERR_ID;
	}
	*idnum = id;
	*hwdlen = (uint8_t)(IDLEN & 0x00FF);
	return FRES_OK;
}

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
 * Erase page flash
 * Return:
 *   FRES_OK - good
 *   FERR_PAGE_ADR
 *   FERR_PAGE_ERASE
 */
int ferasePage( uint32_t adr )
{
	uint32_t adrPage = 0;

	if ( adr < adrBLOCK_B ) { // ToDo: make it easy
		for ( int i = 0; i < BLOCK_SIZE / PAGE_SIZE; i++ ) {
			if ( adr == adrBLOCK_A + i * PAGE_SIZE ) {
				adrPage = adr;
			}
		}
	} else {
		for ( int i = 0; i < BLOCK_SIZE / PAGE_SIZE; i++ ) {
			if ( adr == adrBLOCK_B + i * PAGE_SIZE ) {
				adrPage = adr;
			}
		}
	}
	if ( adrPage == 0 ) {
		urtPrint("Err: ferasePage: adr not correct\n");
		return FERR_PAGE_ADR; // page address is not correct
	}

	FLASH_Unlock();
	FLASH_Status fstat =  FLASH_ErasePage( adrPage );
	FLASH_Lock();
	if ( fstat != FLASH_COMPLETE ) {
		urtPrint("Err: ferasePage: cant erase\n");
		return FERR_PAGE_ERASE; // error during page erasing
	}
	return FRES_OK;
}

/*
 * Erase Block
 * Return:
 *   FRES_OK - good
 *   FERR_PAGE_ADR
 *   FERR_PAGE_ERASE
 */
int feraseBlock ( uint32_t block )
{
	int ret = 0;
	uint32_t page = block;
	for ( int i = 0; i < BLOCK_SIZE / PAGE_SIZE; i++ ) {
		ret = ferasePage( page + i * PAGE_SIZE );
		if ( ret != FRES_OK ) {
			break;
		}
	}
	return ret;
}

/*
 * To copy all active records from overflow block to erased block.
 * Return:
 *   FRES_OK - good
 *   FERR_PAGE_ADR
 *   FERR_PAGE_ERASE
 */
int fswapBlocks( void )
{
	uint16_t data;
	uint32_t adr;
	uint32_t end;
	uint8_t  hwlen;
	int res = 0;

	uint32_t  curBlock;   // current block - where to write and read records
	uint32_t  ersBlock;   // erased block - where to move on overflow current block
	uint32_t  arec[FNUMREC]; // index - is ID, keep last address, if 0 - no record

	// init ===================================
	for ( int i = 0; i < FNUMREC; i++ ) {
		arec[i] = 0;
	}
	// ========================================

	adr = fm.ersBlock;
	end = adr + BLOCK_SIZE;
	while ( 1 ) { // to test - all data in erased block must be 0xFFFF
		data = fread16( adr );
		if ( data != 0xFFFF ) {
			res = 1; // need to erase block
			break;
		}
		adr += 2;
		if ( adr == end ) {
			break;
		}
	}
	if ( res != 0 ) {
		urtPrint("fswapBlocks: need to erase\n");
		res = feraseBlock ( fm.ersBlock );
		if ( res != 0 ) {
			urtPrint("eraseBlock error, need to continue\n");
		}
	}
	// copy all records to new block
	adr = fm.ersBlock;
	for ( int nr = 0; nr < FNUMREC; nr++ ) {
		if ( fm.arec[nr] != 0 ) {
			// write IDLEN
			data = fread16( fm.arec[nr] );
			hwlen = (uint8_t)(data & 0x00FF);
			res = fwriteHW( adr, data );
			if ( res != FRES_OK ) {
				urtPrint("fswapBlocks: err fwriteHW continue\n");
			}
			arec[nr] = adr;
			adr += 2;
			for ( uint16_t i = 0 ; i < (hwlen + 1); i++ ) { // with cs data
				data = fread16( fm.arec[nr] + i*2 );
				res = fwriteHW( adr, data );
				if ( res != FRES_OK ) {
					urtPrint("fswapBlocks: err fwriteHW continue\n");
				}
				adr += 2;
			}
		}
	}
	curBlock = fm.ersBlock;
	ersBlock = fm.curBlock;

	res = feraseBlock ( ersBlock );
	if ( res != 0 ) {
		urtPrint("eraseBlock error, need to continue\n");
	}

	// init fm sturct =========================
	fm.curBlock  = curBlock;
	fm.ersBlock  = ersBlock;
	fm.endAdr    = fm.curBlock + BLOCK_SIZE - 2;;
	fm.cell      = adr;
	for ( int i = 0; i < FNUMREC; i++ ) {
		fm.arec[i] = arec[i];
	}
	// ========================================

	return res;
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
	uint8_t  id;
	uint8_t  hwdlen;
	int  res = 0;

	// init fm sturct =========================
	fm.curBlock  = 0;
	fm.ersBlock  = 0;
	fm.endAdr    = 0;
	fm.cell      = 0;
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
	fm.cell = fm.curBlock;
	while (1) {
		data = fread16( fm.cell );
		if ( data != 0xFFFF ) {
			if ( data == 0 ) {
				fm.cell += 2;
				continue;
			}
			res = fcheckId( data, &id, &hwdlen );
			if ( res != FRES_OK ) {
				break;
			}
			// fill record's address
			fm.arec[id-1] = fm.cell;
			// go to the next record's address
			fm.cell += hwdlen*2 + ADDLEN;
			if ( fm.cell > fm.endAdr) {
				res = FERR_OVER_BLOCK; // need to swap blocks
				break;
			}
			continue;
		}
		res = 0;
		break;
	}
	if ( res != 0 ) {
		res = fswapBlocks();
	}
	return res;
}

/*
 * Writes data to flash.
 * Params:
 *  IDLEN - hi byte - record's ID, lo byte - lenght of hw data (bytes * 2)
 *  *buf - points to the 16 bits data with lenght len bytes.
 * Return:
 *   0 - ok
 *   FERR_ID
 *   FERR_WRITE_HW
 */
int fwrite( const uint16_t IDLEN, const uint16_t* const buf )
{
	int res;
	uint8_t idnum;
	uint8_t hwdlen;

	res = fcheckId( IDLEN, &idnum, &hwdlen );
	if ( res != FRES_OK ) {
		return res;
	}

	// is there enough free space for writing
	if ( (fm.endAdr - fm.cell) < (hwdlen * 2 + ADDLEN) ) { // ToDo: calculate rec len
		urtPrint("fwrite to fswapBlocks\n");
		fswapBlocks();
	}

	// write IDLEN
	res = fwriteHW( fm.cell, IDLEN );
	fm.cell += 2;
	if ( res != FRES_OK ) {
		return res;
	}

	// write data
	for ( uint16_t i = 0 ; i < hwdlen; i++ ) {
		res = fwriteHW( fm.cell, buf[i] );
		fm.cell += 2;
		if ( res != FRES_OK ) {
			return res;
		}
	}

	// calculate and write check summ
	uint16_t cs = fcalcCS( buf, hwdlen );
	res = fwriteHW( fm.cell, cs );
	fm.cell += 2;
	if ( res != FRES_OK ) {
		return res;
	}
	return FRES_OK;
}

/*
 * Read data from flash.
 */
int fread( const uint16_t IDLEN, uint16_t* const buf )
{
	int res;
	uint8_t  id;
	uint8_t  hwdlen;
	uint32_t adr;

	res = fcheckId( IDLEN, &id, &hwdlen );
	if ( res != FRES_OK ) {
		return res;
	}
	adr = fm.arec[id-1];

	// read data and write its to buf
	for ( uint16_t i = 0 ; i < hwdlen; i++ ) {
		buf[i] = fread16( adr );
		adr += 2;
	}

	uint16_t cs = fcalcCS( buf, hwdlen );
	if ( cs != fread16( adr ) ) {
		return FERR_CS;
	}
	return 1;
}

