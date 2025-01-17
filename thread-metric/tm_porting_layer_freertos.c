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

/* Include necessary files.  */

#include "tm_api.h"

/* Kernel includes */
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

/* Define FreeRTOS mapping constants. */
#define TM_FREERTOS_MAX_THREADS 10
#define TM_FREERTOS_MAX_QUEUES 1
#define TM_FREERTOS_MAX_SEMAPHORES 1

/* Define FreeRTOS data structures. */
TaskHandle_t tm_thread_array[TM_FREERTOS_MAX_THREADS];
QueueHandle_t tm_queue_array[TM_FREERTOS_MAX_QUEUES];
SemaphoreHandle_t tm_semaphore_array[TM_FREERTOS_MAX_SEMAPHORES];

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
int tm_thread_create(int thread_id, int priority, void (*entry_function)(void))
{
    int new_priority = configMAX_PRIORITIES - priority + 1;
    BaseType_t status;

    configASSERT(new_priority <= (configMAX_PRIORITIES - 1));
    status = xTaskCreate(entry_function, "Thread-Metric test",
                         configMINIMAL_STACK_SIZE, NULL, /*priority*/ new_priority,
                         &tm_thread_array[thread_id]);

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
int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
    BaseType_t status;

    status = xQueueSendToBack(tm_queue_array[queue_id], (const void *)message_ptr, (TickType_t)0);

    if (status != pdTRUE)
    {
        return TM_ERROR;
    }

    return TM_SUCCESS;
}

/* This function receives a 16-byte message from the specified queue.  If successful,
   the function should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
    BaseType_t status;

    status = xQueueReceive(tm_queue_array[queue_id], (void *const)message_ptr, (TickType_t)0);

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

    status = xSemaphoreTake(tm_semaphore_array[semaphore_id], (TickType_t)0);

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

/* Define an array of mutexes. */
SemaphoreHandle_t tm_mutex_array[TM_FREERTOS_MAX_SEMAPHORES];

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
int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
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
int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
    vPortFree(memory_ptr);

    return TM_SUCCESS;
}

// #include <kernel/dpl/TimerP.h>
// #include "ti_drivers_config.h"
// extern volatile unsigned long overflow_counter;

// /* This function returns the number of ticks to estimate a time*/
// unsigned long tm_time_get(void)
// {
//     static uint32_t last_timer_value = 0;
//     uint32_t current_timer_value = TimerP_getCount(gTimerBaseAddr[CONFIG_TIMER0]);

//     // Check if the timer has overflowed
//     if (TimerP_isOverflowed(gTimerBaseAddr[CONFIG_TIMER0]))
//     {
//         TimerP_clearOverflowInt(gTimerBaseAddr[CONFIG_TIMER0]);
//         overflow_counter++;
//     }

//     // Calculate the total time, considering overflows
//     uint64_t total_ticks = ((uint64_t)overflow_counter << 32) | current_timer_value;

//     // Ensure `last_timer_value` is updated
//     last_timer_value = current_timer_value;

//     return (uint32_t)(total_ticks & 0xFFFFFFFF);
// }
