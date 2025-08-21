/*
 * uart_task.h
 *
 *  Created on: Jun 5, 2025
 *      Author: Ashish Bansal
 */

#ifndef INC_UART_TASK_H_
#define INC_UART_TASK_H_

#include <stdint.h>

typedef enum
{
    WAITING_CLI,
    WAITING_CPU_MONITORING,
}WaitingTaskEnum;

extern WaitingTaskEnum WaitingTask;

#endif /* INC_UART_TASK_H_ */
