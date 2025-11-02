/*
 * settings_task.c
 *
 *  Created on: Jun 1, 2025
 *      Author: Ashish Bansal
 */
#include <usart.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "tasks.h"
#include "settings_task.h"

volatile xQueueHandle SettingsQueue;
volatile xQueueHandle Led_Blink_Queue[LED_COUNT];

void setting_task(void*)
{
    Settings queue_settings;

    while (1)
    {
        if (xQueueReceive(SettingsQueue, (uint8_t*) &queue_settings, portMAX_DELAY) != pdPASS)
        {
            continue;
        }

        switch (queue_settings.config_id)
        {
            case LED_CONFIG:
                uint8_t LedFlags = queue_settings.Buffer[0];
                uint32_t Rate = *(uint32_t*)(queue_settings.Buffer + 1);

                for (COLOR color = 0; color < LED_COUNT; ++color)
                {
                    if (LedFlags & (1 << color))
                    {
                        xQueueOverwrite(Led_Blink_Queue[color], &Rate);
                    }
                }

                break;

            case UART_CONFIG:
                uint32_t NewBaudRate = 0;
                memcpy(&NewBaudRate, queue_settings.Buffer, sizeof(NewBaudRate));

                huart1.Init.BaudRate = NewBaudRate;
                HAL_UART_Init(&huart1);
                break;

            case ADC_CONFIG:
                break;
            default:
                break;
        }
    }
}
