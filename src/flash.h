/*
 * flash.h
 *
 *  Created on: 12 марта 2017 г.
 *      Author: se
 */

#ifndef SRC_FLASH_H_
#define SRC_FLASH_H_

uint16_t fread16(uint32_t Address);
FLASH_Status fwriteInc( uint16_t hw );
FLASH_Status fzeroInc( void );
FLASH_Status ferasePage( void );

#endif /* SRC_FLASH_H_ */
