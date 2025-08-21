/*
 * adc_task.c
 *
 *  Created on: Jun 9, 2025
 *      Author: Ashish Bansal
 */

/* -- Standard Library -- */
#include <stdio.h>
#include <string.h>

/* -- STM32 Library -- */
#include "adc.h"

/* -- FreeRTOS Library -- */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* -- User Library -- */
#include "adc_task.h"

typedef uint16_t Adc_Raw_t;

extern ADC_HandleTypeDef hadc1;
extern volatile xQueueHandle Adc_to_cdc_queue;
extern xTaskHandle Adc_Task_Handle;

uint16_t latest_conv_val = 0;

void adc_task(void*)
{
    uint32_t Notify;
    size_t index = 0;

    Adc_Raw_t raw_values[RING_BUFFER_SIZE][BUFFER_SIZE] = {0};
    uint8_t CurrentBuffer = 0;

    HAL_ADC_Start_IT(&hadc1);

    while(1)
    {
        if( xTaskNotifyWait(0, 0xFFFFFFFF, &Notify, portMAX_DELAY) != pdPASS || Notify != DEF_NOTIFICATION_VAL)
        {
            continue;
        }

        SEGGER_SYSVIEW_Print("Conversion Complete\r\n");

        raw_values[CurrentBuffer][index] = latest_conv_val;

        xQueueSend(Adc_to_cdc_queue, raw_values[CurrentBuffer]+index, 0);

        index++;

        if(index == BUFFER_SIZE)
        {
            CurrentBuffer = CurrentBuffer == 0;
        }
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    latest_conv_val = HAL_ADC_GetValue(hadc);
    xTaskNotify(Adc_Task_Handle, DEF_NOTIFICATION_VAL, eNoAction);
}
