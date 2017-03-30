/*
 * pwr.h
 *
 *  Created on: Feb 17, 2017
 *      Author: se
 */

#ifndef PWR_H_
#define PWR_H_

#define PWR_POWEROFF    1
#define PWR_OVERTIME    2
#define PWR_BUTACTIV    3
#define PWR_POWER_HALF  4
#define PWR_ALARM_1000  10
#define PWR_ALARM_3000  30

int pwrPutEv(uint8_t event);
void power(void);

#endif /* PWR_H_ */
