/*
 * flash.c
 *
 *  Created on: 12 марта 2017 г.
 *      Author: se
 *
 *  Module not complete yet
 *
 *  Purpose of this module is work with flash memory.
 *  Implemented algorithms for write and read records,
 *    switch to another block on overflow.
 *  Read and write are implemented with packet with defined length.
 *
 * ===================================================================================
 * Algorithm for write and read information to/from internal stm32 flash memory.
 *
 * Microcontroller allows to write to flash only 2 bytes (16 bits) at once
 *   and to the eased cell (0xFFFF - checked by controller).
 *   Only exception to this is when 0x0000 is programmed.
 * Addresses of writes must be even value.
 *   We can erase only whole page (1 kB on this device), after erase all cells
 * will stay to 0xFFFF value.
 *
 * To store information on flash, we will be use packets (records) in next format:
 *   START, ID, LEN, DATA[], XOR
 * , where:
 *   START  - start tag of the record: 0xABCD (purpose of this: to catch bugs)
 *   ID     - record's identifier, if ID == 0x0000: record is erased
 *   LEN    - lenght of the record in bytes, from START, to XOR inclusive (even number)
 *   DATA[] - usefull data, by 16 bits, has lenght = LEN - 8 bytes
 *   XOR    - controll summ of DATA[]
 *
 * Record on the flash should be unique, that is, records with the same ID
 *   should not be repeated.
 * If you want to delete record with specific ID without erasing the entire page,
 *   then 0x0000 are written instead of the ID field, the remaining fields
 *   of the records are not touched.
 * The search for an entry in the block by ID occurs according to the algorithm:
 *   the first byte should always be the starting one, then look at the ID,
 *   if the ID is not ours, we increase the address to LEN bytes and iteration
 *   is repeated until we find the desired ID or until we come across
 *   an empty cell or until it ends Page of memory.
 * If you want to write new data with a specific ID, first delete the old record
 *   with the given ID (if it is), then find the first clean cell (0xFFFF)
 *   and if there is enough space for writing - write it down.
 *
 * ===================================================================================
 * В качестве блока будем использовать 2 смежные страницы памяти - для увеличения
 *   объема хранилища.
 * В блоке будем различать:
 *   - свободное пространство : после стирания (имеет 0xFFFF полуслова);
 *   - рабочее пространство   : записанная полезная информация (запись);
 *   - удаленное пространство : запись с обнуленным ID
 *
 * Будем использовать 2 блока. Один из них будем держать свободным (после стирания)
 *   другой блок текущим. Назовем блоки A и B.
 * Переключение с блока A на блок B:
 *   Как только блок А заполнится (т.е. свободного пространства окажется не достаточным
 *   для новой записи), копируем из блока А рабочие записи в блок B.
 *   После этого блок A стираем, а запись новой информации будем вести в блоке B.
 *   Переключение с блока B на блок A - аналогично.
 *
 * Число записей с различными ID должно быть ограничено, иначе после переключения
 *   с одного блока на другой, свободного пространства может стать вновь не достаточным.
 *
 * Под начало блока забираем 2 байта - статус блока, в которой будет хранится
 *   следующая информация:
 *   - 0xFFFF : пустой блок
 *   - 0xBBBB : текущий блок
 *   - 0x0000 : в процессе переключения с данного блока на пустой блок
 *
 * Процесс переключения блока происходит следующим образом:
 *   - в статус блока записываются 0x0000
 *   - другой блок должен быть пустым
 *   - копируются все актуальные записи в новый блок
 *   - удаляется запись в старом блоке
 *   - после копирования всех записей, новый блок помечается как текущий а старый стираем
 *
 * Перед поиском записи в блоке переходим в текущий блок.
 * При операции чтения данных возможны варианты:
 *   - нет данных: здесь софт решает что делать
 *   - ошибка данных: сообщение пользователю об ошибке
 *   - успешное чтение
 *
 * При операции записи возможны варианты:
 *   - успешная операция
 *   - не достаточно свободного места: произвести переключение блока
 *   - ошибка записи: сообщить обо ошибке
 *
 * Если перед операцией чтения или записи текущий блок не найден, то необходимо
 *   его создать. Для этого проверяются текуще блоки.
 *   - помечен ли какой либо блок как в процессе переключения
 *   - если да продолжим процедуру переключения
 *   - если блоков в процессе переключения тоже нет, выбираем младший по адресу блок
 *     и помечаем его текущим.
 *
 * ===================================================================================
 *
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

static const uint32_t BLOCK_A     = PAGE60;
static const uint32_t BLOCK_B     = PAGE62;
static const uint32_t BLOCK_SIZE  = PAGE_SIZE * 2;

static const uint16_t BLOCK_EMPTY = 0xFFFF; // empty block
static const uint16_t BLOCK_CURR  = 0xBBBB; // current block
static const uint16_t BLOCK_SHIFT = 0x0000; // in the process of switching

static const uint16_t START = 0xABCD; // start record tag
static const uint16_t END   = 0xEEEE; // end record tag

#define MIN_LEN   12   // Minimum record length (data + 10 service bytes)
#define MAX_LEN   200  // Maximum record length, in bytes

static uint16_t flashBuf[MAX_LEN << 2];

// Errors codes
#define FRES_OK             0  // ok, successful operation
#define FERR_WRITE_HW       1  // error while write half word data to flash
#define FERR_FND_EMP_CELL   2  // not found ID, catch empty cell
#define FERR_NO_START       3  // error: no START tag
#define FERR_REC_LEN_MIN    4  // record's lenght is less then allowable
#define FERR_REC_LEN_MAX    5  // record's lenght is greater then allowable
#define FERR_OUTOF_BLOCK    6  // out of block range
#define FERR_MISMATCH_ID    7  // the record's ID doesn't match the address
#define FERR_REMOVE_ID      8  // error while remove the old record
#define FERR_ADR_UNCORRECT  9  // address is not correct
#define FERR_PAGE_ERASE    10  // error during page erasing
#define FERR_EMP_NFOUND    11  // curBlock found but embBlock not found
#define FERR_NO_CURR_EMP   12  // non cubBlock and non empBlock
#define FERR_REC_LEN       13  // record's length is not correct
#define FERR_MISS_ST_ID    14  // miss START or Id in flashBuf[]
#define FERR_NOT_SHIFT     15  // block is not in Shift state
#define FERR_NOT_EMPTY     16  // block is not in Empty state
#define FERR_LEN_NOT_EVEN  17  // len is not even
#define FERR_BUF_LITTLE    18  // buff is too little for record

#define FERR_UNREACABLE   -1  // unreachable instruction

/*
 * calculates xor summ
 */
uint16_t fcalcXOR( const uint16_t *buf, uint16_t len )
{
	uint16_t vxor = 0;
	len = len >> 1;
	for ( int i = 0; i < len; i++ ) {
		vxor ^= buf[i];
	}
	return vxor;
}

/*
 * Reads 2 bytes from flash by Address
 */
inline uint16_t fread16(uint32_t Address)
{
	return *(__IO uint16_t*)Address;
}

/*
 * Reads ID data from base startAdr record's address
 */
inline uint16_t fgetId( uint32_t startAdr )
{
	return fread16( startAdr + 2 );
}

/*
 * Reads Len data from base startAdr record's address
 */
inline uint16_t fgetLen( uint32_t startAdr )
{
	return fread16( startAdr + 4 );
}

/*
 * Reads Len data from base startAdr record's address
 *   startAdr - adr with START tag
 *   *len - where will be result
 * Return:
 *   FRES_OK (0) - ok, result in len
 *   FERR_REC_LEN_MIN
 *   FERR_REC_LEN_MAX
 */
int fgetAndCheckLen( uint32_t startAdr, uint16_t *len )
{
	*len = fgetLen( startAdr );
	if ( *len < MIN_LEN ) {
		return FERR_REC_LEN_MIN;
	}
	if ( *len > MAX_LEN ) {
		return FERR_REC_LEN_MAX;
	}
	return 0;
}

/*
 * Writes half word to flash at adr address
 * Return:
 *   code error
 */
int fwriteHalfWord( uint32_t adr, uint16_t hw )
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
 * Looking for record on flash with ID
 * Params:
 *   ID     - record's ID which we want to find
 *   *padr  - pointer from wich begin looking for, result will be also here
 *   endAdr - end address of the current block
 * Return:
 *   FRES_OK - ok: ID is found, *addr points to the base record's address
 *   other - code error
 */
int fFindIDaddr( uint16_t ID, uint32_t *padr, uint32_t endAdr )
{
	uint16_t data;
	uint16_t len;
	uint16_t id;

	do {
		data = fread16( *padr );
		if ( data == START ) {
			id = fgetId( *padr ); // loock at the ID
			if ( ID == id ) {
				urtPrint("Find ID: ");
				urt_uint32_to_str(ID);
				urtPrint(", at adr: ");
				urt_uint32_to_hex( *padr );
				urtPrint("\n");
				return FRES_OK; // founded!
			} else {
				// look at the LEN
				int res;
				res = fgetAndCheckLen( *padr, &len );
				if ( res == 0 ) {
					*padr += len; // will go to the next record
					if ( *padr >= endAdr ) {
						urtPrint("Err: fFindIDaddr: out of block\n");
						return FERR_OUTOF_BLOCK; // out of block range
					}
				} else {
					return res;
				}
			}
		} else if ( data == 0xFFFF ) {
			return FERR_FND_EMP_CELL; // catch empty cell
		} else {
			urtPrint("Err: fFindIDaddr: no START\n");
			return FERR_NO_START; // no start tag
		}
	} while ( 1 );
	return FERR_UNREACABLE; // unreachable instruction
}

/*
 * To delete recort with ID at base recor's address adr
 * Return:
 *   error code
 */
int fdeleteID( uint16_t ID, uint32_t adr ) {
	uint16_t data;
	data = fread16( adr );
	if ( data != START ) {
		return FERR_NO_START; // no START tag
	}
	adr += 2;
	data = fread16( adr );
	if ( data != ID ) {
		return FERR_MISMATCH_ID; // the record's ID doesn't match the address
	}
	FLASH_Unlock();
	FLASH_Status fstat = FLASH_ProgramHalfWord( adr, 0 );
	FLASH_Lock();
	if ( fstat != FLASH_COMPLETE ) {
		return FERR_REMOVE_ID; // error while remove the old record
	}
	return FRES_OK;
}

/*
 * Erase page flash
 * Return:
 *   code error
 */
int ferasePage( uint32_t adr )
{
	uint32_t adrPage = 0;

	if ( adr < BLOCK_B ) {
		for ( int i = 0; i < BLOCK_SIZE / PAGE_SIZE; i++ ) {
			if ( adr == BLOCK_A + i * PAGE_SIZE ) {
				adrPage = adr;
			}
		}
	} else {
		for ( int i = 0; i < BLOCK_SIZE / PAGE_SIZE; i++ ) {
			if ( adr == BLOCK_B + i * PAGE_SIZE ) {
				adrPage = adr;
			}
		}
	}
	if ( adrPage == 0 ) {
		urtPrint("Err: ferasePage: adr not correct\n");
		return FERR_ADR_UNCORRECT; // address is not correct
	}
//	if ( (adr != PAGE60) && (adr != PAGE61) && (adr != PAGE62) && (adr != PAGE63) )
//	{
//		urtPrint("Err: ferasePage: adr not correct\n");
//		return 1;
//	}
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
 *   code error
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
 * Find current block. One block must be current, another empty.
 *   *curBlock - pointer, where will be writes current block's address
 *   *empBlock - pointer, where will be writes empty block's address
 * Return:
 *   code error
 */
int fFindCurrBlock( uint32_t *curBlock, uint32_t *empBlock )
{
	uint16_t blockStat;
	blockStat = fread16( BLOCK_A );
	if ( blockStat == BLOCK_CURR ) {
		*curBlock = BLOCK_A;
		blockStat = fread16( BLOCK_B );
		if ( blockStat == BLOCK_EMPTY ) {
			*empBlock = BLOCK_B;
			return FRES_OK;
		}
		return FERR_EMP_NFOUND; // curBlock found but embBlock not found
	}
	blockStat = fread16( BLOCK_B );
	if ( blockStat == BLOCK_CURR ) {
		*curBlock = BLOCK_B;
		blockStat = fread16( BLOCK_A );
		if ( blockStat == BLOCK_EMPTY ) {
			*empBlock = BLOCK_A;
			return FRES_OK;
		}
		return FERR_EMP_NFOUND; // curBlock found but embBlock not found
	}
	urtPrint("Err:fFindCurrBlock: no cur emp block\n");
	return FERR_NO_CURR_EMP; // non cubBlock and non empBlock
}

/*
 * Copies record at address adrRec to flashBuf[]
 *   Record has to be correct (about START, ID, LEN)
 * Return:
 *   code error
 */
int fcopyRecToBuf( uint32_t adrRec )
{
	uint16_t len;
	int res;

	res = fgetAndCheckLen( adrRec, &len );
	if ( res != FRES_OK ) {
		urtPrint("Err: fcopyRecToBuf len false\n");
		return res;
	}
	for ( int a = 0; a < len/2; a++ ) {
		flashBuf[a] = fread16( adrRec + (a<<1) );
	}
	return FRES_OK;
}

/*
 * Writes the contents of the flashBuf[] buffer to the specified address
 * Return:
 *   code error
 */
int fwriteRecFromBuf( uint32_t adr )
{
	FLASH_Status fstat;
	uint16_t len;
	if ( (flashBuf[0] != START) || (flashBuf[1] == 0) ) {
		return FERR_MISS_ST_ID; // miss START or Id in flashBuf[]
	}
	len = flashBuf[2];
	if ( (len < MIN_LEN) || (len > MAX_LEN) ) {
		return FERR_REC_LEN; //  record's length is not correct
	}
	FLASH_Unlock();
	for ( int i = 0; i < len/2; i++ ) {
		fstat = FLASH_ProgramHalfWord( adr + (i<<1), flashBuf[i] );
		if ( fstat != FLASH_COMPLETE ) {
			FLASH_Lock();
			return FERR_WRITE_HW; // error while write half word data to flash
		}
	}
	FLASH_Lock();
	return FRES_OK;
}

/*
 * To copy all active records from overflow block to empty block.
 *   adrCur - points to current block
 *   adrEmp - points to empty block
 * Return:
 *   code error
 */
int fmoveBlock( uint32_t adrCur, uint32_t adrEmp )
{
	int res;
	uint16_t head;
	if ( fread16( adrCur ) != BLOCK_SHIFT ) {
		return FERR_NOT_SHIFT; // block is not in Shift state
	}
	if ( fread16( adrEmp ) != BLOCK_EMPTY ) {
		return FERR_NOT_EMPTY; // block is not in Empty state
	}
	adrCur += 2;
	adrEmp += 2;
	while ( 1 ) {
		head = fread16( adrCur );
		if ( head == 0xFFFF ) { // if empty cell
			return FRES_OK;
		}
		if ( head != START ) { // if no START tag
			// looking for END tag
			urtPrint("fmoveBlock: no START\n");
			for ( int i = 0; i < MAX_LEN + 10; i++ ) {
				adrCur++;
				head = fread16( adrCur );
				if ( head == END ) {
					adrCur++;
					urtPrint("fmoveBlock: find END\n");
					continue;
				}
			}
			return FERR_NO_START;
		}
		if ( fgetId( adrCur ) == 0 ) { // if record is nulled
			adrCur += fgetLen( adrCur );
			continue;
		}
		// here *adrCur points to record which need to copy to *adrEmp
		res = fcopyRecToBuf( adrCur );
		if ( res != 0 ) { // write rec to buf
			return res;
		}
		res = fwriteRecFromBuf( adrEmp );
		if ( res != 0 ) { // write buf to flash
			return res;
		}
		adrEmp += fgetLen( adrEmp );
		adrCur += fgetLen( adrCur );
		continue;
	}
	return FERR_UNREACABLE; // unreachable
}

/*
 * Changes the block from current to empty.
 * It is need when curren block overflow.
 * Params:
 *   *newCurBlock - pointer, where to write address of new current block
 * Switch to new block walk thrue next steps:
 *   - curren block status changes to BLOCK_SHIFT (0x0000)
 *   - copies all active records to empty block
 *   - change empty block status to current
 *   - erase old block (that was BLOCK_SHIFT)
 * Return:
 *   code error
 */
int fchangeBlock( uint32_t *newCurBlock)
{
	int res;
	uint32_t adrCur;
	uint32_t adrEmp;
	res = fFindCurrBlock( &adrCur, &adrEmp );
	if ( res != FRES_OK ) {
		urtPrint("Err: fchangeBank: can't find cur block\n");
		return res; // handle this case
	}
	// curren block status changes to BLOCK_SHIFT (0x0000)
	res = fwriteHalfWord( adrCur, BLOCK_SHIFT );
	if ( res != FRES_OK ) {
		urtPrint("Err: fchangeBank: can't BLOCK_SHIFT\n");
		return res;
	}
	// copies all active records to empty block
	res = fmoveBlock( adrCur, adrEmp );
	if ( res != FRES_OK ) {
		urtPrint("Err: in fchangeBank while fmoveBlock \n");
		return res;
	}
	// change empty block status to current
	res = fwriteHalfWord( adrEmp, BLOCK_CURR );
	if ( res != FRES_OK ) {
		urtPrint("Err: fchangeBank: can't make BLOCK_CURR\n");
		return res;
	}
	// erase old block (that was BLOCK_SHIFT)
	res = feraseBlock ( adrCur );
	if ( res != FRES_OK ) {
		urtPrint("Err: fchangeBank: can't erase old block\n");
		return res;
	}
	*newCurBlock = adrEmp;
	return FRES_OK;
}

/*
 * Calculates a free space at the current block.
 *   *free     - pointer where to store result
 *   *adrBlock - pointer where to store address of current block
 * Return:
 *   code error, if 0 then *free and *adrBlock has results
 */
int fgetFree( uint16_t *free, uint32_t *adrBlock )
{
	int res;
	uint32_t adr;
	uint32_t doomy;

	*free = 0;

	res = fFindCurrBlock( adrBlock, &doomy );
	if ( res != 0 ) {
		urtPrint("Err: fgetFree: not found cur Block\n");
		return res;
	}
	adr = *adrBlock;

	doomy = adr + BLOCK_SIZE;
	while ( 1 ) {
		res = fFindIDaddr( 0xFFFF, &adr, doomy );
		if ( res == FERR_FND_EMP_CELL ) { // if finded empty cell
			break;
		} else if ( res == FRES_OK ) {
			continue; // we do not look for this ID
		} else {
			return res; // error code
		}
	}
	*free = (doomy - 2) - adr; // at end must be always empty hw
	return FRES_OK;
}

/*
 * when curren block is not found, it is need to call
 * this function, in which will create current block
 *
 * Если перед операцией чтения или записи текущий блок не найден, то необходимо
 *   его создать. Для этого проверяются текуще блоки.
 *   - помечен ли какой либо блок как в процессе переключения
 *   - если да продолжим процедуру переключения
 *   - если блоков в процессе переключения тоже нет, выбираем младший по адресу блок
 *     и помечаем его текущим.
 */
int func( void )
{
	uint32_t  adrCur;
	uint32_t  adrEmp;
	int res = 0;
	uint16_t blockStat;
	blockStat = fread16( BLOCK_A );
	if ( blockStat == BLOCK_EMPTY ) {

		return 0; // curBlock found but embBlock not found
	}
	blockStat = fread16( BLOCK_B );
	if ( blockStat == BLOCK_EMPTY ) {

	}
	return res;
}

// =======================================================================================
// ============================== interface functions ====================================

/*
 * Saves the record with ID to the flash, where:
 *  *buf - points to the 16 bits data with lenght len bytes.
 *   len - must be even number (because can write only 16 bits)
 * Return:
 *   code error
 */
int fsaveRecord( const uint16_t ID, const uint16_t* const buf, const uint16_t len )
{
	int res;
	uint32_t adr;
	uint32_t end;
	uint16_t free;

	if ( (len % 2) != FRES_OK) {
		return FERR_LEN_NOT_EVEN; // len is not even
	}

	res = fgetFree( &free, &adr );
	if ( res != FRES_OK ) {
		// ToDo: implement case when cur block not found
		return res;
	}
	uint16_t LEN = len + 8; // length of the entire record
	if ( free < LEN ) {
		res = fchangeBlock( &adr );
		if ( res != FRES_OK ) {
			return res;
		}
	}

	end = adr + BLOCK_SIZE;
	res = fFindIDaddr( ID, &adr, end );
	if ( res == FRES_OK ) { // if old record with same ID exist
		res = fdeleteID( ID, adr ); // delete old record
		if ( res != FRES_OK ) {
			return res;
		}
		res = fFindIDaddr( ID, &adr, end ); // look for again
	}
	if ( res != FERR_FND_EMP_CELL ) { // if not an empty cell
		return res;
	}
	// now adr is points to the empty cell

	// write START attribute
	res = fwriteHalfWord( adr, START );
	if ( res != FRES_OK ) {
		return res;
	}
	// write ID
	res = fwriteHalfWord( adr + 2, ID );
	if ( res != FRES_OK ) {
		return res;
	}
	// write LEN
	res = fwriteHalfWord( adr + 4, LEN );
	if ( res != FRES_OK ) {
		return res;
	}
	// write data
	for ( uint16_t i = 0 ; i < len/2; i++ ) {
		res = fwriteHalfWord( adr + 6 + i*2, *(buf + i) );
		if ( res != FRES_OK ) {
			return res;
		}
	}
	// calculate and write XOR
	uint16_t vxor = fcalcXOR( buf, len );
	res = fwriteHalfWord( adr + 6 + len, vxor );
	if ( res != FRES_OK ) {
		return res;
	}
	return FRES_OK;
}

/*
 * Loads data with the specified ID, where:
 *  *buf    - points to the 16 bits data
 *   maxLen - max volume of the buffer buf, in bytes
 *   rlen   - The number of bytes read (alwayes even)
 * Return:
 *   code error
 */
int floadRecord( const uint16_t ID, uint16_t* buf, const uint16_t maxLen, uint16_t *rlen )
{
	int res;
	uint16_t dataLen;
	uint32_t adr;
	uint32_t doomy;

	res = fFindCurrBlock( &adr, &doomy );
	if ( res != FRES_OK ) {
		return res;
	}
	doomy = adr + BLOCK_SIZE; // end block
	adr += 2;
	// now adr points to begin of first record
	res = fFindIDaddr( ID, &adr, doomy );
	if ( res != FRES_OK ) {
		return res;
	}
	dataLen = fgetLen( adr );
	dataLen -= 8;
	if ( dataLen > maxLen ) {
		return FERR_BUF_LITTLE; // buff is too little for record
	}
	adr += 6; // now adr points to the data

	// read data and write its to buf
	for ( uint16_t i = 0 ; i < dataLen/2; i++ ) {
		buf[i] = fread16( adr );
		adr += 2;
	}
	*rlen = dataLen;
	uint16_t xor = fcalcXOR( buf, dataLen );
	if ( xor != fread16( adr ) ) {
		return 21; // buf written, but xor is false
	}
	return FRES_OK;
}

