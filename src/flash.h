/*
 * flash.h
 *
 *  Created on: 12 марта 2017 г.
 *      Author: se
 */

#ifndef SRC_FLASH_H_
#define SRC_FLASH_H_


#define FID_FE_DEF   1
#define FID_AL_DEF   2


uint16_t fread16(uint32_t Address);
//FLASH_Status fwriteInc( uint16_t hw );
//FLASH_Status fzeroInc( void );
int ferasePage( uint32_t adr );
int fsaveImage( const uint16_t ID, const uint16_t* const buf, const uint16_t len );
int floadImage( const uint16_t ID, uint16_t* buf, const uint16_t maxLen, uint16_t *rlen );

#endif /* SRC_FLASH_H_ */
