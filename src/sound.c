/*
 * sound.c
 *
 *  Created on: 11 февр. 2017 г.
 *      Author: se
 */

#include "stm32f0xx.h"
#include "main.h"
#include "sound.h"

//===========================================================================
//===========================================================================
//для кругового буфера событий
#define SND_LEN_BITS   4
#define SND_LEN_BUF    (1<<SND_LEN_BITS) // 8 или 2^3 или (1<<3)
#define SND_LEN_MASK   (SND_LEN_BUF-1)   // bits: 0000 0111
static uint8_t sndbufEv[SND_LEN_BUF] = {0};
static uint8_t sndtail = 0;
static uint8_t sndhead = 0;
/*
 * возвращает 1 если в кольцевом буфере есть свободное место для элемента, иначе 0
 */
static int sndHasFree(void) {
	if ( ((sndhead + 1) & SND_LEN_MASK) == sndtail ) {
		return 0;//свободного места нет
	}
	return 1;//есть свободное место
}
/*
 * помещает событие в круговой буфер
 * return 1 - успешно; 0 - нет места в буфере
 */
int sndPutEv(uint8_t event) {
	if (event == 0) {
		return 1;//событие с нулевым кодом пусть не будет для удобства
	}
	if ( sndHasFree() ) {
		sndbufEv[sndhead] = event;
		sndhead = (1 + sndhead) & SND_LEN_MASK;//инкремент кругового индекса
		return 1;
	} else {
		return 0;//нет места в буфере
	}
}
/*
 *  извлекает событие из кругового буфера
 *  если 0 - нет событий
 */
uint8_t sndGetEv(void) {
	uint8_t event = 0;
	if (sndhead != sndtail) {//если в буфере есть данные
		event = sndbufEv[sndtail];
		sndtail = (1 + sndtail) & SND_LEN_MASK;//инкремент кругового индекса
	}
	return event;
}
//===========================================================================
//===========================================================================

//typedef void (*pSndFun)(void);
//static pSndFun pSnd = NULL;

//static uint32_t mstime = 0;

//void sndBeepState(void) {
//	if ( mstime == 0 ) {
//		TIM_SetCounter( TIM17, 0);
//		TIM_SetAutoreload( TIM17, 333 );
//		TIM_SetCompare1( TIM17, 333-40 );
//		TIM_CtrlPWMOutputs( TIM17, ENABLE );
//		TIM_Cmd( TIM17, ENABLE );
//	}
//	mstime++;
//	if ( mstime == 4 ) {
//		TIM_CtrlPWMOutputs( TIM17, DISABLE );
//		TIM_Cmd( TIM17, DISABLE );
//		BEEP_OFF;
//		pSnd = NULL;
//	}
//}
/*
RTTTL Format Specifications

RTTTL (RingTone Text Transfer Language) is the primary format used to distribute
ringtones for Nokia phones. An RTTTL file is a text file, containing the
ringtone name, a control section and a section containing a comma separated
sequence of ring tone commands. White space must be ignored by any reader
application.

Example:
Simpsons:d=4,o=5,b=160:32p,c.6,e6,f#6,8a6,g.6,e6,c6,8a,8f#,8f#,8f#,2g

This file describes a ringtone whose name is 'Simpsons'. The control section
sets the beats per minute at 160, the default note length as 4, and the default
scale as Octave 5.
<RTX file> := <name> ":" [<control section>] ":" <tone-commands>

	<name> := <char> ; maximum name length 10 characters

	<control-section> := <control-pair> ["," <control-section>]

		<control-pair> := <control-name> ["="] <control-value>

		<control-name> := "o" | "d" | "b"
		; Valid in control section: o=default scale, d=default duration, b=default beats per minute.
		; if not specified, defaults are 4=duration, 6=scale, 63=beats-per-minute
		; any unknown control-names must be ignored

		<tone-commands> := <tone-command> ["," <tone-commands>]

		<tone-command> := <note> | <control-pair>

		<note> := [<duration>] <note> [<scale>] [<special-duration>] <delimiter>

			<duration> := "1" | "2" | "4" | "8" | "16" | "32"
			; duration is divider of full note duration, eg. 4 represents a quarter note

			<note> := "P" | "C" | "C#" | "D" | "D#" | "E" | "F" | "F#" | "G" | "G#" | "A" | "A#" | "B"

			<scale> :="4" | "5" | "6" | "7"
			; Note that octave 4: A=440Hz, 5: A=880Hz, 6: A=1.76 kHz, 7: A=3.52 kHz
			; The lowest note on the Nokia 61xx is A4, the highest is B7

			<special-duration> := "." ; Dotted note (признак увеличения длительности ноты на 50%)

; End of specification
*/

// Коллекция мелодий в стандарте RTTL (first song plays at power on)
const char *r_peek = { "peek:d=16,o=5,b=120:f,f" };
const char *r_Xfiles = { "Xfiles:d=4,o=5,b=125:e,b,a,b,d6,2b.,1p" }; //,e,b,a,b,e6,2b.,1p,g6,f#6,e6,d6,e6,2b.,1p,g6,f#6,e6,d6,f#6,2b.,1p,e,b,a,b,d6,2b.,1p,e,b,a,b,e6,2b.,1p,e6,2b.",
const char *r_Eternally = { "Eternally:d=4,o=5,b=112:b,8b,8a,8b,8c6,a,8a,8g,8a,8b,g,8g,8f#,8e,8d#,2e" };
const char *r_Batman = { "Batman:d=8,o=5,b=160:16a,16g#,16g,16f#,16f,16f#,16g,16g#,4a." }; //,p,d,d,c#,c#,c,c,c#,c#,d,d,c#,c#,c,c,c#,c#,d,d,c#,c#,c,c,c#,c#,g6,p,4g6",
const char *r_Simpsons = { "Simpsons:d=4,o=5,b=160:32p,c.6,e6,f#6,8a6,g.6,e6,c6,8a,8f#,8f#,8f#,2g" };
const char *r_TheSimpsons = { "TheSimpsons:d=4,o=5,b=160:c.6,e6,f#6,8a6,g.6,e6,c6,8a,8f#,8f#,8f#,2g,8p,8p,8f#,8f#,8f#,8g,a#.,8c6,8c6,8c6,c6" };
const char *r_DasBoot = { "DasBoot:d=4,o=5,b=100:d#.4,8d4,8c4,8d4,8d#4,8g4,a#.4,8a4,8g4,8a4,8a#4,8d,2f.,p,f.4,8e4,8d4,8e4,8f4,8a4,c.,8b4,8a4,8b4,8c,8e,2g.,2p" };
const char *r_TakeOnMe = { "TakeOnMe:d=4,o=4,b=160:8f#5,8f#5,8f#5,8d5,8p,8b,8p,8e5,8p,8e5,8p,8e5,8g#5,8g#5,8a5,8b5,8a5,8a5,8a5,8e5,8p,8d5,8p,8f#5,8p,8f#5,8p,8f#5,8e5,8e5,8f#5,8e5,8f#5,8f#5,8f#5,8d5,8p,8b,8p,8e5,8p,8e5,8p,8e5,8g#5,8g#5,8a5,8b5,8a5,8a5,8a5,8e5,8p,8d5,8p,8f#5,8p,8f#5,8p,8f#5,8e5,8e5" };
const char *r_MissionImp = { "MissionImp:d=4,o=6,b=150:16d5,16d#5,16d5,16d#5,16d5,16d#5,16d5,16d5,16d#5,16e5,16f5,16f#5,16g5,8g5,4p,8g5,4p,8a#5,8p,8c6,8p,8g5,4p,8g5,4p,8f5,8p,8p,8g5,4p,4p,8a#5,8p,8c6,8p,8g5,4p,4p,8f5,8p,8f#5,8p,8a#5,8g5,1d5" };
const char *r_ET = { "ET:d=2,o=6,b=200:d,a,8g,8f#,8e,8f#,d,1a5,b5,b,8a,8g#,8f#,8g#,e,1c#7,e,d7,8c#7,8b,8a,8g,f,d.,16d,16c#,16d,16e,f,d,d7,1c#7" };
const char *r_Axelf = { "Axelf:d=8,o=5,b=160:4f#,a.,f#,16f#,a#,f#,e,4f#,c6.,f#,16f#,d6,c#6,a,f#,c#6,f#6,16f#,e,16e,c#,g#,4f#." };
const char *r_Hogans = { "Hogans:d=16,o=6,b=45:f5.,g#5.,c#.,f.,f#,32g#,32f#.,32f.,8d#.,f#,32g#,32f#.,32f.,d#.,g#5.,c#,32c,32c#.,32a#5.,8g#5.,f5.,g#5.,c#.,f5.,32f#5.,a#5.,32f#5.,d#.,f#.,32f.,g#.,32f.,c#.,d#.,8c#." };
const char *r_Pinkpanther = { "Pinkpanther:d=16,o=5,b=160:8d#,8e,2p,8f#,8g,2p,8d#,8e,p,8f#,8g,p,8c6,8b,p,8d#,8e,p,8b,2a#,2p,a,g,e,d,2e" };
const char *r_Countdown = { "Countdown:d=4,o=5,b=125:p,8p,16b,16a,b,e,p,8p,16c6,16b,8c6,8b,a,p,8p,16c6,16b,c6,e,p,8p,16a,16g,8a,8g,8f#,8a,g.,16f#,16g,a.,16g,16a,8b,8a,8g,8f#,e,c6,2b.,16b,16c6,16b,16a,1b" };
const char *r_Adamsfamily = { "Adamsfamily:d=4,o=5,b=160:8c,f,8a,f,8c,b4,2g,8f,e,8g,e,8e4,a4,2f,8c,f,8a,f,8c,b4,2g,8f,e,8c,d,8e,1f,8c,8d,8e,8f,1p,8d,8e,8f#,8g,1p,8d,8e,8f#,8g,p,8d,8e,8f#,8g,p,8c,8d,8e,8f" };
const char *r_Indiana = { "Indiana:d=4,o=5,b=250:e,8p,8f,8g,8p,1c6,8p.,d,8p,8e,1f,p.,g,8p,8a,8b,8p,1f6,p,a,8p,8b,2c6,2d6,2e6,e,8p,8f,8g,8p,1c6,p,d6,8p,8e6,1f.6,g,8p,8g,e.6,8p,d6,8p,8g,e.6,8p,d6,8p,8g,f.6,8p,e6,8p,8d6,2c6" };
const char *r_Barbiegirl = { "Barbiegirl:d=4,o=5,b=125:8g#,8e,8g#,8c#6,a,p,8f#,8d#,8f#,8b,g#,8f#,8e,p,8e,8c#,f#,c#,p,8f#,8e,g#,f#" };
const char *r_Entertainer = { "Entertainer:d=4,o=5,b=140:8d,8d#,8e,c6,8e,c6,8e,2c.6,8c6,8d6,8d#6,8e6,8c6,8d6,e6,8b,d6,2c6,p,8d,8d#,8e,c6,8e,c6,8e,2c.6,8p,8a,8g,8f#,8a,8c6,e6,8d6,8c6,8a,2d6" };
const char *r_Autumn = { "Autumn:d=8,o=6,b=125:a,a,a,a#,4a,a,a#,a,a,a,a#,4a,a,a#,a,16g,16a,a#,a,g.,16p,a,a,a,a#,4a,a,a#,a,a,a,a#,4a,a,a#,a,16g,16a,a#,a,g." };
const char *r_Spring = { "Spring:d=16,o=6,b=125:8e,8g#,8g#,8g#,f#,e,4b.,b,a,8g#,8g#,8g#,f#,e,4b.,b,a,8g#,a,b,8a,8g#,8f#,8d#,4b.,8e,8g#,8g#,8g#,f#,e,4b.,b,a,8g#,8g#,8g#,f#,e,4b.,b,a,8g#,a,b,8a,8g#,8f#,8d#,4b." };
const char *r_Gadget = { "Gadget:d=16,o=5,b=50:32d#,32f,32f#,32g#,a#,f#,a,f,g#,f#,32d#,32f,32f#,32g#,a#,d#6,4d6,32d#,32f,32f#,32g#,a#,f#,a,f,g#,f#,8d#" };
const char *r_Looney = { "Looney:d=4,o=5,b=140:32p,c6,8f6,8e6,8d6,8c6,a.,8c6,8f6,8e6,8d6,8d#6,e.6,8e6,8e6,8c6,8d6,8c6,8e6,8c6,8d6,8a,8c6,8g,8a#,8a,8f" };
const char *r_Muppets = { "Muppets:d=4,o=5,b=250:c6,c6,a,b,8a,b,g,p,c6,c6,a,8b,8a,8p,g.,p,e,e,g,f,8e,f,8c6,8c,8d,e,8e,8e,8p,8e,g,2p,c6,c6,a,b,8a,b,g,p,c6,c6,a,8b,a,g.,p,e,e,g,f,8e,f,8c6,8c,8d,e,8e,d,8d,c" };
const char *r_Halloween = { "Halloween:d=4,o=5,b=180:8d6,8g,8g,8d6,8g,8g,8d6,8g,8d#6,8g,8d6,8g,8g,8d6,8g,8g,8d6,8g,8d#6,8g,8c#6,8f#,8f#,8c#6,8f#,8f#,8c#6,8f#,8d6,8f#,8c#6,8f#,8f#,8c#6,8f#,8f#,8c#6,8f#,8d6,8f#" };
const char *r_Careaboutus = { "Careaboutus:d=4,o=5,b=125:16f,16e,16f,16e,16f,16e,8d,16e,16d,16e,16d,16e,16d,16c,16d,d" };
const char *r_Timetosay = { "Timetosay:d=4,o=5,b=80:8c,16d,16e,16d,16e,16f#,16g,16f#,16g,16a,16g,16e,16a,16b,c6,b" };
const char *r_KnightRider = { "KnightRider:d=4,o=5,b=125:16e,16p,16f,16e,16e" };

//const int nsongs = sizeof(rtttl_library)/sizeof(rtttl_library[0]);

// Этот массив хранит частоты музыкальных нот
const uint16_t note[4][12] =
{	// C    C#    D     D#    E     F     F#    G     G#    A     A#    B
	{262,  277,  294,  311,  330,  349,  370,  392,  415,  440,  466,  494}, //4-ая окт.
	{523,  554,  587,  622,  659,  698,  740,  784,  830,  880,  932,  988}, //5-ая окт.
	{1047, 1109, 1175, 1244, 1319, 1397, 1480, 1568, 1660, 1760, 1865, 1976}, //6-ая окт.
	{2093, 2218, 2349, 2489, 2637, 2794, 2960, 3136, 3320, 3520, 3728, 3951}  //7-ая окт.
};

/*
 * вызывается из main раз в 1 мс
 */
void sound(void)
{
	//static int canSound = 0; //пока звук запрещен, события звука теряются
	uint8_t event = 0;
	uint8_t rInit = 0;
	//======================================
	static const char *song = NULL;
	static uint16_t tempo;
	static char duration;
	static char octave;
	uint8_t temp_duration, temp_octave, current_note, dot_flag;
	uint16_t calc_duration;
	//=======================================

	event = sndGetEv();
	switch ( event ) {
	case SND_PERMIT:
		//canSound = 1;
		break;
	case SND_BEEP:
	case SND_peek:
		rInit = 1;
		song = r_peek;
		break;
	case SND_Xfiles:
		rInit = 1;
		song = r_Xfiles;
		break;
	}

	if ( song == NULL ) {
		return;
	}

	if ( rInit ) {
		duration = 4; // Длительность звучания = 4/4 = 1
		tempo = 63;   // Темп = 63 bpm
		octave = 6;   // Октава = 6-я

		while (*song != ':') song++;  // Найти первый символ ':'
		song++;                       // Пропустить символ ':'
		while ( *song != ':' )            // Повторять до символа ':'
		{
			if (*song == 'd')           // Если это символ длительности ноты, то
			{
				duration = 0;             // временно duration=0
				song++;                   // Перейти к следующему символу
				while (*song == '=') song++;  // Символ '=' пропустить
				while (*song == ' ') song++;  // Пробелы пропустить
				// Если символ – число, то записать его в переменную duration
				if (*song >= '0' && *song <='9') duration = *song - '0';
				song++;                   // Перейти к следующему символу
				// Если символ – тоже число (длительность может состоять из двух цифр), то
				if (*song >= '0' && *song <= '9')
				{ // умножить на 10 и добавить текущую цифру (символ)
					duration = duration*10 + (*song - '0');
					song++;                 // Перейти к следующему символу
				}
				while (*song == ',') song++;  // Пропустить символ ','
			}
			if (*song == 'o')           // Если это указатель октавы, то
			{
				octave = 0;               // временно octave=0
				song++;                   // Перейти к следующему символу
				while (*song == '=') song++;  // Пропустить символ '='
				while (*song == ' ') song++;  // Пропустить пробелы
				// Если символ – число, то записать его в переменную octave
				if (*song >= '0' && *song <= '9') octave = *song - '0';
				song++;                   // Перейти к следующему символу
				while (*song == ',') song++;  // Пропустить символ ','
			}
			if (*song == 'b')           // Если это указатель темпа (нот в минуту),
			{
				tempo = 0;                // временно tempo=0
				song++;                   // Перейти к следующему символу
				while (*song == '=') song++;  // Пропустить символ '='
				while (*song == ' ') song++;  // Пропустить пробелы
				// Теперь читать цифры темпа (может быть число из 3 цифр)
				if (*song >= '0' && *song <= '9') tempo = *song - '0';
				song++;                   // Перейти к следующему символу
				if (*song >= '0' && *song <= '9')
				{
					tempo = tempo*10 + (*song - '0'); // Число состоит из 2 цифр
					song++;                 // Перейти к следующему символу
					if (*song >= '0' && *song <= '9')
					{
						tempo = tempo*10 + (*song - '0'); // Число состоит из 3 цифр
						song++;               // Перейти к следующему символу
					}
				}
				while (*song == ',') song++;  // Пропустить символ ','
			}
			while (*song == ',') song++;    // Пропустить символ ','
		}
		song++;                       // Перейти к следующему символу
	}
	// read the musical notes
	TIM_Cmd(TIM17, ENABLE);

	//while (*song)                 // Повторять, пока символы не пустые
	{
		current_note = 255;         // Текущая нота - пауза
		temp_octave = octave;       // Установить октаву по умолчанию
		temp_duration = duration;   // Установить длительность по умолчанию
		dot_flag = 0;               // Сбросить флаг dot flag
		// Определить и обработать префикс длительности
		if ( *song >= '0' && *song <= '9' ) {
			temp_duration = *song - '0';
			song++;
			if ( *song >= '0' && *song <= '9' ) {
				temp_duration = temp_duration*10 + (*song - '0');
				song++;
			}
		}
		// Определить ноту
		switch (*song)
		{
		case 'c': current_note = 0; break;    // C (до)
		case 'd': current_note = 2; break;    // D (ре)
		case 'e': current_note = 4; break;    // E (ми)
		case 'f': current_note = 5; break;    // F (фа)
		case 'g': current_note = 7; break;    // G (соль)
		case 'a': current_note = 9; break;    // A (ля)
		case 'b': current_note = 11; break;   // B (си)
		case 'p': current_note = 255; break;  // пауза
		}
		song++;                     // Перейти к следующему символу
		// Если за символом ноты следует символ #,
		if (*song=='#'){
			current_note++;   // то увеличить на 1 переменную note
			//(A-A#, C-C#, D-D#, F-F#, G-G#)
			song++;                   // Перейти к следующему символу
		}
		// Если следующий символ '.' (признак увеличения длительности ноты на 50%),
		if (*song=='.'){
			dot_flag = 1;             // то установить флаг dot flag
			song++;                   // Перейти к следующему символу
		}
		// Анализировать признак октавы
		if (*song >= '0' && *song <= '9'){
			temp_octave = *song - '0';// Установить октаву
			song++;                   // Перейти к следующему символу
		}
		if (*song=='.'){// За признаком октавы должна следовать точка
			dot_flag = 1;             // Если это так, то установить флаг dot flag
			song++;                   // Перейти к следующему символу
		}
		while (*song == ',') song++;    // Пропустить символ ','


		// Рассчитать длительность ноты
		calc_duration = ( 60000/tempo ) / ( temp_duration );
		calc_duration *= 4;         // Нота состоит из 4 интервалов темпа
		// Если флаг dot flag установлен, то увеличить длительность на 50%
		if (dot_flag) {
			calc_duration = (calc_duration*3) / 2;
		}
		// Если текущая нота – не пауза, то проигрывать эту ноту с использованием
		// функции sound
		if ( current_note < 255 ) {
			uint32_t x = (uint32_t)1000000 / note[temp_octave-4][current_note];
			TIM_SetAutoreload(TIM17, x);
			TIM_SetCompare1(TIM17, x >> 1);
		}
		else
		{ // Если текущая нота = 255 (пауза), то реализовать задержку
			TIM_SetCompare1(TIM17, 0);
		}


		// перейти к следующей мелодии
		//vTaskDelay(calc_duration / portTICK_RATE_MS);
	}



	TIM_SetAutoreload(TIM17, 0xFFFF);
	TIM_SetCompare1(TIM17, 0);
	TIM_Cmd(TIM17, DISABLE);
}
