/*[CR]**************************************************************************
Copyright (c) Marco Milenkovic 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
SPDX-License-Identifier: Apache-2.0
*/
/*[FH]**************************************************************************
PROJECT: MASTER THESIS
MODULE: Priority Inheritance Test
*/
/*[CL]**************************************************************************
 * Message Queue Thread to Thread Benchmark
 *
 * This program measures the delay due to priority inheritance over a fixed
 * number of inversion cycles (ITERATION_COUNT). PMU measurement keys are precomputed,
 * and the low‑priority task busy-waits—calling tm_task_priority_get()—until its effective
 * priority changes from its nominal value (LOW_TASK_PRIO). At that point it stops the PMU,
 * records the cycle count, and releases the mutex.
 *
 * The high‑priority task triggers the inversion and increments a counter each cycle.
 * The medium‑priority (interference) task continuously runs until all cycles are complete,
 * at which point it prints the PMU measurements and total inversion count.
 *
********************************************************************************/

/*******************************************************************************
 * includes
 ******************************************************************************/
#include "tm_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Number of inversion cycles (measurements) */
#define ITERATION_COUNT 32

#define PMU_KEY_LEN 8
/* Global arrays for precomputed PMU keys and measured cycle counts */
char pmu_names[ITERATION_COUNT][PMU_KEY_LEN];

/* Global counter for inversion cycles (should equal ITERATION_COUNT at test end) */
volatile unsigned long inversion_count = 0;

/*******************************************************************************
 * Task Priority and IDs
 ******************************************************************************/
#define SHARED_MUTEX_ID 1

#define HIGH_TASK_ID 0
#define HIGH_TASK_PRIO 5 /* Highest priority */

#define MED_TASK_ID 1
#define MED_TASK_PRIO 10 /* Medium priority */

#define LOW_TASK_ID 2
#define LOW_TASK_PRIO 20 /* Nominal low priority */

/*******************************************************************************
 * Debug Macro, use this to enable or diable the DBG_PRINT function
 * This must be deactivated for the benchmarking!
 ******************************************************************************/
// #define DEBUG_PRIO_INHERITANCE_ON
#ifdef DEBUG_PRIO_INHERITANCE_ON
#define DBG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DBG_PRINT(fmt, ...)                                                                                            \
   do                                                                                                                  \
   {                                                                                                                   \
   } while (0)
#endif

/*******************************************************************************
 * Low Priority Task
 *
 * For each cycle:
 *  1. Acquire the shared mutex.
 *  2. Start the PMU measurement using the precomputed key.
 *  3. Enter a busy loop calling tm_task_priority_get(LOW_TASK_ID) until its effective
 *     priority is no longer LOW_TASK_PRIO.
 *  4. Stop the PMU measurement, record the cycle count, and release the mutex.
 *  5. Sleep briefly before the next cycle.
 ******************************************************************************/
static void LowPrioTask(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;

   for (int i = 0; i < ITERATION_COUNT; i++)
   {
      DBG_PRINT("[LowPrioTask] Cycle %d: Attempting to acquire mutex.\r\n", i);
      if (tm_mutex_get(SHARED_MUTEX_ID) == TM_SUCCESS)
      {
         DBG_PRINT("[LowPrioTask] Cycle %d: Mutex acquired.\r\n", i);

         /* Busy loop until effective priority changes from the nominal value. */
         while (tm_task_priority_get(LOW_TASK_ID) == LOW_TASK_PRIO)
         {
            // tm_pmu_profile_start(pmu_names[i]);
            __asm__ volatile("nop");
         }

         // tm_pmu_profile_end(pmu_names[i]);

         DBG_PRINT("[LowPrioTask] Cycle %d: Inheritance detected. Releasing mutex.\r\n", i);
         tm_mutex_put(SHARED_MUTEX_ID);

         inversion_count++; /* Completed inversion cycle */
      }
      else
      {
         DBG_PRINT("[LowPrioTask] Cycle %d: Failed to acquire mutex.\r\n", i);
      }
      tm_thread_sleep_ticks(1);
   }
   DBG_PRINT("[LowPrioTask] All cycles complete. Suspending task.\r\n");
   tm_thread_suspend(LOW_TASK_ID);
}

/*******************************************************************************
 * High Priority Task
 *
 * For each cycle:
 *  1. Sleep briefly to allow LowPrioTask to acquire the mutex and start its busy loop.
 *  2. Attempt to acquire the mutex (which blocks until LowPrioTask releases it).
 *  3. Release the mutex and sleep briefly before the next cycle.
 *
 * (The inversion cycle is measured by LowPrioTask.)
 ******************************************************************************/
static void HighPrioTask(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;
   for (int i = 0; i < ITERATION_COUNT; i++)
   {
      DBG_PRINT("[HighPrioTask] Cycle %d: Sleeping briefly before attempting mutex.\r\n", i);
      tm_thread_sleep_ticks(15);
      DBG_PRINT("[HighPrioTask] Cycle %d: Attempting to acquire mutex.\r\n", i);
      tm_pmu_profile_start(pmu_names[i]);
      if (tm_mutex_get(SHARED_MUTEX_ID) == TM_SUCCESS)
      {
         DBG_PRINT("[HighPrioTask] Cycle %d: Mutex acquired. Releasing mutex again.\r\n", i);
         tm_pmu_profile_end(pmu_names[i]);
         tm_mutex_put(SHARED_MUTEX_ID);
      }
      else
      {
         DBG_PRINT("[HighPrioTask] Cycle %d: Failed to acquire mutex.\r\n", i);
      }
      tm_thread_sleep_ticks(5);
   }
   DBG_PRINT("[HighPrioTask] All cycles complete. Suspending task.\r\n");
   tm_thread_suspend(HIGH_TASK_ID);
}

/*******************************************************************************
 * Medium Priority Task (Interference & Final Results)
 *
 * This task continuously runs interference. Once all inversion cycles are complete,
 * it prints the PMU measurement results and the total inversion count.
 ******************************************************************************/
static void MedPrioTask(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;
   while (1)
   {
      DBG_PRINT("[MedPrioTask] Interference active.\r\n");

      for (int i = 0; i < 1000; i++)
      {
         __asm__ volatile("nop");
      }
      tm_thread_sleep_ticks(2);

      /* Check if all cycles have been completed */
      if (inversion_count >= ITERATION_COUNT)
      {
         /* Allow a short delay for the other tasks to finish */
         tm_thread_sleep_ticks(10);
         printf("\r\n*** Test Complete ***\r\n");
         printf("Total inversion cycles completed: %lu\r\n", inversion_count);
         for (int i = 0; i < ITERATION_COUNT; i++)
         {
            tm_pmu_profile_print(pmu_names[i]);
         }
         DBG_PRINT("[MedPrioTask] Test ended. Suspending task.\r\n");
         tm_thread_exit(MED_TASK_ID);
         break;
      }
   }
}

/*******************************************************************************
 * Test Initialization
 *
 * Precomputes PMU keys, creates the mutex and tasks, and starts the test.
 ******************************************************************************/
static void tm_priority_inheritance_initialize(void)
{
   int i;
   tm_setup_pmu();
   tm_mutex_create(SHARED_MUTEX_ID);

   /* Precompute PMU keys (e.g. "INV00", "INV01", ... "INV29") */
   for (i = 0; i < ITERATION_COUNT; i++)
   {
      snprintf(pmu_names[i], PMU_KEY_LEN, "INV%02d", i);
   }

   tm_thread_create(LOW_TASK_ID, LOW_TASK_PRIO, LowPrioTask);
   tm_thread_create(MED_TASK_ID, MED_TASK_PRIO, MedPrioTask);
   tm_thread_create(HIGH_TASK_ID, HIGH_TASK_PRIO, HighPrioTask);

   tm_thread_resume(LOW_TASK_ID);
   tm_thread_resume(MED_TASK_ID);
   tm_thread_resume(HIGH_TASK_ID);

   printf("[Init] Priority Inheritance detection test started for %d cycles.\r\n", ITERATION_COUNT);
}

/*******************************************************************************
 * Main Entry Point
 *
 * Initializes the RTOS and starts the test.
 ******************************************************************************/
int main_inheritance(void)
{
   tm_initialize(tm_priority_inheritance_initialize);
   return 0;
}
