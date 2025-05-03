/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation
 * Copyright (c) Marco Milenkovic 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG

 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** Thread-Metric Component                                               */
/**                                                                       */
/** Application Interface (API)                                           */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#ifndef TM_API_H
#define TM_API_H
#include "rtos_config.h"

#ifdef USING_ZEPHYR
#include <cmsis_core.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/drivers/interrupt_controller/intc_vim.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/syscalls/libc-hooks.h>

#endif /* USING_ZEPHYR */
#ifdef USING_FREERTOS
#include "ti_board_open_close.h"
#include "ti_drivers_open_close.h"
#include <drivers/uart.h>
#include <kernel/dpl/DebugP.h>
#endif /* USING_FREERTOS */
#ifdef USING_THREADX
#include "ti_board_open_close.h"
#include "ti_drivers_open_close.h"
#include "tx_api.h"
#include <drivers/uart.h>
#include <kernel/dpl/DebugP.h>

#ifndef TX_DISABLE_ERROR_CHECKING
#define TX_DISABLE_ERROR_CHECKING
#endif
#define TX_ENABLE_STACK_CHECKING

#endif /* USING_THREADX */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef __cplusplus
/* Yes, C++ compiler is present.  Use standard C.  */
extern "C"
{
#endif

   /* Define API constants.  */

#define TM_SUCCESS 0
#define TM_ERROR 1

#ifdef USING_ZEPHYR
#define printf printk
#endif /* USING_ZEPHYR */
#ifdef USING_FREERTOS
#define printf DebugP_log
#endif /* USING_FREERTOS */
#ifdef USING_THREADX
#define printf DebugP_log
#endif /* USING_THREADX */

/* Define the time interval in seconds. This can be changed with a -D compiler option.  */
#ifndef TM_TEST_DURATION
#define TM_TEST_DURATION 30
#endif
/* Define the size used by the queue create function ensure that the maximum queue size macros can hold the size*/
#ifndef MESSAGE_SIZE
#define MESSAGE_SIZE 4 /* N x 4 Byte  integers. WARNING: USE 4 when using the Thread Metric Message Processing Test */
#endif

   /* Prototype of the test functions */
   int tm_main(void);
   int tm_main_one(void);
   int tm_main_two(void);
   int tm_main_three(void);
   int tm_main_four(void);
   int tm_main_five(void);
   int tm_main_six(void);
   int tm_main_seven(void);
   int main_sync(void);
   int main_inheritance(void);
   int main_pmu(void);
   int main_message_isr_test(void);
   int main_message_task_test(void);
   int main_specific_sync(void);
   int main_critical_section_test(void);
   int main_thread_locking_test(void);
   int main_uart(void);
   int main_memory_throughput_test(void);

   /* Define RTOS Neutral APIs. RTOS vendors should fill in the guts of the
      following API. Once this is done the Thread-Metric tests can be
      successfully run.  */

   /* This function called from main performs basic RTOS initialization,
   calls the test initialization function, and then starts the RTOS function.
   */
   void tm_initialize(void (*test_initialization_function)(void));
   /* This function takes a thread ID and priority and attempts to create the
   file in the underlying RTOS.  Valid priorities range from 1 through 31,
   where 1 is the highest priority and 31 is the lowest. If successful,
   the function should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
   */
   int tm_thread_create(int thread_id, int priority, void (*entry_function)(void*, void*, void*));
   int tm_thread_create_param(int thread_id, int priority, void (*entry_function)(void*, void*, void*), void* param);
   /* This function resumes the specified thread.  If successful, the function
   should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
   int tm_thread_resume(int thread_id);
   /* This function suspends the specified thread.  If successful, the function
   should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
   int tm_thread_suspend(int thread_id);
   /* This function relinquishes to other ready threads at the same
   priority.  */
   void tm_thread_relinquish(void);
   /* Only really needed in FreeRTOS as Threads are not allowed to return */
   void tm_thread_exit(int thread_id);
   /* This function suspends the specified thread for the specified number
   of seconds.  If successful, the function should return TM_SUCCESS.
   Otherwise, TM_ERROR should be returned.  */
   void tm_thread_sleep(int seconds);
   void tm_thread_sleep_ticks(int ticks);
   /* This function creates the specified queue.  If successful, the function
   should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
   int tm_queue_create(int queue_id);
   /* This function sends a 16-byte message to the specified queue.  If successful,
   the function should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
   */
   int tm_queue_send(int queue_id, unsigned long* message_ptr);
   /* This function sends a 16-byte message to the specified queue.  If successful,
   the function should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
   */
   int tm_queue_send_from_isr(int queue_id, unsigned long* message_ptr);
   /* This function receives a 16-byte message from the specified queue.  If
   successful, the function should return TM_SUCCESS. Otherwise, TM_ERROR should be
   returned.  */
   int tm_queue_receive(int queue_id, unsigned long* message_ptr);
   /* This function creates the specified semaphore.  If successful, the function
   should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
   int tm_semaphore_create(int semaphore_id);
   /* This function gets the specified semaphore.  If successful, the function
   should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
   int tm_semaphore_get(int semaphore_id);

   int tm_semaphore_wait(int semaphore_id);
   /* This function puts the specified semaphore.  If successful, the function
   should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
   int tm_semaphore_put(int semaphore_id);
   int tm_semaphore_put_from_isr(int semaphore_id);
   /* This function creates the specified memory pool that can support one or more
   allocations of 128 bytes.  If successful, the function should
   return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
   int tm_memory_pool_create(int pool_id);
   /* This function allocates a 128 byte block from the specified memory pool.
   If successful, the function should return TM_SUCCESS. Otherwise, TM_ERROR
   should be returned.  */
   int tm_memory_pool_allocate(int pool_id, unsigned char** memory_ptr);
   /* This function releases a previously allocated 128 byte block from the
   specified memory pool. If successful, the function should return TM_SUCCESS.
   Otherwise, TM_ERROR should be returned.  */
   int tm_memory_pool_deallocate(int pool_id, unsigned char* memory_ptr);
   /* This function creates the specified mutex.  If successful, the function
     should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
   int tm_mutex_create(int mutex_id);
   /* This function gets the specified mutex.  If successful, the function
     should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
   int tm_mutex_get(int mutex_id);
   /* This function puts the specified mutex.  If successful, the function
      should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
   int tm_mutex_put(int mutex_id);
   /* Forward declaration of the ISR callback. */
   void tm_timer_isr(void* args);
   /* This function initializes the timer module. */
   void tm_time_init(void);
   /* This function creates the specified event flags group.  If successful,
   the function should return TM_SUCCESS. Otherwise, TM_ERROR should be
   returned.  */
   unsigned long tm_time_get(void);
   /* Configures and initializes the PMU on R5F Core */
   int tm_setup_pmu(void);
   /* Start PMU profiling */
   void tm_pmu_profile_start(const char* name);
   /* End PMU profiling */
   void tm_pmu_profile_end(const char* name);
   /* Print PMU profiling results */
   void tm_pmu_profile_print(const char* name);
   /* Get the priority of the current thread */
   int tm_task_priority_get(int thread_id);
   /* Set the handle and block the current thread */
   int rtos_sync_wait();
   /* Take the handle and unblock the waiting thread */
   int rtos_sync_signal();
   /* Initialize the RTOS specificmechanism for synchronization */
   void init_rtos_sync();
   /* Enter the criticial Section (disable tick Interrupts) */
   void tm_enter_critical_section();
   /* Exit the criticial Section (enable tick Interrupts) */
   void tm_exit_critical_section();
   /* Suspending the scheduler and locking to current thread */
   void tm_suspend_scheduler(void);
   /* Resuming the scheduler */
   void tm_resume_scheduler(void);
   /* Low latency print function */
   void lowLatencyPrint(const char* format, ...);
   /* Calculate the time per iteration */
   // double calculate_iteration_time(unsigned long time_sec, unsigned long count_diff);

   /* APIs for interrupt handling */
   void tm_interrupt_raise();
   void tm_interrupt_processing_handler();
   void tm_interrupt_preemption_handler();
   void tm_isr_message_handler(void);

/* Determine if a C++ compiler is being used.  If so, complete the standard
   C conditional started above.  */
#ifdef __cplusplus
}
#endif

#endif
