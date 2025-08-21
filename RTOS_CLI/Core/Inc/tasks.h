/*
 * tasks.h
 *
 *  Created on: May 30, 2025
 *      Author: Ashish Bansal
 */

#ifndef INC_TASKS_H_
#define INC_TASKS_H_

typedef enum
{
    BLUE,
    RED,
    ORANGE,
    GREEN,
    LED_COUNT
}COLOR;

#define BIT(m)  (1 << (m))

void create_led_tasks();

void Cli_Task(void *Arguments);

void adc_task(void*);
void cdc_task(void*);

#endif /* INC_TASKS_H_ */
