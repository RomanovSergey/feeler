/*
 * sound.h
 *
 *  Created on: 11 февр. 2017 г.
 *      Author: se
 */

#ifndef SRC_SOUND_H_
#define SRC_SOUND_H_

#define SND_BEEP          1
#define SND_STOP          2
#define SND_PERMIT       10
#define SND_peek         0x81
#define SND_Xfiles       0x82
#define SND_Eternally    0x83
#define SND_Batman       0x84
#define SND_Simpsons     0x85

int sndPutEv(uint8_t event);

void sound(void);

#endif /* SRC_SOUND_H_ */
