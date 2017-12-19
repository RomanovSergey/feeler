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
#define SND_TheSimpsons  0x86
#define SND_DasBoot      0x87
#define SND_TakeOnMe     0x88
#define SND_MissionImp   0x89

int sndPutEv(uint8_t event);

int sndGetSize();
char* sndGetName( uint8_t ind );
void sound(void);

#endif /* SRC_SOUND_H_ */
