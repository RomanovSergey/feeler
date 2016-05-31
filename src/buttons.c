/*
 * buttons.c
 *
 *  Created on: 30 апр. 2016 г.
 *      Author: Se
 */

#include "stm32f0xx.h"
#include "main.h"

#define READ_B1     GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0)
#define ANTI_TIME   30

typedef struct  {
	uint16_t count;//count of acquision (debounce)
	uint8_t  current;//current middle state of Button
	uint8_t  prev;//previes middle state of Button
	uint8_t* ptrPush;//pointer on global - event push (will reset by handler)
} button_t;

//only one button yet
button_t B1 = {
	.count = 0,
	.current = 0,
	.prev = 0,
	.ptrPush = &g.b1_push,
};

//================================================================================

/*
 * Local function debounce, takes pointer
 * to the struct of the button and button instance sensor
 */
void debounce(button_t *b, uint8_t instance) {
	if ( instance > 0 ) {//if button is pushed
		if ( b->count < ANTI_TIME ) {
			b->count++;//filter
		} else {
			b->current = 1;
			if ( b->prev == 0 ) {//if state is change
				b->prev = 1;
				if ( b->ptrPush != NULL ) {
					*(b->ptrPush) = 1;//to generate global push button event
				}
			}
		}
	} else {//if button is pulled
		if ( b->count > 0 ) {
			b->count--;//filter
		} else {
			b->current = 0;
			if ( b->prev == 1 ) {//if state is change
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

