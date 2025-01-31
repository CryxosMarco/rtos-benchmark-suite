/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation
 *
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
/**   Porting Layer (Must be completed with RTOS specifics)               */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/
#include "rtos_config.h"
#ifdef USING_FREERTOS

/* Include necessary files.  */

#include "ti_drivers_config.h"
#include "tm_api.h"
#include <drivers/pmu.h>
#include <kernel/dpl/TimerP.h>

/* Kernel includes */
#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>

/* Global time counter incremented by the timer ISR */
static volatile uint64_t g_timeCounter = 0ULL;

/* Define FreeRTOS mapping constants. */
#define TM_FREERTOS_MAX_THREADS 10
#define TM_FREERTOS_MAX_QUEUES 1
#define TM_FREERTOS_MAX_SEMAPHORES 2

/* Define FreeRTOS data structures. */
TaskHandle_t tm_thread_array[TM_FREERTOS_MAX_THREADS];
QueueHandle_t tm_queue_array[TM_FREERTOS_MAX_QUEUES];
SemaphoreHandle_t tm_semaphore_array[TM_FREERTOS_MAX_SEMAPHORES];
SemaphoreHandle_t tm_mutex_array[TM_FREERTOS_MAX_SEMAPHORES];

/* This function called from main performs basic RTOS initialization,
   calls the test initialization function, and then starts the RTOS function.  */
void tm_initialize(void (*test_initialization_function)(void))
{
   /* Call initialization function. */
   test_initialization_function();
   /* Enter the FreeRTOS kernel. */
   // vTaskStartScheduler();
}

/* This function takes a thread ID and priority and attempts to create the
   file in the underlying RTOS.  Valid priorities range from 1 through 31,
   where 1 is the highest priority and 31 is the lowest. If successful,
   the function should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.   */
int tm_thread_create(int thread_id, int priority, void (*entry_function)(void*, void*, void*))
{
   int new_priority = configMAX_PRIORITIES - priority + 1;
   BaseType_t status;

   configASSERT(new_priority <= (configMAX_PRIORITIES - 1));
   status = xTaskCreate(entry_function, "Thread-Metric test", configMINIMAL_STACK_SIZE, NULL,
                        /*priority*/ new_priority, &tm_thread_array[thread_id]);

   if (status != pdPASS)
   {
      return TM_ERROR;
   }
   // vTaskSuspend(tm_thread_array[thread_id]);
   /* threads start active */

   // printf("Creating thread ID: %d, Priority: %d\n", thread_id, priority);
   return TM_SUCCESS;
}

/* This function resumes the specified thread.  If successful, the function should
   return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_thread_resume(int thread_id)
{
   // printf("Resuming thread ID: %d\n", thread_id);
   vTaskResume(tm_thread_array[thread_id]);
   return TM_SUCCESS;
}

/* This function suspends the specified thread.  If successful, the function should
   return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_thread_suspend(int thread_id)
{
   vTaskSuspend(tm_thread_array[thread_id]);

   return TM_SUCCESS;
}

/* This function relinquishes to other ready threads at the same
   priority.  */
void tm_thread_relinquish(void)
{
   taskYIELD();
}

/* Terminates the current thread and removes it from the scheduler */
void tm_thread_exit(void)
{
   // Ensure the function is called from a valid FreeRTOS task context
   if (xTaskGetCurrentTaskHandle() != NULL)
   {
      // Delete the current task
      vTaskDelete(NULL);
   }
   else
   {
      // Print an error if this is called outside a task context
      printf("Error: tm_thread_exit called outside of a task context\n");
      // Optionally loop indefinitely or trigger a system error
      for (;;)
      {
      }
   }
}

/* This function suspends the specified thread for the specified number
   of seconds.  If successful, the function should return TM_SUCCESS.
   Otherwise, TM_ERROR should be returned.  */
void tm_thread_sleep(int seconds)
{
   vTaskDelay((seconds * 1000U) / portTICK_RATE_MS);
}

/* This function creates the specified queue.  If successful, the function should
   return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_queue_create(int queue_id)
{
   tm_queue_array[queue_id] = xQueueCreate(10, 4 * sizeof(int32_t));

   if (tm_queue_array[queue_id] == NULL)
   {
      return TM_ERROR;
   }

   return TM_SUCCESS;
}

/* This function sends a 16-byte message to the specified queue.  If successful,
   the function should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_queue_send(int queue_id, unsigned long* message_ptr)
{
   BaseType_t status;

   status = xQueueSendToBack(tm_queue_array[queue_id], (const void*) message_ptr, (TickType_t) 0);

   if (status != pdTRUE)
   {
      return TM_ERROR;
   }

   return TM_SUCCESS;
}

/* This function receives a 16-byte message from the specified queue.  If successful,
   the function should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_queue_receive(int queue_id, unsigned long* message_ptr)
{
   BaseType_t status;

   status = xQueueReceive(tm_queue_array[queue_id], (void* const) message_ptr, (TickType_t) 0);

   if (status != pdTRUE)
   {
      return TM_ERROR;
   }

   return TM_SUCCESS;
}

/* This function creates the specified semaphore.  If successful, the function should
   return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_semaphore_create(int semaphore_id)
{
   tm_semaphore_array[semaphore_id] = xSemaphoreCreateBinary();

   if (tm_semaphore_array[semaphore_id] == NULL)
   {
      return TM_ERROR;
   }

   /* so it starts available */
   return tm_semaphore_put(semaphore_id);
}

/* This function gets the specified semaphore.  If successful, the function should
   return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_semaphore_get(int semaphore_id)
{
   BaseType_t status;

   status = xSemaphoreTake(tm_semaphore_array[semaphore_id], (TickType_t) 0);

   if (status != pdTRUE)
   {
      return TM_ERROR;
   }

   return TM_SUCCESS;
}

/* This function waits the specified semaphore.  If successful, the function should
   return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_semaphore_wait(int semaphore_id)
{
   BaseType_t status;

   status = xSemaphoreTake(tm_semaphore_array[semaphore_id], portMAX_DELAY);

   if (status != pdTRUE)
   {
      return TM_ERROR;
   }

   return TM_SUCCESS;
}

/* This function puts the specified semaphore.  If successful, the function should
   return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_semaphore_put(int semaphore_id)
{
   BaseType_t status;

   status = xSemaphoreGive(tm_semaphore_array[semaphore_id]);

   if (status != pdTRUE)
   {
      return TM_ERROR;
   }

   return TM_SUCCESS;
}

/* This function puts the specified semaphore.  If successful, the function should
   return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_semaphore_put_from_isr(int semaphore_id)
{
   BaseType_t yield_required;
   BaseType_t status;

   status = xSemaphoreGiveFromISR(tm_semaphore_array[semaphore_id], &yield_required);

   if (status != pdTRUE)
   {
      return TM_ERROR;
   }

   portYIELD_FROM_ISR(yield_required);

   return TM_SUCCESS;
}

int tm_mutex_create(int mutex_id)
{
   if (mutex_id < 0 || mutex_id >= TM_FREERTOS_MAX_SEMAPHORES)
   {
      printf("Invalid mutex ID: %d. Max allowed: %d\n", mutex_id, TM_FREERTOS_MAX_SEMAPHORES - 1);
      return TM_ERROR;
   }

   tm_mutex_array[mutex_id] = xSemaphoreCreateMutex();

   if (tm_mutex_array[mutex_id] == NULL)
   {
      printf("Failed to create mutex for ID %d\n", mutex_id);
      return TM_ERROR;
   }

   return TM_SUCCESS;
}

/* Mutex lock function. */
int tm_mutex_get(int mutex_id)
{
   BaseType_t status;

   /* Attempt to lock the mutex (wait indefinitely). */
   status = xSemaphoreTake(tm_mutex_array[mutex_id], portMAX_DELAY);
   /* Return appropriate status. */
   return (status == pdTRUE) ? TM_SUCCESS : TM_ERROR;
}

/* Mutex unlock function. */
int tm_mutex_put(int mutex_id)
{
   BaseType_t status;

   /* Attempt to release the mutex. */
   status = xSemaphoreGive(tm_mutex_array[mutex_id]);
   /* Return appropriate status. */
   return (status == pdTRUE) ? TM_SUCCESS : TM_ERROR;
}

/* This function creates the specified memory pool that can support one or more
   allocations of 128 bytes.  If successful, the function should
   return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_memory_pool_create(int pool_id)
{
   return TM_SUCCESS;
}

/* This function allocates a 128 byte block from the specified memory pool.
   If successful, the function should return TM_SUCCESS. Otherwise, TM_ERROR
   should be returned.  */
int tm_memory_pool_allocate(int pool_id, unsigned char** memory_ptr)
{
   *memory_ptr = pvPortMalloc(128);

   if (*memory_ptr == NULL)
   {
      return TM_ERROR;
   }

   return TM_SUCCESS;
}

/* This function releases a previously allocated 128 byte block from the specified
   memory pool. If successful, the function should return TM_SUCCESS. Otherwise, TM_ERROR
   should be returned.  */
int tm_memory_pool_deallocate(int pool_id, unsigned char* memory_ptr)
{
   vPortFree(memory_ptr);

   return TM_SUCCESS;
}

/*******************************************************************************
 * tm_time_init
 *
 *  - Configures and starts TimerP in a known mode (periodic or continuous).
 *  - Ties the ISR callback (tm_time_isr) to increment the global time counter.
 ******************************************************************************/
void tm_time_init(void)
{
   /* Start the timer */
   TimerP_start(gTimerBaseAddr[CONFIG_TIMER0]);

   DebugP_log("tm_time_init: Timer started.\r\n");
}

/*******************************************************************************
 * tm_time_isr
 *
 *  - The ISR callback invoked by TimerP on each timer interrupt.
 *  - Increments our global counter, which tm_time_get() will read.
 *
 *  NOTE: The frequency of increments = (timer frequency).
 *        So if the timer triggers every 1 ms, g_timeCounter increments every 1
 *ms.
 ******************************************************************************/
void tm_timer_isr(void* args)
{
   (void) args;
   g_timeCounter++;
}

/*******************************************************************************
 * tm_time_get
 *
 *  - Part of the tm_api.h specification: returns an unsigned long (32 bits).
 *  - Here we retrieve the lower 32 bits of our 64-bit counter.
 *  - If your tests need a longer timescale, you could return 64 bits or handle
 *    overflow logic differently.
 *
 *  NOTE: If the timer triggers every 1 ms, tm_time_get() returns "milliseconds
 *        since timer started" (lower 32 bits).
 *        You can scale or interpret as needed (e.g., microseconds, ticks).
 ******************************************************************************/
unsigned long tm_time_get(void)
{
   /* Return the lower 32 bits */
   return (unsigned long) (g_timeCounter & 0xFFFFFFFFUL);
}

/*-----------------------------------------------------------
 * Performance Monitoring Unit (PMU) Configuration
 *-----------------------------------------------------------
 * This configuration defines events to be monitored using the
 * ARM R5 PMU. The selected events track instruction and data
 * cache activity, providing insights into cache performance.
 *-----------------------------------------------------------*/
PMU_EventCfg gPmuEventCfg[3] = {
   {
      .name = "ICache Miss", // Tracks instruction cache misses
      .type = CSL_ARM_R5_PMU_EVENT_TYPE_ICACHE_MISS,
   },
   {
      .name = "DCache Access", // Tracks data cache accesses
      .type = CSL_ARM_R5_PMU_EVENT_TYPE_DCACHE_ACCESS,
   },
   {
      .name = "DCache Miss", // Tracks data cache misses
      .type = CSL_ARM_R5_PMU_EVENT_TYPE_DCACHE_MISS,
   },
};

/*-----------------------------------------------------------
 * Global PMU Configuration
 *-----------------------------------------------------------
 * Enables cycle counting and sets up event counters.
 * The PMU will track the three configured events.
 *-----------------------------------------------------------*/
PMU_Config gPmuConfig = {
   .bCycleCounter = TRUE,         // Enables cycle counter
   .numEventCounters = 3U,        // Number of event counters
   .eventCounters = gPmuEventCfg, // Pointer to event configuration
};

/*-----------------------------------------------------------
 * Initialize Performance Monitoring Unit (PMU)
 *-----------------------------------------------------------
 * This function initializes the PMU with the configured
 * event counters. It must be called before profiling begins.
 *-----------------------------------------------------------*/
int tm_setup_pmu(void)
{
   PMU_init(&gPmuConfig);
   return 1;
}

/*-----------------------------------------------------------
 * Start PMU Profiling
 *-----------------------------------------------------------
 * Begins profiling for a specific section of code.
 * @param name: Identifier for the profiling session.
 *-----------------------------------------------------------*/
void tm_pmu_profile_start(const char* name)
{
   PMU_profileStart(name);
}

/*-----------------------------------------------------------
 * End PMU Profiling
 *-----------------------------------------------------------
 * Stops profiling for the specified session.
 * @param name: Identifier for the profiling session.
 *-----------------------------------------------------------*/
void tm_pmu_profile_end(const char* name)
{
   PMU_profileEnd(name);
}

/*-----------------------------------------------------------
 * Print PMU Profiling Results
 *-----------------------------------------------------------
 * Displays the recorded profiling data for the given session.
 * @param name: Identifier for the profiling session.
 *-----------------------------------------------------------*/
void tm_pmu_profile_print(const char* name)
{
   PMU_profilePrintEntry(name);
}

#endif /* USING_FREERTOS */
