/*
 * buttons.h
 *
 *  Created on: 30 апр. 2016 г.
 *      Author: se
 */

#ifndef INC_BUTTONS_H_
#define INC_BUTTONS_H_

void butNo(void);
void butWait(void);
void butProcess(void);

typedef void (*pBut_t)(void);
extern pBut_t pButton;

void buttons(void);

#endif /* INC_BUTTONS_H_ */
