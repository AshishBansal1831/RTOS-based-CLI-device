/*
 * blue_led_task.c
 *
 *  Created on: May 30, 2025
 *      Author: Ashish Bansal
 */

#include "gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "tasks.h"
#include "queue.h"
#include "stm32f4xx_ll_gpio.h"

#define LED_TASK_SIZE   (100)

typedef struct
{
    COLOR colour;
    char* name;
    char* Task_Name;
    uint32_t blink_frequency;
    GPIO_TypeDef * Port;
    uint16_t Pin;

}LED;

LED Leds[LED_COUNT] = {
    {
        .colour          = BLUE,
        .name            = "BLUE",
        .Task_Name       = "Blue Led Task",
        .blink_frequency = 1000,
        .Port            = BLUE_LED_GPIO_Port,
        .Pin             = BLUE_LED_Pin
    },
    {
        .colour          = RED,
        .name            = "RED",
        .Task_Name       = "Red Led Task",
        .blink_frequency = 1000,
        .Port            = RED_LED_GPIO_Port,
        .Pin             = RED_LED_Pin
    },
    {
        .colour          = ORANGE,
        .name            = "ORANGE",
        .Task_Name       = "Orange Led Task",
        .blink_frequency = 1000,
        .Port            = ORANGE_LED_GPIO_Port,
        .Pin             = ORANGE_LED_Pin
    },
    {
        .colour          = GREEN,
        .name            = "GREEN",
        .Task_Name       = "Green Led Task",
        .blink_frequency = 1000,
        .Port            = GREEN_LED_GPIO_Port,
        .Pin             = GREEN_LED_Pin
    }
};

const char COLOR_NAMES[LED_COUNT][10] = { "BLUE", "RED", "ORANGE", "GREEN"};

extern volatile xQueueHandle Led_Blink_Queue[LED_COUNT];

void led_blink_task(void* Arguments)
{
    if(Arguments == NULL)
    {
        SEGGER_SYSVIEW_Print("Arguments required for led_blink_task");
        return;
    }

    LED *const LedPtr = (LED*)Arguments;
    TickType_t new_delay;

    if(LedPtr->colour >= LED_COUNT)
    {
        SEGGER_SYSVIEW_Print("Invalid Argument Passed for led_blink_task");
        return;
    }

    while(1)
    {
        LL_GPIO_TogglePin(LedPtr->Port, LedPtr->Pin);

        vTaskDelay((TickType_t) LedPtr->blink_frequency);
        if (xQueueReceive(Led_Blink_Queue[LedPtr->colour], &new_delay, 1) == pdPASS)
        {
            LedPtr->blink_frequency = new_delay;
        }
    }

}

void create_led_tasks()
{
    for(uint8_t iter = 0 ;  iter < LED_COUNT ; ++iter)
    {
        assert_param(xTaskCreate(led_blink_task, Leds[iter].Task_Name, LED_TASK_SIZE, Leds + iter, tskIDLE_PRIORITY + 1, NULL) == pdPASS);
        Led_Blink_Queue[iter] = xQueueCreate(1, sizeof(uint32_t));
        assert_param(Led_Blink_Queue[iter] != 0 );
    }
}
