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
#define PAGE_SIZE  1024

static const uint32_t BLOCK_A     = PAGE60;
static const uint32_t BLOCK_B     = PAGE62;
static const uint32_t BLOCK_SIZE  = PAGE_SIZE * 2;

static const uint16_t BLOCK_EMPTY = 0xFFFF; // пустой блок
static const uint16_t BLOCK_CURR  = 0xBBBB; // текущий блок
static const uint16_t BLOCK_SHIFT = 0x0000; // в процессе переключения

static const uint16_t START = 0xABCD; // признак старта записи
static const uint16_t MIN_LEN = 10;   // минимальная длина записи (данные + 8 служ. байт)
static const uint16_t MAX_LEN = 200;  // максимальная длина записи

typedef struct {
	uint8_t   page; // номер текущего блока: 0 или 1
	uint8_t   pageState; // состояние блока:
	uint16_t  freeBytes; // свободное число байт для записей в блоке
	uint16_t  usedBytes; // рабочее число байт в блоке
	uint16_t  zeroBytes; // число байт в удаленных записях
	uint16_t  records;   // общее число записей
} finfo_t;


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
 *   5 - выход за пределы страницы
 */
int fFindIDaddr( uint16_t ID, uint32_t *padr )
{
	uint16_t data;
	uint16_t len;
	uint16_t id;
	*padr = BLOCK_A; //PAGE60;
	do {
		data = fread16( *padr );
		if ( data == START ) {
			//id = fread16( *padr + 2 ); // смотрим ID
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
				//len = fread16( *padr + 4 );
				len = fgetLen( *padr );
				if ( len < MIN_LEN ) {
					return 3; // длина записи меньше минимально допустимой
				} else if ( len + 8 > MAX_LEN ) {
					return 4; // длина записи больше максимально допустимой
				} else {
					*padr += len; // перейдем на следующую запись
					if ( (*padr - BLOCK_A) > BLOCK_SIZE ) {
						return 5; // выход за пределы блока
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
		urtPrint("Err: ferasePage: 1\n");
		return 1;
	}
	FLASH_Unlock();
	FLASH_Status fstat =  FLASH_ErasePage( adr );
	FLASH_Lock();
	if ( fstat != FLASH_COMPLETE ) {
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
	return 1;
}

/*
 * Сохраняет запись с уникальным ID на флэш
 * , где
 *  *buf - указатель на 16 битные данные длиной len байт.
 *   len - должно быть четным числом (т.к. данные 16 битные)
 * Return:
 *   0 - Ok
 *   1 - ошибка удаления старой записи
 *   2 - не дошли до пустой ячейки или прочая ошибка
 *   3 - не достаточно свободного места для новой записи
 *   4 - ошибка во время записи
 *   5 - len не является четным числом
 */
int fsaveRecord( const uint16_t ID, const uint16_t* const buf, const uint16_t len )
{
	int res;
	FLASH_Status fstat;
	uint32_t adr = BLOCK_A; //PAGE60;

	if ( len%2 != 0) {
		return 5; // len is not even
	}

	res = fFindIDaddr( ID, &adr );
	if ( res == 0 ) { // если запись уже существует
		int r;
		r = fdeleteID( ID, adr ); // удалим старую запись
		if ( r != 0 ) {
			return 1;
		}
		res = fFindIDaddr( ID, &adr ); // снова ищем
	}
	if ( res != 1 ) {
		return 2;
	}
	// теперь adr указывает на пустую ячейку
	// определим объем оставшейся памяти
	//uint32_t free = PAGE61 - adr;
	uint32_t free = BLOCK_B - adr;
	uint16_t LEN = len + 8; // длина всей записи
	if ( free < LEN ) {
		return 3;
	}

	FLASH_Unlock();

	// запишем старт признак записи
	fstat = FLASH_ProgramHalfWord( adr, START );
	if ( fstat != FLASH_COMPLETE ) {
		FLASH_Lock();
		return 4;
	}

	// запишем ID записи
	fstat = FLASH_ProgramHalfWord( adr + 2, ID );
	if ( fstat != FLASH_COMPLETE ) {
		FLASH_Lock();
		return 4;
	}

	// запишем LEN - длину всей записи
	fstat = FLASH_ProgramHalfWord( adr + 4, LEN );
	if ( fstat != FLASH_COMPLETE ) {
		FLASH_ProgramHalfWord( adr + 2, 0 ); // стерём ID
		FLASH_Lock();
		return 4;
	}

	// запишем данные
	const uint16_t *data = buf; // чтобы не инкрементировать buf
	for ( uint16_t i = 0 ; i < len; i += 2 ) {
		fstat = FLASH_ProgramHalfWord( adr + 6 + i, *data );
		if ( fstat != FLASH_COMPLETE ) {
			FLASH_ProgramHalfWord( adr + 2, 0 ); // стерём ID
			FLASH_Lock();
			return 4;
		}
		data++;
	}

	// запишем XOR
	uint16_t vxor = fcalcXOR( buf, len );
	fstat = FLASH_ProgramHalfWord( adr + 6 + len, vxor );
	if ( fstat != FLASH_COMPLETE ) {
		FLASH_ProgramHalfWord( adr + 2, 0 ); // стерём ID
		FLASH_Lock();
		return 4;
	}

	FLASH_Lock();
	return 0;
}

/*
 * Загружает данные с указанным ID, где
 *  *buf    - указатель на 16 битные данные
 *   maxLen - максимальный объем буфера buf в байтах
 *   rlen    - считанное количество байт (всегда четное)
 * Return:
 *   0 - Ok, данные в buf записались
 *
 *   1 - не нашли ID, дошли до пустой ячейки, paddr на пустую ячейку
 *   2 - нет старта данных, ошибка
 *   3 - длина записи меньше минимально допустимой
 *   4 - длина записи больше максимально допустимой
 *   5 - выход за пределы страницы
 *
 *   11 - объем буфер buf не достаточен для записи
 *   12 - данные в buf записали, но xor не совпадает
 */
int floadRecord( const uint16_t ID, uint16_t* buf, const uint16_t maxLen, uint16_t *rlen )
{
	int res;
	uint32_t adr = BLOCK_A; //PAGE60;
	uint16_t dataLen;

	res = fFindIDaddr( ID, &adr );
	if ( res == 0 ) {
		// нашли запись
		dataLen = fread16( adr + 4 );
		dataLen -= 8;
		if ( dataLen > maxLen ) {
			return 11; // объем буфер buf не достаточен для записи
		}
		adr += 6; // теперь адрес указывает на данные

		// прочитаем данные и запишем их в буфер
		for ( uint16_t i = 0 ; i < dataLen; i++ ) {
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

/*
 * Найти текущий блок, один блок должен быть текущим
 *   а другой блок должен быть пустым.
 *   *curBlock - куда запишется адрес текущего блока
 *   *empBlock - куда запишется адрес пустого блока
 * Return:
 *   0 - ok
 *   1 - not found
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
	return 1;
}

/*
 * Копирование записи
 * Return:
 *   0 - ok
 *   1 - no record at adrCur
 *   2 - error
 */
int fmoveBlock( uint32_t *adrCur, uint32_t *adrEmp )
{
	uint16_t data;
	while ( 1 ) {
		data = fread16( *adrCur );
		if ( data == 0xFFFF ) {
			return 1;
		}
		if ( data != START ) {
			return 2;
		}
		data = fgetId( *adrCur );
		if ( data == 0 ) {
			data = fgetLen( *adrCur );
			*adrCur += data;
			continue;
		}
		// copy
	}
	return 0;
}

/*
 * Процесс переключения блока происходит следующим образом:
 *   - в статус блока записываются 0x0000
 *   - другой блок должен быть пустым
 *   - копируется первая запись в новый блок
 *   - проверяется запись в новом блоке
 *   - удаляется запись в старом блоке
 *   - после копирования всех записей, новый блок помечается как текущий а старый стираем
 */
void fchangeBank(void)
{
	FLASH_Status fstat;
	uint32_t adrCur, adrEmp;
	if ( fFindCurrBlock( &adrCur, &adrEmp ) != 0 ) {
		urtPrint("Err: can't find cur block\n");
		return; // ToDo to handle this case
	}
	FLASH_Unlock();
	fstat = FLASH_ProgramHalfWord( adrCur, BLOCK_SHIFT ); // в статус блока записываются 0x0000
	FLASH_Lock();
	adrCur += 2;
	adrEmp += 2;
	fmoveBlock( &adrCur, &adrEmp );
}