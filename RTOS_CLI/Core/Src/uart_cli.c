/*
 * uart_cli.c
 *
 *  Created on: May 30, 2025
 *      Author: Ashish Bansal
 */

/* -- Standard Library -- */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>

/* -- STM32 Library -- */
#include "usart.h"
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_rng.h"
#include "rng.h"

/* -- FreeRTOS Library -- */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "SEGGER_RTT.h"

/* -- User Library -- */
#include "uart_cli.h"
#include "tasks.h"
#include "settings_task.h"

/* -- Extern Variables -- */
extern UART_HandleTypeDef hUSART1;
extern volatile xQueueHandle User_Uart_Queue;
extern volatile xQueueHandle SettingsQueue;
extern xTaskHandle CpuMonitorHandler;
extern RNG_HandleTypeDef hrng;
extern const char COLOR_NAMES[LED_COUNT][10];

/* -- Function Declarations -- */

/* Returns a pointer to the static buffer containing the user’s input command. */
static inline char* get_command_input(void);

/* Transmit a single byte over the CLI UART using polling. */
static inline void cli_tx_byte(const uint8_t);


/* Compare user input to registered commands and invoke the matching handler. */
static void command_handler(const char*);

/* Extracts the first integer found in `src_string`, returns it, and sets `length` to number of characters consumed. */
long extract_number_from_string(const char *src_string, size_t *length);

/* Print a list of all registered CLI commands and their descriptions. */
static inline void list_commands(const char*);

/* Generate and print a random number (possibly within a user-specified range). */
void rand_data(const char*);

/* Parse LED color names and blink rate from `Arguments`, then queue a settings change. */
void set_blink_rate(const char*);

/* Display CPU usage and free stack of all tasks, either once or continuously until ENTER is pressed. */
void cpu_monitor(const char*);

/* Handle UART Settings */
void uart_settings(const char*);

/* -- Global Variables -- */

const static CliStruct Command_Handlers[] = {
    { .command = "list",           .handler = list_commands, .privilege_level = ALL,   .description = "List all commands" },
    { .command = "uart",           .handler = uart_settings, .privilege_level = GUEST, .description = "Do uart settings from here" },
    { .command = "set_blink_rate", .handler = set_blink_rate,.privilege_level = GUEST, .description = "Set LED Blink Speed" },
    { .command = "rand_data",      .handler = rand_data,     .privilege_level = GUEST, .description = "Generate Random Data" },
    { .command = "update",         .handler = NULL,          .privilege_level = ROOT,  .description = "Should Put Device in update mode" },
    { .command = "cpu_monitor",    .handler = cpu_monitor,   .privilege_level = ALL,   .description = "Prints CPU Stats" },
};

static const char* CPU_USAGE_Commands[] = { "once", "continue" };

/* -------------------------------------------------------------------------- */
/*                          Static Inline Function Group                      */
/* -------------------------------------------------------------------------- */

/* -- CLI UART TX Byte -- */
/* Transmit a single byte over the CLI UART using polling. */
static inline void cli_tx_byte(const uint8_t byte)
{
    while (!LL_USART_IsActiveFlag_TXE(USART1))
        ;
    LL_USART_TransmitData8(USART1, byte);
}

/* -- CLI Print String -- */
/* Print a null-terminated string over the CLI UART by sending each byte. */
void cli_print(const char *ptr)
{
    const size_t len = strlen(ptr);
    for (size_t i = 0; i < len; ++i)
    {
        cli_tx_byte(ptr[i]);
    }
}

/* -- CLI Print Fixed Length String -- */
/* Print a buffer of length `len` over the CLI UART by sending each byte. */
void cli_printn(const char *ptr, const size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        cli_tx_byte(ptr[i]);
    }
}

/* -- Get Command Input from User -- */
/* Reads characters from the CLI queue until ENTER is pressed, handling backspace, and returns the resulting command string. */
static inline char* get_command_input(void)
{
    char received_char = 0;
    static char received_command[MAX_CMD_LEN] = { 0 };
    size_t index = 0;

    cli_print("\r>>>> ");

    do
    {
        if (User_Uart_Queue != NULL)
            xQueueReceive(User_Uart_Queue, &received_char, portMAX_DELAY);

        switch (received_char)
        {
            case BACK_SPACE:
                cli_tx_byte(BACK_SPACE);
                received_command[index--] = '\0';
                break;

            case ENTER:
                received_command[index] = '\0';
                cli_print("\r\n");
                break;

            default:
                cli_tx_byte(received_char);
                received_command[index++] = received_char;
                break;
        }

    } while (received_char != ENTER);

    return received_command;
}

/* -- List All Registered Commands -- */
/* Iterates through `Command_Handlers` and prints each command name and description. */
static inline void list_commands(const char*)
{
    for (uint8_t iter = 0; iter < TOTAL_COMMANDS; ++iter)
    {
        cli_printf("%d. %-10s: %s %s\r\n",
                   iter + 1,
                   Command_Handlers[iter].command,
                   Command_Handlers[iter].description,
                   Command_Handlers[iter].handler == NULL ? "(Not Implemented)" : "");
    }
}

/* -------------------------------------------------------------------------- */
/*                              CLI Logic Functions                           */
/* -------------------------------------------------------------------------- */

/* -- CLI Task -- */
/* FreeRTOS task that waits for a full command via UART interrupts and then dispatches it to the command handler. */
void Cli_Task(void *Arguments)
{
    MX_USART1_UART_Init();

    LL_USART_EnableIT_RXNE(USART1);
    NVIC_SetPriority(USART1_IRQn, 6);
    NVIC_EnableIRQ(USART1_IRQn);

    while (1)
    {
        command_handler(get_command_input());
    }
}

/* -- Extract Number from String -- */
/* Scans `src_string` for the first digit sequence, converts it to a long, and returns it.
 * Sets `length` to number of chars consumed. */
long extract_number_from_string(const char *src_string, size_t *length)
{
    long number = __LONG_MAX__;
    const char *follower = src_string;
    char *endpointer = NULL;

    while (*follower != 0)
    {
        follower++;
        if (isdigit((int)*follower))
        {
            number = strtol(follower, &endpointer, 10);
            *length = endpointer - src_string;
            break;
        }
    }

    return number;
}

/* -- Generate a 32-bit Random Number from RNG Peripheral -- */
/* Waits for RNG data-ready flag and returns the next random value. */
uint32_t random_gen(void)
{
    while (!LL_RNG_IsActiveFlag_DRDY(RNG))
        ;
    return LL_RNG_ReadRandData32(RNG);
}

/* -- Generate Random Data Command -- */
/* Parses optional range arguments, generates a random value, and prints it. */
void rand_data(const char *Arguments)
{
    long min = 0, max = 1;
    long random_number = 0;

    if (Arguments != NULL)
    {
        size_t len = 0;
        min = extract_number_from_string(Arguments, &len);
        max = extract_number_from_string(Arguments + len, &len);

        if (min == __LONG_MAX__ || max == __LONG_MAX__)
        {
            cli_printf("Unexpected Error %lX %lX\r\n", min, max);
            return;
        }
        else if (min > max)
        {
            cli_print("Why the fuck is min greater than max value\r\n");
            return;
        }

        random_number = random_gen() % (max - min) + min;
    }
    else
    {
        random_number = random_gen();
    }

    cli_printf("%ld\r\n", random_number);
}

/* -- Set Blink Rate Command -- */
/* Parses color names and blink rate from `Arguments` then sends appropriate settings to `SettingsQueue`. */
void set_blink_rate(const char *Arguments)
{
    uint8_t ColorFlag = 0;
    size_t ptr_indx = 0;
    uint32_t blink_rate = 0;
    const size_t arg_len = strlen(Arguments);

    Settings blink_settings = { .config_id = LED_CONFIG, .Buffer = { 0 } };

    if (isdigit((int)Arguments[1]))
    {
        ColorFlag = (1 << BLUE) | (1 << RED) | (1 << ORANGE) | (1 << GREEN);
        blink_settings.Buffer[0] = ColorFlag;
    }
    else
    {
        while (ptr_indx < arg_len && !isdigit((int)Arguments[ptr_indx]))
        {
            while (Arguments[ptr_indx] == ' ')
            {
                ptr_indx++;
            }

            const size_t prev_index = ptr_indx;

            for (uint8_t i = 0; i < LED_COUNT; ++i)
            {
                if (strncasecmp(Arguments + ptr_indx, COLOR_NAMES[i], strlen(COLOR_NAMES[i])) == 0)
                {
                    blink_settings.Buffer[0] |= 1 << i;
                    ptr_indx += strlen(COLOR_NAMES[i]) + 1;
                }
            }

            if (prev_index == ptr_indx)
            {
                break;
            }
        }
    }

    blink_rate = strtol(Arguments + ptr_indx, NULL, 10);
    memcpy(blink_settings.Buffer + sizeof(blink_settings.Buffer[0]), &blink_rate, sizeof(blink_rate));

    for (COLOR color = BLUE; color < LED_COUNT; color++)
    {
        if (ColorFlag & (1 << color))
        {
            blink_settings.Buffer[0] |= 1 << color;
        }
    }

    xQueueSend(SettingsQueue, &blink_settings, portMAX_DELAY);
}


/* -- CPU Monitor Command -- */
/* Retrieves runtime stats for each FreeRTOS task and prints CPU usage and free stack.
 * If "continue" is passed, updates in place until ENTER is pressed. */
void cpu_monitor(const char* arguments)
{
    uint8_t command_type = 0;
    if (arguments == NULL)
    {
        cli_print("Please pass arguments\r\n");
        return;
    }

    uint8_t index = 0;

    while(arguments[index] == ' ')
    {
        index++;
    }

    if (strcasecmp(CPU_USAGE_Commands[0], arguments + index) == 0)
    {
        command_type = 1;  // once
    }
    else if (strcasecmp(CPU_USAGE_Commands[1], arguments + index) == 0)
    {
        cli_print("Press Enter to stop\r\n");
        command_type = 2;  // continue
    }
    else
    {
        cli_print("Invalid argument. Use \"once\" or \"continue\"\r\n");
        return;
    }

    uint32_t total_run_time = 0;
    uint8_t  total_tasks = 0;
    BaseType_t TaskIter = 0;

    char received_char = 0;
    TaskStatus_t task_status[TOTAL_TASKS_TO_WATCH] = { 0 };

    cli_printf("%-10s | %-6s | %-17s |\r\n", "Task", "CPU%", "Free Stack (words)");
    cli_print("------------------------------------------------------\r\n");

    cli_print("\033[?25l"); // Hide cursor

    do
    {
        total_tasks = uxTaskGetSystemState(task_status, TOTAL_TASKS_TO_WATCH, &total_run_time);

        for (TaskIter = 0; TaskIter < total_tasks; ++TaskIter)
        {
            const float cpu_percentage = (task_status[TaskIter].ulRunTimeCounter * 100.0f) / total_run_time;
            cli_printf("%-10s | %6.2f | %17d |\r\n",
                            task_status[TaskIter].pcTaskName,
                            cpu_percentage,
                            task_status[TaskIter].usStackHighWaterMark);
        }

        if (command_type == 2) // continuous
        {
            xQueueReceive(User_Uart_Queue, &received_char, 1000);
            cli_printf("\033[%dA\033[2K\r", total_tasks); // Move cursor up and clear line
        }

    } while (command_type == 2 && received_char != ENTER);

    cli_print("\033[?25h\r\n"); // Show cursor again
}

static inline uint8_t is_baudrate_valid(const uint32_t BaudRate)
{
    uint8_t valid = 0;

    switch(BaudRate)
    {
        case 1200:
        case 4800:
        case 9600:
        case 19200:
        case 115200:
        case 460800:
        case 920600:
            valid = 1;
            break;
        default:
    }

    return valid;
}

void uart_settings(const char* Arguments)
{
    if(NULL == Arguments)
    {
        cli_print("Please provide BaudRate\r\n");
        return;
    }
    char* endptr = NULL;

    uint32_t NewBaudRate = strtol(Arguments, &endptr, 10);

    if(0 == NewBaudRate || is_baudrate_valid(NewBaudRate) != 1)
    {
        cli_print("Please provide Valid BaudRate\r\n");
        return;
    }

    huart1.Init.BaudRate = NewBaudRate;
    HAL_UART_Init(&huart1);
}

/* -- Handle Parsed Command -- */
/* Compares `user_input` against registered commands and invokes the corresponding handler, passing any arguments. */
static void command_handler(const char *user_input)
{
    if(*user_input == '\0') // <! User just pressed enter without any input
    {
        return;
    }

    uint8_t i = 0;
    const size_t total = sizeof(Command_Handlers) / sizeof(Command_Handlers[0]);

    for (i = 0; i < total; ++i)
    {
        const char *cmd = Command_Handlers[i].command;

        if (strncasecmp(user_input, cmd, strlen(cmd)) == 0)
        {
            if (Command_Handlers[i].handler != NULL)
            {
                const char *Arguments = (user_input[strlen(cmd)] == '\0') ? NULL : strchr(user_input, ' ');
                Command_Handlers[i].handler(Arguments);
            }
            else
            {
                cli_print("Not handled\r\n");
            }
            return;
        }
    }

    if (i == total)
    {
        cli_printf("%s cmd not found\r\n", user_input);
    }
}

/* -- Formatted Print to CLI -- */
/* Formats a string using vsprintf and sends it via `cli_print`. Returns number of chars printed. */
size_t cli_printf(const char *format, ...)
{
    char print_buffer[100];
    va_list args;
    va_start(args, format);
    size_t len = vsprintf(print_buffer, format, args);
    va_end(args);

    cli_print(print_buffer);
    return len;
}

/* -------------------------------------------------------------------------- */
/*                            UART IRQ Handler                                */
/* -------------------------------------------------------------------------- */

/* -- USART1 Interrupt Service Routine -- */
/* Called when a new byte arrives on USART1. Reads the byte and places it into the CLI queue. */
void USART1_IRQHandler(void)
{
    uint8_t received = 0xff;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Check if RXNE (Receive Data Register Not Empty) flag is active
    // and the RXNE interrupt is enabled
    if (LL_USART_IsActiveFlag_RXNE(USART1) && LL_USART_IsEnabledIT_RXNE(USART1))
    {
        // Also check for potential error flags (optional but recommended)
        if (!LL_USART_IsActiveFlag_FE(USART1) &&  // Framing Error
            !LL_USART_IsActiveFlag_NE(USART1) &&  // Noise Error
            !LL_USART_IsActiveFlag_ORE(USART1))   // Overrun Error
        {
            received = LL_USART_ReceiveData8(USART1);
            if (User_Uart_Queue != NULL)
            {
                xQueueSendFromISR(User_Uart_Queue, &received, &xHigherPriorityTaskWoken);
            }
        }
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
