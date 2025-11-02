# STM32 FreeRTOS CLI Project

A simple yet powerful **Command Line Interface (CLI)** running on **STM32F407** using **FreeRTOS**.  
This project demonstrates task management, UART-based command interaction, and real-time performance analysis using SEGGER SystemView.

---

## 🧩 Features

- 🧠 **FreeRTOS-based multitasking**
- 💡 **LED blink control** using dedicated tasks
- 🔀 **UART-driven CLI** with command parsing and argument handling
- 🎲 **Random data generation task**
- 🧮 **CPU usage monitoring** via SystemView and runtime stats
- ⚙️ **Modular command registration system** for easy extension

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

>>>> cpu_monitor once

|Task       | CPU%   | Free Stack (words) |
|----------|--------|--------------------|
|CLI Task   |   0.02 |               174 |
|IDLE       |  99.96 |               106 |
|Red Led T  |   0.00 |                58 |
|Orange Le  |   0.00 |                58 |
|Green Led  |   0.00 |                58 |
|Blue Led   |   0.00 |                58 |
|Setting T  |   0.00 |               407 |
