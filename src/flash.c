/*
 * flash.c
 *
 *  Created on: 12 марта 2017 г.
 *      Author: se
 *
 *  Модуль для работы с флэш памятью
 *  Реализует алгоритм записи, чтения данных, а также стирает страницу
 *  при ее заполнении.
 *  Запись и чтение осуществляется пакетами, заданной длины.
 *
 *====================================================================================
 * Алгоритм записи и чтения информации во флэш.
 *
 *   Минимальная операция записи контроллером - 2 байта (16 бит), записать можно
 * только в стертую ячеку (0xFFFF - проверяется флэш контроллером),
 * исключение - если записываемые данные имеют все биты нули (0х0000).
 *   Адрес записи должен быть выровнен по четным адресам.
 * Стирать можно только страницу целиком (1 кБ на данном контроллере), посте стирания
 * все данные страницы переведутся в 0xFF.
 *
 *   В качестве формата пакета данных (record) во флэше примем следующий вид:
 * START, ID, LEN, DATA[], XOR
 * ,где
 *     START  - признак старта записи, 16 бит: 0xABCD (чтобы распознать ошибку)
 *     ID     - идентификатор записи, 16 бит, если 0 - то запись стерта
 *     LEN    - длина всей записи, начиная от START и заканчивая XOR, в байтах
 *     DATA[] - сами данные, по 16 бит, имеют длину = LEN - 8 байт
 *     XOR    - контрольная сумма DATA[] данных
 *
 * - Запись данных во флэше должна быть уникальной, то есть записи с
 * одинаковыми ID повторятся не должны.
 * - Если запись с определенным ID нужно удалить не стирая всю страницу, то вместо
 * поля ID записываются нули, остальные поля записи не трогаются.
 * - Поиск записи в блоке по ID происходит по алгоримту: первый байт всегда должен быть
 * стартовым, далее смотрим ID, если ID не наш, увеличиваем адрес на LEN байт и
 * итерация повторяется пока не найдем нужный ID или пока не наткнемся на пустую
 * ячейку или пока не закончится страница памяти.
 * - Если нужно записать новые данные с определенным ID, то сначала удаляем старую
 * запись с данным ID (если она есть), находим первую чистую ячейку (0xFFFF) и если
 * хватает места для записи - записываем.
 *
 *====================================================================================
 *   В качестве блока будем использовать 2 смежные страницы памяти - для увеличения
 * объема хранилища.
 * В блоке будем различать:
 *   - свободное пространство : после стирания (имеет 0xFFFF полуслова);
 *   - рабочее пространство   : записанная полезная информация (запись);
 *   - удаленное пространство : запись с обнуленным ID
 *
 *   Будем использовать 2 блока. Один из них будем держать свободным (после стирания)
 * другой блок текущим. Назовем блоки A и B.
 *   Переключение с блока A на блок B:
 * Как только блок А заполнится (т.е. свободного пространства окажется не достаточным
 * для новой записи), копируем из блока А рабочие записи в блок B.
 * После этого блок A стираем, а запись новой информации будем вести в блоке B.
 * Переключение с блока B на блок A - аналогично.
 *
 *   Число записей с различными ID должно быть ограничено, иначе после переключения
 * с одного блока на другой, свободного пространства может стать вновь не достаточным.
 *
 *   Под начало блока забираем 2 байта - статус блока, в которой будет хранится
 * следующая информация:
 *   - 0xFFFF : пустой блок
 *   - 0xBBBB : текущий блок
 *   - 0x0000 : в процессе переключения с данного блока на пустой блок
 *
 * Процесс переключения блока происходит следующим образом:
 *   - в статус блока записываются 0x0000
 *   - другой блок должен быть пустым
 *   - копируется первая запись в новый блок
 *   - проверяется запись в новом блоке
 *   - удаляется запись в старом блоке
 *   - после копирования всех записей, новый блок помечается как текущий а старый стираем
 *
 *   Если перед операцией чтения или записи текущий блок не найден, то необходимо
 * его создать. Для этого проверяются текуще блоки.
 *   - помечен ли какой либо блок как в процессе переключения
 *   - если да продолжим процедуру переключения
 *   - если блоков в процессе переключения тоже нет, выбираем младший по адресу блок
 *     и помечаем его текущим.
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
 *====================================================================================
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

static const uint16_t BLOCK_EMPTY = 0xFFFF; // пустой блок
static const uint16_t BLOCK_CURR  = 0xBBBB; // текущий блок
static const uint16_t BLOCK_SHIFT = 0x0000; // в процессе переключения

static const uint16_t START = 0xABCD; // признак старта записи

#define MIN_LEN   10   // минимальная длина записи (данные + 8 служ. байт)
#define MAX_LEN   200  // максимальная длина записи, в байтах

static uint16_t flashBuf[MAX_LEN << 2];

/*
 * расчет контрольной суммы
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
 * Чтение 2х байтного значения с флэш по заданому адресу
 */
inline uint16_t fread16(uint32_t Address)
{
	return *(__IO uint16_t*)Address;
}

/*
 * Возвращает ID записи согласно начальному адресу
 */
inline uint16_t fgetId( uint32_t startAdr )
{
	return fread16( startAdr + 2 );
}

/*
 * Возвращает длину записи согласно начальному адресу
 */
inline uint16_t fgetLen( uint32_t startAdr )
{
	return fread16( startAdr + 4 );
}

/*
 * Пишет полуслово - hw во флэш по адресу - adr
 * Return:
 *   0 - ok
 *   1 - error
 */
int fwriteHalfWord( uint32_t adr, uint16_t hw )
{
	FLASH_Status fstat;
	FLASH_Unlock();
	fstat = FLASH_ProgramHalfWord( adr, hw ); // в статус блока записываются 0x0000
	FLASH_Lock();
	if ( fstat != FLASH_COMPLETE ) {
		return 1;
	}
	return 0;
}

/*
 * Производит поиск записи во флэш с заданным ID
 * Params:
 *   ID - номер ID которую хотим найти
 *   *padr - указатель, куда сохранится адрес записи
 * Return:
 *   0 - ID найден, *addr указывает на старт записи
 *   1 - не нашли ID, дошли до пустой ячейки, paddr на пустую ячейку
 *   2 - нет старта данных, ошибка
 *   3 - длина записи меньше минимально допустимой
 *   4 - длина записи больше максимально допустимой
 *   5 - out of block range
 */
int fFindIDaddr( uint16_t ID, uint32_t *padr, uint32_t endAdr )
{
	uint16_t data;
	uint16_t len;
	uint16_t id;
	//*padr = BLOCK_A; //PAGE60;
	do {
		data = fread16( *padr );
		if ( data == START ) {
			id = fgetId( *padr ); // смотрим ID
			if ( ID == id ) {
				urtPrint("Find ID: ");
				urt_uint32_to_str(ID);
				urtPrint(", at adr: ");
				urt_uint32_to_hex( *padr );
				urtPrint("\n");
				return 0; // нашли!
			} else {
				// смотрим LEN
				len = fgetLen( *padr );
				if ( len < MIN_LEN ) {
					return 3; // длина записи меньше минимально допустимой
				} else if ( len + 8 > MAX_LEN ) {
					return 4; // длина записи больше максимально допустимой
				} else {
					*padr += len; // перейдем на следующую запись
					if ( *padr >= endAdr ) {
						return 5; // out of block range
					}
				}
			}
		} else if ( data == 0xFFFF ) {
			return 1; // дошли до пустой ячейки
		} else {
			return 2; // нет старта данных
		}
	} while ( 1 );
	return -1; // не достижимая инструкция
}

/*
 * Удаляет запись под данным адресом
 *   Return:
 *    0 - Ok
 *    1 - нет старта записи
 *    2 - ID записи не соответствует адресу
 *    3 - ошибка удаления записи
 */
int fdeleteID( uint16_t ID, uint32_t adr ) {
	uint16_t data;
	data = fread16( adr );
	if ( data != START ) {
		return 1;
	}
	adr += 2;
	data = fread16( adr );
	if ( data != ID ) {
		return 2;
	}
	FLASH_Unlock();
	FLASH_Status fstat = FLASH_ProgramHalfWord( adr, 0 );
	FLASH_Lock();
	if ( fstat != FLASH_COMPLETE ) {
		return 3;
	}
	return 0;
}

/*
 * Стирает флэш страницу
 *   Return:
 *     0 - Ok
 *     1 - не правильный адрес
 *     2 - ошибка стирания страницы
 */
int ferasePage( uint32_t adr )
{
	if ( (adr != PAGE60) && (adr != PAGE61) && (adr != PAGE62) && (adr != PAGE63) )
	{
		urtPrint("Err: ferasePage: adr not correct\n");
		return 1;
	}
	FLASH_Unlock();
	FLASH_Status fstat =  FLASH_ErasePage( adr );
	FLASH_Lock();
	if ( fstat != FLASH_COMPLETE ) {
		urtPrint("Err: ferasePage: cant erase\n");
		return 2;
	}
	return 0;
}

/*
 * Стирает блок
 *   Return:
 *     0 - Ok
 *     1 - не правильный адрес
 *     2 - ошибка стирания страницы
 */
int feraseBlock ( uint32_t block )
{
	int ret = 0;
	uint32_t page = block;
	for ( int i = 0; i < BLOCK_SIZE / PAGE_SIZE; i++ ) {
		ret = ferasePage( page + i * PAGE_SIZE );
		if ( ret != 0) {
			break;
		}
	}
	return ret;
}

/*
 * Найти текущий блок, один блок должен быть текущим
 *   а другой блок должен быть пустым.
 *   *curBlock - куда запишется адрес текущего блока
 *   *empBlock - куда запишется адрес пустого блока
 * Return:
 *   0 - ok
 *   1 - Error: curBlock found but embBlock not found
 *   2 - Error: non cubBlock and non empBlock
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
			return 0;
		}
		return 1;
	}
	blockStat = fread16( BLOCK_B );
	if ( blockStat == BLOCK_CURR ) {
		*curBlock = BLOCK_B;
		blockStat = fread16( BLOCK_A );
		if ( blockStat == BLOCK_EMPTY ) {
			*empBlock = BLOCK_A;
			return 0;
		}
		return 1;
	}
	urtPrint("Err:fFindCurrBlock: no cur emp block\n");
	return 2;
}

/*
 * Копирует запись adrRec в буфер flashBuf
 *   запись должна быть уже проверенной на
 *   наличие START ID LEN
 * Return:
 *   0 - ok
 *   1 - error: record's length is not correct
 */
int fcopyRecToBuf( uint32_t adrRec )
{
	uint16_t len;
	len = fgetLen( adrRec );
	if ( (len < MIN_LEN) || (len > MAX_LEN) ) {
		urtPrint("Err: fcopyRecToBuf len false\n");
		return 1;
	}
	for ( int a = 0; a < len; a += 2 ) {
		flashBuf[a] = fread16( adrRec + a );
	}
	return 0;
}

/*
 * Производит запись содержимого буфера flashBuf
 *   по указанному адресу
 * Return:
 *   0 - ok
 *   1 - Error start or Id in flashBuf
 *   2 - Error lenght in flashBuf
 *   3 - Error while flash write half word
 */
int fwriteRecFromBuf( uint32_t adr )
{
	FLASH_Status fstat;
	uint16_t len;
	if ( (flashBuf[0] != START) || (flashBuf[1] == 0) ) {
		return 1;
	}
	len = flashBuf[2];
	if ( (len < MIN_LEN) || (len > MAX_LEN) ) {
		return 2;
	}
	FLASH_Unlock();
	for ( int i = 0; i < len; i++ ) {
		fstat = FLASH_ProgramHalfWord( adr + (i<<1), flashBuf[i] );
		if ( fstat != FLASH_COMPLETE ) {
			FLASH_Lock();
			return 3;
		}
	}
	FLASH_Lock();
	return 0;
}

/*
 * Копирование активных записей начная с адреса adrCur
 *   в пустое пространство с адреса adrEmp
 * Return:
 *   0 - ok
 *   1,2 - error
 */
int fmoveBlock( uint32_t adrCur, uint32_t adrEmp )
{
	uint16_t head;
	while ( 1 ) {
		head = fread16( adrCur );
		if ( head == 0xFFFF ) { // если дошли до пустой ячейки
			return 0;
		}
		if ( head != START ) { // если какой то левый байт
			return 1; // нужно ли как то обрабатывать?
		}
		if ( fgetId( adrCur ) == 0 ) { // если запись аннулированная
			adrCur += fgetLen( adrCur );
			continue;
		}
		// здесь *adrCur указывает на запись которую нужно скопировать в *adrEmp
		fcopyRecToBuf( adrCur ); // скопируем запись *adrCur в буфер
		if ( fwriteRecFromBuf( adrEmp ) != 0 ) { // запишем содержимое буфера в новый блок
			return 2;
		}
		adrEmp += fgetLen( adrEmp );
		adrCur += fgetLen( adrCur );
		continue;
	}
	return 0;
}

/*
 * Процесс переключения блока происходит следующим образом:
 *   - в статус блока записываются 0x0000
 *   - другой блок должен быть пустым
 *   - копируются все записи в новый блок
 *   - после копирования всех записей, новый блок помечается как текущий а старый стираем
 * Return:
 *   0 - ok
 *   1 - can't find current block
 *   2 - error flash write
 *   3 - can't errase block
 */
int fchangeBank(void)
{
	uint32_t adrCur; // будет указывать на адрес текущего блока
	uint32_t adrEmp; // будет указывать на адрес пустого блока
	if ( fFindCurrBlock( &adrCur, &adrEmp ) != 0 ) {
		urtPrint("Err: fchangeBank: can't find cur block\n");
		return 1; // handle this case
	}
	// текущий блок пометим как в режиме переключения
	if ( fwriteHalfWord( adrCur, BLOCK_SHIFT ) != 0 ) {
		urtPrint("Err: fchangeBank: can't BLOCK_SHIFT\n");
		return 2;
	}
	// скопируем активные записи в пустой блок
	if ( fmoveBlock( adrCur + 2, adrEmp + 2 ) != 0 ) {
		urtPrint("Err: in fchangeBank while fmoveBlock \n");
	}
	// пометим новый блок как текущий
	if ( fwriteHalfWord( adrEmp, BLOCK_CURR ) != 0 ) {
		urtPrint("Err: fchangeBank: can't make BLOCK_CURR\n");
		return 2;
	}
	// сотрем старый текущий блок который находится в режиме переключенияb
	if ( feraseBlock ( adrCur ) != 0 ) {
		urtPrint("Err: fchangeBank: can't erase old block\n");
		return 3;
	}
	return 0;
}

// =======================================================================================
// ======================== interface functions ==========================================

/*
 * Saves the record with ID to the flash
 * , where
 *  *buf - points to the 16 bits data with lenght len bytes.
 *   len - must be even number (because can write only 16 bits)
 * Return:
 *   0 - Ok
 *   1 - error delete of an old record
 *   2 - don't find the empty cell or other mistake
 *   3 - not enough free space for new record
 *   4 - error while writing to flash
 *   5 - len is not an even number
 *   6 - current block is not found
 */
int fsaveRecord( const uint16_t ID, const uint16_t* const buf, const uint16_t len )
{
	int res;
	uint32_t adr;
	uint32_t doomy;

	res = fFindCurrBlock( &adr, &doomy );
	if ( res != 0 ) {
		//urtPrint("Err: fsaveRecord: not found cur Block\n");
		return 6;
	}
	doomy = adr + BLOCK_SIZE;

	if ( len%2 != 0) {
		return 5; // len is not even
	}

	res = fFindIDaddr( ID, &adr, doomy );
	if ( res == 0 ) { // if old record with same ID exist
		int r;
		r = fdeleteID( ID, adr ); // delete old record
		if ( r != 0 ) {
			return 1;
		}
		res = fFindIDaddr( ID, &adr, doomy ); // look for again
	}
	if ( res != 1 ) {
		return 2;
	}

	// and now adr is points to the empty cell
	// to calculate a volume of the free memory
	uint32_t free = (doomy - 2) - adr; // at end must be always empty hw
	uint16_t LEN = len + 8; // length of the entire record
	if ( free < LEN ) {
		return 3;
	}

	// write START attribute
	if ( fwriteHalfWord( adr, START ) != 0 ) {
		return 4;
	}
	// write ID
	if ( fwriteHalfWord( adr + 2, ID ) != 0 ) {
		return 4;
	}
	// write LEN
	if ( fwriteHalfWord( adr + 4, LEN ) != 0 ) {
		return 4;
	}
	// write data
	for ( uint16_t i = 0 ; i < len/2; i++ ) {
		if ( fwriteHalfWord( adr + 6 + i*2, *(buf + i) ) != 0 ) {
			return 4;
		}
	}
	// calculate and write XOR
	uint16_t vxor = fcalcXOR( buf, len );
	if ( fwriteHalfWord( adr + 6 + len, vxor ) != 0 ) {
		return 4;
	}
	return 0;
}

/*
 * Loads data with the specified ID, where:
 *  *buf    - points to the 16 bits data
 *   maxLen - max volume of the buffer buf, in bytes
 *   rlen   - The number of bytes read (alwayes even)
 * Return:
 *   0 - Ok, data was written to flash
 *
 *   1 - не нашли ID, дошли до пустой ячейки, paddr на пустую ячейку
 *   2 - нет старта данных, ошибка
 *   3 - длина записи меньше минимально допустимой
 *   4 - длина записи больше максимально допустимой
 *   5 - out of block range
 *   6 - current block is not found
 *
 *   11 - объем буфер buf не достаточен для записи
 *   12 - данные в buf записали, но xor не совпадает
 */
int floadRecord( const uint16_t ID, uint16_t* buf, const uint16_t maxLen, uint16_t *rlen )
{
	int res;
	uint16_t dataLen;
	uint32_t adr;
	uint32_t doomy;

	res = fFindCurrBlock( &adr, &doomy );
	if ( res != 0 ) {
		return 6;
	}
	doomy = adr + BLOCK_SIZE;
	adr += 2;

	res = fFindIDaddr( ID, &adr, doomy );
	if ( res == 0 ) {
		// нашли запись
		dataLen = fgetLen( adr );
		dataLen -= 8;
		if ( dataLen > maxLen ) {
			return 11; // объем буфер buf не достаточен для записи
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
			return 12;
		}
		return 0;
	}
	return res;
}

