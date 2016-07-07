/*
 * buttons.c
 *
 *  Created on: 30 апр. 2016 г.
 *      Author: Se
 */

#include "stm32f0xx.h"
#include "main.h"

#define READ_B1     GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_8)
#define ANTI_TIME   30    //time of debounce
#define LPUSH_TIME  1000  //time for long push button event generation

typedef struct  {
	uint16_t debcount;//count of acquision (debounce)
	uint8_t  current;//current middle state of Button
	uint16_t debcurrent;//count of current state of pressed Button
	uint8_t  prev;//previes middle state of Button
	uint8_t* ptrPush; //pointer on global - event push (will reset by handler)
	uint8_t* ptrLPush;//pointer on global - event long push (will reset by handler)
} button_t;

//only one button yet
button_t B1 = {
	.debcount = 0,
	.current = 0,
	.debcurrent = 0,
	.prev = 0,
	.ptrPush = &g.b1_push,
	.ptrLPush = &g.b1_Lpush,
};

//================================================================================

/*
 * Local function debounce, takes pointer
 * to the struct of the button and button instance sensor
 */
void debounce(button_t *b, uint8_t instance) {
	if ( instance > 0 ) {//if button is pushed *************************
		if ( b->debcount < ANTI_TIME ) {
			b->debcount++;//filter
		} else {
			if ( b->prev == 0 ) {//if state is change
				b->current = 1;
				b->debcurrent = 0;
				b->prev = 1;
				if ( b->ptrPush != NULL ) {
					*(b->ptrPush) = 1;//to generate global push button event
				}
			} else if ( b->debcurrent > LPUSH_TIME ) {//if time of push exceed long time
				if ( b->ptrLPush != NULL ) {
					*(b->ptrLPush) = 1;//to generate global long push button event
				}
			} else {
				b->debcurrent++;
			}
		}
	} else {//if button is pulled **************************************
		if ( b->debcount > 0 ) {
			b->debcount--;//filter
		} else {
			if ( b->prev == 1 ) {//if state is change
				b->current = 0;
				b->prev = 0;
				//yet no task to generate global pull event
			}
		}
	}
}


/*
 * this function called from main loop every 1 ms
 */
void buttons(void) {

	debounce(&B1, READ_B1);//read and filter B1
}

