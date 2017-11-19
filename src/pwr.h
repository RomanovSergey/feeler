/*
 * pwr.h
 *
 *  Created on: Feb 17, 2017
 *      Author: se
 */

#ifndef PWR_H_
#define PWR_H_

#define PWR_POWEROFF    1
#define PWR_INACTIVE    2
#define PWR_BUTACTIV    3
#define PWR_RESET_ALARM 10
#define PWR_ALARM_84    11
#define PWR_ALARM_1000  20
#define PWR_ALARM_3000  30

int pwrPutEv(uint8_t event);
void power(void);

#endif /* PWR_H_ */
