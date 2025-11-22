# STM32 FreeRTOS CLI Project

## Project Overview

A modular Command Line Interface (CLI) firmware framework built on STM32F407 using FreeRTOS.
Designed to demonstrate real-time task management, non-blocking communication, runtime diagnostics, and clean embedded software architecture.

This project highlights skills essential for embedded roles:
- RTOS-based design
- Driver abstraction
- Modular command parsing
- Debugging with SystemView
- CPU profiling using FreeRTOS runtime stats

---

## 🧩 Features

- 🧠 **FreeRTOS-based multitasking**
- 💡 **LED blink control** using dedicated tasks
- 🔀 **UART-driven CLI** with command parsing and argument handling
- 🎲 **Random data generation task**
- 🧮 **CPU usage monitoring** via SystemView and runtime stats
- ⚙️ **Modular command registration system** for easy extension

---

## 🧱 Architecture Overview

📌 High-Level Design
```
UART ISR → RX Queue → CLI Task → Command Dispatcher → Subsystem (LED, RNG, CPU)
```

📁 Project Structure

```
/Core
 ├── app/
 │     ├── cli/          # CLI task, parser, command registry
 │     ├── tasks/        # LED task, RNG task, CPU monitor task
 │     └── utils/        # Helpers, message formatting
 ├── drivers/
 │     ├── uart/         # Non-blocking UART driver using IRQ
 │     ├── led/          # LED abstraction layer
 │     └── rng/          # Random data generator
 └── freertos/
       ├── freertos.c    # Task creation, hooks, timers
       └── stats.c       # Runtime CPU statistics provider
```

🧠 Key Embedded Concepts Demonstrated

- Non-blocking UART RX using interrupts + FreeRTOS queues
- Task-safe command execution
- Real-time scheduling with vTaskDelay, priorities, and tick hooks
- Custom CLI engine with command dispatch table
- Accurate CPU load measurement using run-time stats timer
- Debugging with SEGGER SystemView timeline
- Clean modular code separation for scalability

---

## ⚙️ Hardware Setup

| Peripheral | Description |
|-------------|-------------|
| MCU | STM32F407VGT6 Discovery |
| UART | USART1 (TX: PB6, RX: PA10) |
| UART | Baudrate - 115200 |
| LED | Onboard LEDs (PD12–PD15) |
| Debugger | ST-Link V2 |
| Toolchain | STM32CubeIDE / SEGGER SystemView |

---

## 🧵 Tasks Overview

| Task Name | Function | Notes |
|------------|-----------|-------|
| **LED Task** | Controls LED blink timing | Blink rate adjustable via CLI |
| **CLI Task** | Handles UART input and command parsing | Non-blocking, uses FreeRTOS queues |
| **Rand Task** | Generates pseudo-random data | For demonstration only |
| **CPU Monitor Task** | Collects and reports CPU stats | Uses FreeRTOS runtime stats |
| **Idle Task** | System idle loop | Enters low-power mode |

---

## 💻 CLI Commands

| Command | Description | Arguments | Example |
|----------|--------------|------------|----------|
| **list** | Lists all available commands | None | `list` |
| **uart** | Configure UART parameters | `<baud>` | `uart 115200 ` |
| **set_blink_rate** | Set LED blink interval (ms) | `<rate_ms>` | `set_blink_rate 500` |
| **set_blink_rate** | Set LED blink interval (ms) | `<led colour> <rate_ms>` | `set_blink_rate blue 500` |
| **rand_data** | Generate and print random data | `<length>` | `rand_data 16` |
| **update** | Enter firmware update mode *(not implemented)* | None | `update` |
| **cpu_monitor** | Show CPU usage and task stats | `<once or continue>` | `cpu_monitor` |

---

## 📊 CPU Monitoring

The `cpu_monitor` command prints FreeRTOS runtime statistics such as:

`>>>> cpu_monitor once`

|Task       | CPU%   | Free Stack (words) |
|----------|--------|--------------------|
|CLI Task   |   0.02 |               174 |
|IDLE       |  99.96 |               106 |
|Red Led T  |   0.00 |                58 |
|Orange Le  |   0.00 |                58 |
|Green Led  |   0.00 |                58 |
|Blue Led   |   0.00 |                58 |
|Setting T  |   0.00 |               407 |

---
