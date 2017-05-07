/*
 * flash2.h
 *
 *  Created on: Apr 27, 2017
 *      Author: se
 */

#ifndef FLASH2_H_
#define FLASH2_H_

#define FID_FE_DEF   (1<<8)|(20*2+2)  // ferrum default
#define FID_AL_DEF   (2<<8)|(20*2+2)  // alumin default
#define FNUMREC       2               // numbers of ID, from 1 to FNUMREC

int flashInit(void);
int fwrite( const uint16_t IDLEN, const uint16_t* const buf );
int fread( const uint16_t IDLEN, uint16_t* const buf );

#endif /* FLASH2_H_ */
