Copyright (c) Marco Milenkovic 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
SPDX-License-Identifier: Apache-2.0



# Benchmark_RTOS

This repository contains the source code used to benchmark synchronization primitives and interrupt behavior across different Real-Time Operating Systems (RTOSes).

---

## RTOS Selection

To choose the RTOS for benchmarking, edit the `rtos_config.h` file. This controls which RTOS-specific implementation and configuration will be compiled.

---

## Entry Points

Each RTOS has its own `main_<rtosname>.c` file that serves as the entry point for the benchmark application.

You will need to manually adjust the benchmark function and interrupt handler names for each RTOS. For example:

```c
void main_task(void* pvParameters)
{
   /* Start Thread-Metric tests */
   printk("Starting Thread-Metric tests...\r\n");

   /* Initialize custom interrupts */
   test_interrupt_handler = tm_isr_message_handler;
   setup_interrupt();

   /* Call the main Thread-Metric function */
   main_critical_section_test();

   /* Delete thread after completion */
   k_thread_abort(k_current_get());
}
```

Adjust the function `main_critical_section_test()` and `tm_isr_message_handler` to suit your specific benchmark scenario.

---

## RTOS Abstraction

The file `api.h` together with the RTOS-specific implementation (e.g., `porting_layer_<rtos>.c`) provides the abstraction layer. All RTOS-dependent functionality—such as thread creation, synchronization, and interrupt setup—is handled there.

This makes it easy to extend the benchmark to new RTOSes by implementing the necessary API functions defined in `api.h`.

---

## Structure Overview

```
Benchmark_RTOS/
├── rtos_config.h                # Select active RTOS
├── api.h                        # Abstract API header
├── porting_layer_zephyr.c       # Zephyr-specific implementation
├── porting_layer_freertos.c     # FreeRTOS-specific implementation
├── porting_layer_threadx.c      # ThreadX-specific implementation
├── main_zephyr.c                # Zephyr benchmark entry point
├── main_freertos.c              # FreeRTOS benchmark entry point
├── main_threadx.c               # ThreadX benchmark entry point
└── ...                          # Additional tests, utilities, etc.
```

---

## Notes

- All benchmark logic is RTOS-agnostic and controlled via the abstraction in `api.h`.
- Interrupts must be correctly configured per RTOS using the appropriate `setup_interrupt()` logic.
- Benchmark cases (e.g., Thread-Metric style tests) are grouped in standalone functions like `main_critical_section_test()`.

---
