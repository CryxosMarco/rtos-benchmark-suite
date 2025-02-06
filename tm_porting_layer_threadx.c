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
/**   Porting Layer (ThreadX Example)                                     */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/* Turn off ThreadX error checking.  */
#include "rtos_config.h"
#ifdef USING_THREADX

#ifndef TX_DISABLE_ERROR_CHECKING
#define TX_DISABLE_ERROR_CHECKING
#endif

/* For smallest size, the ThreadX library and application code should be built
   with the following options defined (easiest to add in tx_port.h):

#define TX_ENABLE_EXECUTION_CHANGE_NOTIFY
#define TX_DISABLE_PREEMPTION_THRESHOLD
#define TX_DISABLE_NOTIFY_CALLBACKS
#define TX_DISABLE_REDUNDANT_CLEARING
#define TX_DISABLE_STACK_FILLING
#define TX_NOT_INTERRUPTABLE
#define TX_TIMER_PROCESS_IN_ISR

  For the fastest performance, these additional options should also be used:

#define TX_REACTIVATE_INLINE
#define TX_INLINE_THREAD_RESUME_SUSPEND

*/

/* Include necessary files.  */

#include "tm_api.h"
#include "tx_api.h"

#include "ti_drivers_config.h"
#include <drivers/pmu.h>
#include <kernel/dpl/TimerP.h>

/* Define ThreadX mapping constants.  */

#define TM_THREADX_MAX_THREADS 10
#define TM_THREADX_MAX_QUEUES 4
#define TM_THREADX_MAX_SEMAPHORES 4
#define TM_THREADX_MAX_MEMORY_POOLS 4

/* Define the default ThreadX stack size.  */

// #define TM_THREADX_THREAD_STACK_SIZE 1048
#define TM_THREADX_THREAD_STACK_SIZE 2096 // Increase to a higher value

/* Define the default ThreadX queue size.  */

#define TM_THREADX_QUEUE_SIZE 200

/* Define the default ThreadX memory pool size.  */

#define TM_THREADX_MEMORY_POOL_SIZE 2048

/* Define the number of timer interrupt ticks per second.  */

#define TM_THREADX_TICKS_PER_SECOND 1000

/* Define ThreadX data structures.  */

TX_THREAD tm_thread_array[TM_THREADX_MAX_THREADS];
TX_QUEUE tm_queue_array[TM_THREADX_MAX_QUEUES];
TX_SEMAPHORE tm_semaphore_array[TM_THREADX_MAX_SEMAPHORES];
TX_BLOCK_POOL tm_block_pool_array[TM_THREADX_MAX_MEMORY_POOLS];
/* Define an array of mutexes. */
TX_MUTEX tm_mutex_array[TM_THREADX_MAX_SEMAPHORES];

/* Define ThreadX object data areas.  */

unsigned char tm_thread_stack_area[TM_THREADX_MAX_THREADS * TM_THREADX_THREAD_STACK_SIZE];
unsigned char tm_queue_memory_area[TM_THREADX_MAX_QUEUES * TM_THREADX_QUEUE_SIZE];
unsigned char tm_pool_memory_area[TM_THREADX_MAX_MEMORY_POOLS * TM_THREADX_MEMORY_POOL_SIZE];

/* Global time counter incremented by the timer ISR */
static volatile uint64_t g_timeCounter = 0ULL;

/* Define array to remember the test entry function.  */

void* tm_thread_entry_functions[TM_THREADX_MAX_THREADS];

/* Define our shell entry function to match ThreadX.  */

VOID tm_thread_entry(ULONG thread_input);

/* This function called from main performs basic RTOS initialization,
   calls the test initialization function, and then starts the RTOS function. */
void tm_initialize(void (*test_initialization_function)(void))
{
   /* Call initialization function. */
   test_initialization_function();
}

/* This function takes a thread ID and priority and attempts to create the
   file in the underlying RTOS.  Valid priorities range from 1 through 31,
   where 1 is the highest priority and 31 is the lowest. If successful,
   the function should return TM_SUCCESS. Otherwise, TM_ERROR should be
   returned.   */
int tm_thread_create(int thread_id, int priority, void (*entry_function)(void*, void*, void*))
{

   UINT status;

   /* Remember the actual thread entry.  */
   tm_thread_entry_functions[thread_id] = (void*) entry_function;

   /* Create the thread under ThreadX.  */
   status =
      tx_thread_create(&tm_thread_array[thread_id], "Thread-Metric test", tm_thread_entry, (ULONG) thread_id,
                       &tm_thread_stack_area[thread_id * TM_THREADX_THREAD_STACK_SIZE], TM_THREADX_THREAD_STACK_SIZE,
                       (UINT) priority, (UINT) priority, TX_NO_TIME_SLICE, TX_DONT_START);

   /* Determine if the thread create was successful.  */
   if (status == TX_SUCCESS)
      return (TM_SUCCESS);
   else
      return (TM_ERROR);
}

/* This function resumes the specified thread.  If successful, the function
   should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_thread_resume(int thread_id)
{

   UINT status;

   /* Attempt to resume the thread.  */
   status = tx_thread_resume(&tm_thread_array[thread_id]);

   /* Determine if the thread resume was successful.  */
   if (status == TX_SUCCESS)
      return (TM_SUCCESS);
   else
      return (TM_ERROR);
}

/* This function suspends the specified thread.  If successful, the function
   should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_thread_suspend(int thread_id)
{

   UINT status;

   /* Attempt to suspend the thread.  */
   status = tx_thread_suspend(&tm_thread_array[thread_id]);

   /* Determine if the thread suspend was successful.  */
   if (status == TX_SUCCESS)
      return (TM_SUCCESS);
   else
      return (TM_ERROR);
}

/* This function relinquishes to other ready threads at the same
   priority.  */
void tm_thread_relinquish(void)
{

   /* Relinquish to other threads at the same priority.  */
   tx_thread_relinquish();
}

/* This function suspends the specified thread for the specified number
   of seconds.  If successful, the function should return TM_SUCCESS.
   Otherwise, TM_ERROR should be returned.  */
void tm_thread_sleep(int seconds)
{
   if (seconds == 0)
   {
      /* Sleep for 1 Tick */
      tx_thread_sleep((UINT) 2);
   }
   /* Attempt to sleep.  */
   tx_thread_sleep(((UINT) seconds) * TM_THREADX_TICKS_PER_SECOND);
}

/* This function creates the specified queue.  If successful, the function
   should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_queue_create(int queue_id)
{

   UINT status;

   /* Create the specified queue with 16-byte messages.  */
   status = tx_queue_create(&tm_queue_array[queue_id], "Thread-Metric test", TX_4_ULONG,
                            &tm_queue_memory_area[queue_id * TM_THREADX_QUEUE_SIZE], TM_THREADX_QUEUE_SIZE);

   /* Determine if the queue create was successful.  */
   if (status == TX_SUCCESS)
      return (TM_SUCCESS);
   else
      return (TM_ERROR);
}

/* This function sends a 16-byte message to the specified queue.  If successful,
   the function should return TM_SUCCESS. Otherwise, TM_ERROR should be
   returned.  */
int tm_queue_send(int queue_id, unsigned long* message_ptr)
{

   UINT status;

   /* Send the 16-byte message to the specified queue.  */
   status = tx_queue_send(&tm_queue_array[queue_id], message_ptr, TX_NO_WAIT);

   /* Determine if the queue send was successful.  */
   if (status == TX_SUCCESS)
      return (TM_SUCCESS);
   else
      return (TM_ERROR);
}

/* This function receives a 16-byte message from the specified queue.  If
   successful, the function should return TM_SUCCESS. Otherwise, TM_ERROR should
   be returned.  */
int tm_queue_receive(int queue_id, unsigned long* message_ptr)
{

   UINT status;

   /* Receive a 16-byte message from the specified queue.  */
   status = tx_queue_receive(&tm_queue_array[queue_id], message_ptr, TX_NO_WAIT);

   /* Determine if the queue receive was successful.  */
   if (status == TX_SUCCESS)
      return (TM_SUCCESS);
   else
      return (TM_ERROR);
}

/* This function creates the specified semaphore.  If successful, the function
   should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_semaphore_create(int semaphore_id)
{

   UINT status;

   /*  Create semaphore.  */
   status = tx_semaphore_create(&tm_semaphore_array[semaphore_id], "Thread-Metric test", 1);

   /* Determine if the semaphore create was successful.  */
   if (status == TX_SUCCESS)
      return (TM_SUCCESS);
   else
      return (TM_ERROR);
}

/* This function gets the specified semaphore.  If successful, the function
   should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_semaphore_get(int semaphore_id)
{

   UINT status;

   /*  Get the semaphore.  */
   status = tx_semaphore_get(&tm_semaphore_array[semaphore_id], TX_NO_WAIT);

   /* Determine if the semaphore get was successful.  */
   if (status == TX_SUCCESS)
      return (TM_SUCCESS);
   else
      return (TM_ERROR);
}

/* This function puts the specified semaphore.  If successful, the function
   should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_semaphore_put(int semaphore_id)
{

   UINT status;

   /*  Put the semaphore.  */
   status = tx_semaphore_put(&tm_semaphore_array[semaphore_id]);

   /* Determine if the semaphore put was successful.  */
   if (status == TX_SUCCESS)
      return (TM_SUCCESS);
   else
      return (TM_ERROR);
}

/* This function waits for the specified semaphore. If successful, the function
   should return TM_SUCCESS. Otherwise, TM_ERROR should be returned*/
int tm_semaphore_wait(int semaphore_id)
{
   UINT status;

   /* Wait for the semaphore. */
   status = tx_semaphore_get(&tm_semaphore_array[semaphore_id], TX_WAIT_FOREVER);

   /* Return appropriate status. */
   return (status == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

/* Mutex create function. */
int tm_mutex_create(int mutex_id)
{
   UINT status;

   /* Create the mutex with priority inheritance. */
   status = tx_mutex_create(&tm_mutex_array[mutex_id], "Thread-Metric Mutex", TX_INHERIT);

   /* Return appropriate status. */
   return (status == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

/* Mutex get function. */
int tm_mutex_get(int mutex_id)
{
   UINT status;

   /* Acquire the mutex. */
   status = tx_mutex_get(&tm_mutex_array[mutex_id], TX_WAIT_FOREVER);

   /* Return appropriate status. */
   return (status == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

/* Mutex put function. */
int tm_mutex_put(int mutex_id)
{
   UINT status;

   /* Release the mutex. */
   status = tx_mutex_put(&tm_mutex_array[mutex_id]);

   /* Return appropriate status. */
   return (status == TX_SUCCESS) ? TM_SUCCESS : TM_ERROR;
}

/* This function creates the specified memory pool that can support one or more
   allocations of 128 bytes.  If successful, the function should
   return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_memory_pool_create(int pool_id)
{

   UINT status;

   /*  Create the memory pool.  */
   status =
      tx_block_pool_create(&tm_block_pool_array[pool_id], "Thread-Metric test", 128,
                           &tm_pool_memory_area[pool_id * TM_THREADX_MEMORY_POOL_SIZE], TM_THREADX_MEMORY_POOL_SIZE);

   /* Determine if the block pool memory was successful.  */
   if (status == TX_SUCCESS)
      return (TM_SUCCESS);
   else
      return (TM_ERROR);
}

/* This function allocates a 128 byte block from the specified memory pool.
   If successful, the function should return TM_SUCCESS. Otherwise, TM_ERROR
   should be returned.  */
int tm_memory_pool_allocate(int pool_id, unsigned char** memory_ptr)
{

   UINT status;

   /*  Allocate a 128-byte block from the specified memory pool.  */
   status = tx_block_allocate(&tm_block_pool_array[pool_id], (void**) memory_ptr, TX_NO_WAIT);

   /* Determine if the block pool allocate was successful.  */
   if (status == TX_SUCCESS)
      return (TM_SUCCESS);
   else
      return (TM_ERROR);
}

/* This function releases a previously allocated 128 byte block from the
   specified memory pool. If successful, the function should return TM_SUCCESS.
   Otherwise, TM_ERROR should be returned.  */
int tm_memory_pool_deallocate(int pool_id, unsigned char* memory_ptr)
{

   UINT status;

   /*  Release the 128-byte block back to the specified memory pool.  */
   status = tx_block_release((void*) memory_ptr);

   /* Determine if the block pool release was successful.  */
   if (status == TX_SUCCESS)
      return (TM_SUCCESS);
   else
      return (TM_ERROR);
}

/* This is the ThreadX thread entry.  It is going to call the Thread-Metric
   entry function saved earlier.  */
VOID tm_thread_entry(ULONG thread_input)
{

   void (*entry_function)(void);

   /* Pickup the entry function from the saved array.  */
   entry_function = (void (*)(void)) tm_thread_entry_functions[thread_input];

   /* Call the entry function.   */
   (entry_function)();
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

#endif /* USING_THREADX */
