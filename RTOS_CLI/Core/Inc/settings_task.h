/*
 * settings_task.h
 *
 *  Created on: Jun 1, 2025
 *      Author: Ashish Bansal
 */

#ifndef _SETTINGS_TASK_H_
#define _SETTINGS_TASK_H_

#include <stdint.h>
#include <limits.h>

typedef enum
{
    LED_CONFIG, UART_CONFIG, ADC_CONFIG, CPU_USAGE_DISPLAY_CONFIG
} CONFIGS;

typedef struct
{
    CONFIGS config_id;
    uint8_t Buffer[UINT8_MAX];
} Settings;

void setting_task(void*);

#endif /* _SETTINGS_TASK_H_ */
