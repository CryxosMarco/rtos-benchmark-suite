/*[CR]******************************************************************************
 *   Priority Inheritance Test Program (30 Iterations with Debug Output)
 *
 * Copyright (c) 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 *
 * This program demonstrates how three tasks of different priorities (High,
 * Medium, Low) share a mutex-protected resource to trigger and observe
 * priority inheritance. Debug prints (toggled via DEBUG_PRIO_INHERITANCE_ON)
 * help validate the execution sequence.
 ******************************************************************************/

/*******************************************************************************
 * includes
 ******************************************************************************/
#include "tm_api.h"
#include <stdio.h>
#include <stdlib.h>

#define ITERATION_COUNT 5
/* Adjust this value to fine-tune the delay */
#define LOOP_COUNT 1000000
/* Uncomment the following line to enable debug prints in critical sections */
#define DEBUG_PRIO_INHERITANCE_ON
#ifdef DEBUG_PRIO_INHERITANCE_ON
#define DBG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DBG_PRINT(fmt, ...)                                                                                            \
   do                                                                                                                  \
   {                                                                                                                   \
   } while (0)
#endif

/* Precomputed PMU name arrays for start and end measurements */
char pmu_profile_iter[ITERATION_COUNT][16];

/* Define a unique mutex ID for our shared resource */
#define SHARED_MUTEX_ID 1

/* Define Task IDs and their priorities */
#define HIGH_TASK_ID 0
#define HIGH_TASK_PRIO 5 /* High priority */

#define MED_TASK_ID 1
#define MED_TASK_PRIO 10 /* Medium priority */

#define LOW_TASK_ID 2
#define LOW_TASK_PRIO 20 /* Low priority */

/*******************************************************************************
 * Low Priority Task
 *
 * This task acquires the mutex first and simulates a long critical section.
 * It prints debug messages before/after the busy loop when debugging is enabled.
 ******************************************************************************/
static void LowPrioTask(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;
   for (int i = 0; i < ITERATION_COUNT; i++)
   {
      DBG_PRINT("[LowPrioTask] Iteration %d: Attempting to acquire mutex.\n", i);
      if (tm_mutex_get(SHARED_MUTEX_ID) == TM_SUCCESS)
      {
         DBG_PRINT("[LowPrioTask] Iteration %d: Mutex acquired, starting long work.\n", i);
         /* Critical busy loop: no prints here for accurate timing */
         for (volatile unsigned long j = 0; j < LOOP_COUNT; j++)
         {
            __asm__ volatile("nop");
         }
         DBG_PRINT("[LowPrioTask] Iteration %d: Work done, releasing mutex.\n", i);
         tm_mutex_put(SHARED_MUTEX_ID);
      }
      else
      {
         DBG_PRINT("[LowPrioTask] Iteration %d: Failed to acquire mutex.\n", i);
      }
      /* Sleep to allow other tasks to run and interfere if needed */
      tm_thread_sleep(1);
   }
   DBG_PRINT("[LowPrioTask] All iterations complete. Suspending task.\n");
   tm_thread_suspend(LOW_TASK_ID);
}

/*******************************************************************************
 * High Priority Task
 *
 * This task waits briefly (letting the low task acquire the mutex) then starts
 * PMU measurement, attempts to acquire the mutex, and stops the measurement when
 * the mutex is obtained. It prints debug messages to show the intended sequence.
 ******************************************************************************/
static void HighPrioTask(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;
   for (int i = 0; i < ITERATION_COUNT; i++)
   {
      DBG_PRINT("[HighPrioTask] Iteration %d: Starting iteration. Sleeping briefly...\n", i);
      tm_thread_sleep(0); // Allow LowPrioTask to run first

      DBG_PRINT("[HighPrioTask] Iteration %d: Initiating PMU measurement and attempting to acquire mutex.\n", i);
      tm_pmu_profile_start(pmu_profile_iter[i]);

      if (tm_mutex_get(SHARED_MUTEX_ID) == TM_SUCCESS)
      {
         DBG_PRINT("[HighPrioTask] Iteration %d: Mutex acquired. Ending PMU measurement.\n", i);
         tm_pmu_profile_end(pmu_profile_iter[i]);
         DBG_PRINT("[HighPrioTask] Iteration %d: Reporting PMU result.\n", i);
         tm_pmu_profile_print(pmu_profile_iter[i]);
         tm_mutex_put(SHARED_MUTEX_ID);
      }
      else
      {
         DBG_PRINT("[HighPrioTask] Iteration %d: Failed to acquire mutex.\n", i);
      }
      /* Additional sleep to allow interference from MedPrioTask */
      tm_thread_sleep(1);
   }
   DBG_PRINT("[HighPrioTask] All iterations complete. Test finished.\n");
   tm_thread_suspend(HIGH_TASK_ID);
}

/*******************************************************************************
 * Medium Priority Task
 *
 * This task runs concurrently to simulate interference. It prints debug messages
 * to help you verify its execution relative to the high and low tasks.
 ******************************************************************************/
static void MedPrioTask(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;
   /* Run interference longer than the other tasks to ensure overlapping activity */
   for (int i = 0; i < ITERATION_COUNT * 2; i++)
   {
      DBG_PRINT("[MedPrioTask] Iteration %d: Running interference.\n", i);
      tm_thread_sleep(1);
   }
   DBG_PRINT("[MedPrioTask] Interference complete. Suspending task.\n");
   tm_thread_suspend(MED_TASK_ID);
}

/*******************************************************************************
 * Priority Inheritance Test Initialization
 *
 * Sets up the PMU, precomputes the PMU names, creates the mutex, creates the tasks,
 * and then resumes them.
 ******************************************************************************/
static void tm_priority_inheritance_initialize(void)
{
   int i;
   tm_setup_pmu();

   /* Precompute PMU names for each iteration */
   for (i = 0; i < ITERATION_COUNT; i++)
   {
      snprintf(pmu_profile_iter[i], sizeof(pmu_profile_iter[i]), "S%02d", i);
   }

   tm_mutex_create(SHARED_MUTEX_ID);

   /* Create tasks */
   tm_thread_create(LOW_TASK_ID, LOW_TASK_PRIO, LowPrioTask);
   tm_thread_create(MED_TASK_ID, MED_TASK_PRIO, MedPrioTask);
   tm_thread_create(HIGH_TASK_ID, HIGH_TASK_PRIO, HighPrioTask);

   /* Resume tasks */
   tm_thread_resume(LOW_TASK_ID);
   tm_thread_resume(MED_TASK_ID);
   tm_thread_resume(HIGH_TASK_ID);

   printf("[Init] Priority Inheritance test started for %d iterations.\n", ITERATION_COUNT);
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
