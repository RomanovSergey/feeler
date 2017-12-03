/*
 * sound.h
 *
 *  Created on: 11 февр. 2017 г.
 *      Author: se
 */

#ifndef SRC_SOUND_H_
#define SRC_SOUND_H_

#define SND_BEEP      1
#define SND_PERMIT   10
#define SND_peek     11
#define SND_Xfiles   12

int sndPutEv(uint8_t event);

void sound(void);

#endif /* SRC_SOUND_H_ */
