/*
 * uart_cli.h
 *
 *  Created on: May 31, 2025
 *      Author: Ashish Bansal
 */

#ifndef INC_UART_CLI_H_
#define INC_UART_CLI_H_

#define TOTAL_TASKS_TO_WATCH (20)
#define MAX_CMD_LEN (100)

#define BACK_SPACE  0x7F
#define ENTER       0x0D
#define CAR_RET     '\r'

typedef enum
{
    GUEST = 1,
    USER,
    ROOT,
    ALL,
}PRIVILAGE_LEVEL;

#define TOTAL_COMMANDS  ( sizeof(Command_Handlers) / sizeof(Command_Handlers[0]) )

typedef void (*cmd_handler)(const char*);

typedef struct cli
{
    char* command;
    cmd_handler handler;
    PRIVILAGE_LEVEL privilege_level;
    char* description;
}CliStruct;

typedef struct
{
    uint8_t* data;
    size_t len;
}ParamConfig;


/* Print a null-terminated string over the CLI UART by sending each byte. */
void cli_print(const char *ptr);

/* Print a buffer of length `len` over the CLI UART by sending each byte. */
void cli_printn(const char *ptr, const size_t);

/* Format and send a string (like printf) over the CLI UART. */
size_t cli_printf(const char *format, ...);

#endif /* INC_UART_CLI_H_ */
