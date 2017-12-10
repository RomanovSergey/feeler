/*
 * sound.c
 *
 *  Created on: 11 февр. 2017 г.
 *      Author: se
 */

#include "stm32f0xx.h"
#include "main.h"
#include "sound.h"

/* converted rtttl melody to convenient for cpu image */

typedef struct {
    uint16_t period;
    uint16_t delay;
} rtttl_img;

rtttl_img r_peek[] = {
	{1432, 124},{1432, 124},
};
rtttl_img r_xfiles[] = {
	{1517, 480},{1012, 480},{1136, 480},{1012, 480},{ 851, 480},{1012,1440},{   0,1920},

};
rtttl_img r_eternally[] = {
	{1012, 532},{1012, 264},{1136, 264},{1012, 264},{ 955, 264},{1136, 532},{1136, 264},
	{1275, 264},{1136, 264},{1012, 264},{1275, 532},{1275, 264},{1351, 264},{1517, 264},
	{1607, 264},{1517,1068},
};
rtttl_img r_batman[] = {
	{1136,  92},{1204,  92},{1275,  92},{1351,  92},{1432,  92},{1351,  92},{1275,  92},
	{1204,  92},{1136, 558},
};
rtttl_img r_simpsons[] = {
	{   0,  44},{ 955, 558},{ 758, 372},{ 675, 372},{ 568, 184},{ 637, 558},{ 758, 372},
	{ 955, 372},{1136, 184},{1351, 184},{1351, 184},{1351, 184},{1275, 748},
};
rtttl_img r_thesimpsons[] = {
	{ 955, 558},{ 758, 372},{ 675, 372},{ 568, 184},{ 637, 558},{ 758, 372},{ 955, 372},
	{1136, 184},{1351, 184},{1351, 184},{1351, 184},{1275, 748},{   0, 184},{   0, 184},
	{1351, 184},{1351, 184},{1351, 184},{1275, 184},{1072, 558},{ 955, 184},{ 955, 184},
	{ 955, 184},{ 955, 372},
};
rtttl_img r_dasboot[] = {
	{3215, 900},{3401, 300},{3816, 300},{3401, 300},{3215, 300},{2551, 300},{2145, 900},
	{2272, 300},{2551, 300},{2272, 300},{2145, 300},{1703, 300},{1432,1800},{   0, 600},
	{2865, 900},{3030, 300},{3401, 300},{3030, 300},{2865, 300},{2272, 300},{1912, 900},
	{2024, 300},{2272, 300},{2024, 300},{1912, 300},{1517, 300},{1275,1800},{   0,1200},

};
rtttl_img r_takeonme[] = {
	{1351, 184},{1351, 184},{1351, 184},{1703, 184},{   0, 184},{2024, 184},{   0, 184},
	{1517, 184},{   0, 184},{1517, 184},{   0, 184},{1517, 184},{1204, 184},{1204, 184},
	{1136, 184},{1012, 184},{1136, 184},{1136, 184},{1136, 184},{1517, 184},{   0, 184},
	{1703, 184},{   0, 184},{1351, 184},{   0, 184},{1351, 184},{   0, 184},{1351, 184},
	{1517, 184},{1517, 184},{1351, 184},{1517, 184},{1351, 184},{1351, 184},{1351, 184},
	{1703, 184},{   0, 184},{2024, 184},{   0, 184},{1517, 184},{   0, 184},{1517, 184},
	{   0, 184},{1517, 184},{1204, 184},{1204, 184},{1136, 184},{1012, 184},{1136, 184},
	{1136, 184},{1136, 184},{1517, 184},{   0, 184},{1703, 184},{   0, 184},{1351, 184},
	{   0, 184},{1351, 184},{   0, 184},{1351, 184},{1517, 184},{1517, 184},
};
rtttl_img r_missionimp[] = {
	{1703, 100},{1607, 100},{1703, 100},{1607, 100},{1703, 100},{1607, 100},{1703, 100},
	{1703, 100},{1607, 100},{1517, 100},{1432, 100},{1351, 100},{1275, 100},{1275, 200},
	{   0, 400},{1275, 200},{   0, 400},{1072, 200},{   0, 200},{ 955, 200},{   0, 200},
	{1275, 200},{   0, 400},{1275, 200},{   0, 400},{1432, 200},{   0, 200},{   0, 200},
	{1275, 200},{   0, 400},{   0, 400},{1072, 200},{   0, 200},{ 955, 200},{   0, 200},
	{1275, 200},{   0, 400},{   0, 400},{1432, 200},{   0, 200},{1351, 200},{   0, 200},
	{1072, 200},{1275, 200},{1703,1600},
};
rtttl_img r_et[] = {
	{ 851, 600},{ 568, 600},{ 637, 148},{ 675, 148},{ 758, 148},{ 675, 148},{ 851, 600},
	{1136,1200},{1012, 600},{ 506, 600},{ 568, 148},{ 602, 148},{ 675, 148},{ 602, 148},
	{ 758, 600},{ 450,1200},{ 758, 600},{ 425, 600},{ 450, 148},{ 506, 148},{ 568, 148},
	{ 637, 148},{ 715, 600},{ 851, 900},{ 851,  72},{ 901,  72},{ 851,  72},{ 758,  72},
	{ 715, 600},{ 851, 600},{ 425, 600},{ 450,1200},
};
rtttl_img r_axelf[] = {
	{1351, 372},{1136, 276},{1351, 184},{1351,  92},{1072, 184},{1351, 184},{1517, 184},
	{1351, 372},{ 955, 276},{1351, 184},{1351,  92},{ 851, 184},{ 901, 184},{1136, 184},
	{1351, 184},{ 901, 184},{ 675, 184},{1351,  92},{1517, 184},{1517,  92},{1805, 184},
	{1204, 184},{1351, 558},
};
rtttl_img r_hogans[] = {
	{1432, 498},{1204, 498},{ 901, 498},{ 715, 498},{ 675, 332},{ 602, 164},{ 675, 246},
	{ 715, 246},{ 803, 996},{ 675, 332},{ 602, 164},{ 675, 246},{ 715, 246},{ 803, 498},
	{1204, 498},{ 901, 332},{ 955, 164},{ 901, 246},{1072, 246},{1204, 996},{1432, 498},
	{1204, 498},{ 901, 498},{1432, 498},{1351, 246},{1072, 498},{1351, 246},{ 803, 498},
	{ 675, 498},{ 715, 246},{ 602, 498},{ 715, 246},{ 901, 498},{ 803, 498},{ 901, 996},

};
rtttl_img r_pinkpanther[] = {
	{1607, 184},{1517, 184},{   0, 748},{1351, 184},{1275, 184},{   0, 748},{1607, 184},
	{1517, 184},{   0,  92},{1351, 184},{1275, 184},{   0,  92},{ 955, 184},{1012, 184},
	{   0,  92},{1607, 184},{1517, 184},{   0,  92},{1012, 184},{1072, 748},{   0, 748},
	{1136,  92},{1275,  92},{1517,  92},{1703,  92},{1517, 748},
};
rtttl_img r_countdown[] = {
	{   0, 480},{   0, 240},{1012, 120},{1136, 120},{1012, 480},{1517, 480},{   0, 480},
	{   0, 240},{ 955, 120},{1012, 120},{ 955, 240},{1012, 240},{1136, 480},{   0, 480},
	{   0, 240},{ 955, 120},{1012, 120},{ 955, 480},{1517, 480},{   0, 480},{   0, 240},
	{1136, 120},{1275, 120},{1136, 240},{1275, 240},{1351, 240},{1136, 240},{1275, 720},
	{1351, 120},{1275, 120},{1136, 720},{1275, 120},{1136, 120},{1012, 240},{1136, 240},
	{1275, 240},{1351, 240},{1517, 480},{ 955, 480},{1012,1440},{1012, 120},{ 955, 120},
	{1012, 120},{1136, 120},{1012,1920},
};
rtttl_img r_adamsfamily[] = {
	{1912, 184},{1432, 372},{1136, 184},{1432, 372},{1912, 184},{2024, 372},{1275, 748},
	{1432, 184},{1517, 372},{1275, 184},{1517, 372},{3030, 184},{2272, 372},{1432, 748},
	{1912, 184},{1432, 372},{1136, 184},{1432, 372},{1912, 184},{2024, 372},{1275, 748},
	{1432, 184},{1517, 372},{1912, 184},{1703, 372},{1517, 184},{1432,1500},{1912, 184},
	{1703, 184},{1517, 184},{1432, 184},{   0,1500},{1703, 184},{1517, 184},{1351, 184},
	{1275, 184},{   0,1500},{1703, 184},{1517, 184},{1351, 184},{1275, 184},{   0, 372},
	{1703, 184},{1517, 184},{1351, 184},{1275, 184},{   0, 372},{1912, 184},{1703, 184},
	{1517, 184},{1432, 184},
};
rtttl_img r_indiana[] = {
	{1517, 240},{   0, 120},{1432, 120},{1275, 120},{   0, 120},{ 955, 960},{   0, 180},
	{1703, 240},{   0, 120},{1517, 120},{1432, 960},{   0, 360},{1275, 240},{   0, 120},
	{1136, 120},{1012, 120},{   0, 120},{ 715, 960},{   0, 240},{1136, 240},{   0, 120},
	{1012, 120},{ 955, 480},{ 851, 480},{ 758, 480},{1517, 240},{   0, 120},{1432, 120},
	{1275, 120},{   0, 120},{ 955, 960},{   0, 240},{ 851, 240},{   0, 120},{ 758, 120},
	{ 715,1440},{1275, 240},{   0, 120},{1275, 120},{ 758, 360},{   0, 120},{ 851, 240},
	{   0, 120},{1275, 120},{ 758, 360},{   0, 120},{ 851, 240},{   0, 120},{1275, 120},
	{ 715, 360},{   0, 120},{ 758, 240},{   0, 120},{ 851, 120},{ 955, 480},
};
rtttl_img r_barbiegirl[] = {
	{1204, 240},{1517, 240},{1204, 240},{ 901, 240},{1136, 480},{   0, 480},{1351, 240},
	{1607, 240},{1351, 240},{1012, 240},{1204, 480},{1351, 240},{1517, 240},{   0, 480},
	{1517, 240},{1805, 240},{1351, 480},{1805, 480},{   0, 480},{1351, 240},{1517, 240},
	{1204, 480},{1351, 480},
};
rtttl_img r_entertainer[] = {
	{1703, 212},{1607, 212},{1517, 212},{ 955, 428},{1517, 212},{ 955, 428},{1517, 212},
	{ 955,1284},{ 955, 212},{ 851, 212},{ 803, 212},{ 758, 212},{ 955, 212},{ 851, 212},
	{ 758, 428},{1012, 212},{ 851, 428},{ 955, 856},{   0, 428},{1703, 212},{1607, 212},
	{1517, 212},{ 955, 428},{1517, 212},{ 955, 428},{1517, 212},{ 955,1284},{   0, 212},
	{1136, 212},{1275, 212},{1351, 212},{1136, 212},{ 955, 212},{ 758, 428},{ 851, 212},
	{ 955, 212},{1136, 212},{ 851, 856},
};
rtttl_img r_autumn[] = {
	{ 568, 240},{ 568, 240},{ 568, 240},{ 536, 240},{ 568, 480},{ 568, 240},{ 536, 240},
	{ 568, 240},{ 568, 240},{ 568, 240},{ 536, 240},{ 568, 480},{ 568, 240},{ 536, 240},
	{ 568, 240},{ 637, 120},{ 568, 120},{ 536, 240},{ 568, 240},{ 637, 360},{   0, 120},
	{ 568, 240},{ 568, 240},{ 568, 240},{ 536, 240},{ 568, 480},{ 568, 240},{ 536, 240},
	{ 568, 240},{ 568, 240},{ 568, 240},{ 536, 240},{ 568, 480},{ 568, 240},{ 536, 240},
	{ 568, 240},{ 637, 120},{ 568, 120},{ 536, 240},{ 568, 240},{ 637, 360},
};
rtttl_img r_spring[] = {
	{ 758, 240},{ 602, 240},{ 602, 240},{ 602, 240},{ 675, 120},{ 758, 120},{ 506, 720},
	{ 506, 120},{ 568, 120},{ 602, 240},{ 602, 240},{ 602, 240},{ 675, 120},{ 758, 120},
	{ 506, 720},{ 506, 120},{ 568, 120},{ 602, 240},{ 568, 120},{ 506, 120},{ 568, 240},
	{ 602, 240},{ 675, 240},{ 803, 240},{ 506, 720},{ 758, 240},{ 602, 240},{ 602, 240},
	{ 602, 240},{ 675, 120},{ 758, 120},{ 506, 720},{ 506, 120},{ 568, 120},{ 602, 240},
	{ 602, 240},{ 602, 240},{ 675, 120},{ 758, 120},{ 506, 720},{ 506, 120},{ 568, 120},
	{ 602, 240},{ 568, 120},{ 506, 120},{ 568, 240},{ 602, 240},{ 675, 240},{ 803, 240},
	{ 506, 720},
};
rtttl_img r_gadget[] = {
	{1607, 148},{1432, 148},{1351, 148},{1204, 148},{1072, 300},{1351, 300},{1136, 300},
	{1432, 300},{1204, 300},{1351, 300},{1607, 148},{1432, 148},{1351, 148},{1204, 148},
	{1072, 300},{ 803, 300},{ 851,1200},{1607, 148},{1432, 148},{1351, 148},{1204, 148},
	{1072, 300},{1351, 300},{1136, 300},{1432, 300},{1204, 300},{1351, 300},{1607, 600},

};
rtttl_img r_looney[] = {
	{   0,  52},{ 955, 428},{ 715, 212},{ 758, 212},{ 851, 212},{ 955, 212},{1136, 642},
	{ 955, 212},{ 715, 212},{ 758, 212},{ 851, 212},{ 803, 212},{ 758, 642},{ 758, 212},
	{ 758, 212},{ 955, 212},{ 851, 212},{ 955, 212},{ 758, 212},{ 955, 212},{ 851, 212},
	{1136, 212},{ 955, 212},{1275, 212},{1072, 212},{1136, 212},{1432, 212},
};
rtttl_img r_muppets[] = {
	{ 955, 240},{ 955, 240},{1136, 240},{1012, 240},{1136, 120},{1012, 240},{1275, 240},
	{   0, 240},{ 955, 240},{ 955, 240},{1136, 240},{1012, 120},{1136, 120},{   0, 120},
	{1275, 360},{   0, 240},{1517, 240},{1517, 240},{1275, 240},{1432, 240},{1517, 120},
	{1432, 240},{ 955, 120},{1912, 120},{1703, 120},{1517, 240},{1517, 120},{1517, 120},
	{   0, 120},{1517, 120},{1275, 240},{   0, 480},{ 955, 240},{ 955, 240},{1136, 240},
	{1012, 240},{1136, 120},{1012, 240},{1275, 240},{   0, 240},{ 955, 240},{ 955, 240},
	{1136, 240},{1012, 120},{1136, 240},{1275, 360},{   0, 240},{1517, 240},{1517, 240},
	{1275, 240},{1432, 240},{1517, 120},{1432, 240},{ 955, 120},{1912, 120},{1703, 120},
	{1517, 240},{1517, 120},{1703, 240},{1703, 120},{1912, 240},
};
rtttl_img r_halloween[] = {
	{ 851, 164},{1275, 164},{1275, 164},{ 851, 164},{1275, 164},{1275, 164},{ 851, 164},
	{1275, 164},{ 803, 164},{1275, 164},{ 851, 164},{1275, 164},{1275, 164},{ 851, 164},
	{1275, 164},{1275, 164},{ 851, 164},{1275, 164},{ 803, 164},{1275, 164},{ 901, 164},
	{1351, 164},{1351, 164},{ 901, 164},{1351, 164},{1351, 164},{ 901, 164},{1351, 164},
	{ 851, 164},{1351, 164},{ 901, 164},{1351, 164},{1351, 164},{ 901, 164},{1351, 164},
	{1351, 164},{ 901, 164},{1351, 164},{ 851, 164},{1351, 164},
};
rtttl_img r_careaboutus[] = {
	{1432, 120},{1517, 120},{1432, 120},{1517, 120},{1432, 120},{1517, 120},{1703, 240},
	{1517, 120},{1703, 120},{1517, 120},{1703, 120},{1517, 120},{1703, 120},{1912, 120},
	{1703, 120},{1703, 480},
};
rtttl_img r_timetosay[] = {
	{1912, 372},{1703, 184},{1517, 184},{1703, 184},{1517, 184},{1351, 184},{1275, 184},
	{1351, 184},{1275, 184},{1136, 184},{1275, 184},{1517, 184},{1136, 184},{1012, 184},
	{ 955, 748},{1012, 748},
};
rtttl_img r_knightrider[] = {
	{1517, 120},{   0, 120},{1432, 120},{1517, 120},{1517, 120},
};


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

rtttl_img *song = NULL;
volatile int ind_song = 0; // song index
volatile int ind_size = 0; // size current song

#define COMPARE 10

/*
 * вызывается из main раз в 1 мс
 */
void sound(void)
{
	uint8_t event = 0;

	event = sndGetEv();
	if ( event == 0 ) {
		return;
	}
	if ( event & 0x80 ) {
		switch ( event ) {
		case SND_BEEP:
		case SND_peek:
			song = r_peek;
			ind_size = sizeof( r_peek ) / sizeof( rtttl_img );
			break;
		case SND_Xfiles:
			song = r_xfiles;
			ind_size = sizeof( r_xfiles ) / sizeof( rtttl_img );
			break;
		case SND_Eternally:
			song = r_eternally;
			ind_size = sizeof( r_eternally ) / sizeof( rtttl_img );
			break;
		case SND_Batman:
			song = r_batman;
			ind_size = sizeof( r_batman ) / sizeof( rtttl_img );
			break;
		case SND_Simpsons:
			song = r_simpsons;
			ind_size = sizeof( r_simpsons ) / sizeof( rtttl_img );
			break;
		}
		ind_song = 0;
		TIM_SetCounter( TIM17, 0 );
		TIM_SetAutoreload( TIM17, song[0].period );
		TIM_SetCompare1( TIM17, COMPARE );
		TIM_SetCounter( TIM6, 0 );
		TIM_SetAutoreload( TIM6, song[0].delay );
		TIM_ITConfig( TIM6, TIM_IT_Update, ENABLE );
		TIM_Cmd( TIM17, ENABLE );
		TIM_Cmd( TIM6, ENABLE );
	} else {
		switch ( event ) {
		case SND_PERMIT:
			//canSound = 1;
			break;
		case SND_STOP:
			song = NULL;
			TIM_SetAutoreload( TIM17, 0xFFFF );
			TIM_SetCompare1( TIM17, 0 );
			TIM_Cmd( TIM17, DISABLE );
			TIM_Cmd( TIM6, DISABLE );
			TIM_ITConfig( TIM6, TIM_IT_Update, DISABLE );
			break;
		}
	}
}
//	if ( *song == 0 ) {
//		song = NULL;
//		TIM_SetAutoreload(TIM17, 0xFFFF);
//		TIM_SetCompare1(TIM17, 0);
//		TIM_Cmd(TIM17, DISABLE);
//		return;
//	}
//	if ( current_note < 255 ) {
//		uint32_t x = (uint32_t)1000000 / note[temp_octave-4][current_note];
//		TIM_SetAutoreload( TIM17, x );
//		TIM_SetCompare1( TIM17, 30 );
//	}
//	else
//	{ // Если текущая нота = 255 (пауза), то реализовать задержку
//		TIM_SetCompare1(TIM17, 0);
//	}


/*
 * interrupt handler
 */
void TIM6_DAC_IRQHandler( void )
{
	if ( TIM_GetITStatus( TIM6, TIM_IT_Update ) == SET ) {
		TIM_ClearITPendingBit( TIM6, TIM_IT_Update );
		ind_song++;
		if ( ind_song >= ind_size ) {
			song = NULL;
			TIM_SetAutoreload( TIM17, 0xFFFF );
			TIM_SetCompare1( TIM17, 0 );
			TIM_Cmd( TIM17, DISABLE );
			TIM_Cmd( TIM6, DISABLE );
			TIM_ITConfig( TIM6, TIM_IT_Update, DISABLE );
		}
		uint16_t per = song[ind_song].period;
		if ( per == 0 ) {
			//TIM_SetCounter( TIM17, 0 );
			//TIM_SetAutoreload( TIM17, 0xFFFF );
			TIM_SetCompare1( TIM17, 0 );
			TIM_SetCounter( TIM6, 0 );
			TIM_SetAutoreload( TIM6, song[ind_song].delay );
		} else {
			//TIM_SetCounter( TIM17, 0 );
			TIM_SetAutoreload( TIM17, per );
			TIM_SetCompare1( TIM17, COMPARE );
			TIM_SetCounter( TIM6, 0 );
			TIM_SetAutoreload( TIM6, song[ind_song].delay );
		}
	}
}



