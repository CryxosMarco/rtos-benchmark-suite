/*[CR]******************************************************************************
 *   Priority Inheritance Benchmark Test Program (Baseline vs Inheritance)
 *
 * Copyright (c) 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 *
 * This program demonstrates how three tasks of different priorities (High,
 * Medium, Low) share a mutex-protected resource to trigger and observe priority
 * inheritance. Two modes are supported:
 *
 *   - Baseline: The low‑priority task releases the mutex immediately.
 *   - Inheritance: The low‑priority task holds the mutex for a fixed delay (using sleep)
 *     to simulate a critical section.
 *
 * The high‑priority task measures the blocking delay via the PMU. With proper
 * priority inheritance, the delay in inheritance mode should be approximately
 * equal to the fixed hold delay, even with interference.
 ******************************************************************************/

/*******************************************************************************
 * includes
 ******************************************************************************/
#include "tm_api.h"
#include <stdio.h>
#include <stdlib.h>

/* Number of iterations for the test */
#define ITERATION_COUNT 5

/* Fixed delay for low-priority task in inheritance mode (in ticks) */
#define CRITICAL_SECTION_DELAY_TICKS 1

/* Uncomment the following line to enable debug prints */
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
 * Test Mode Selection
 *
 * MODE_BASELINE: Low-priority task releases the mutex immediately.
 * MODE_INHERITANCE: Low-priority task holds the mutex for a fixed delay.
 ******************************************************************************/
typedef enum
{
   MODE_BASELINE,
   MODE_INHERITANCE
} test_mode_t;

/* Select the mode for the test */
volatile test_mode_t current_test_mode = MODE_INHERITANCE;

/*******************************************************************************
 * PMU Profile Names (one per iteration)
 ******************************************************************************/
char pmu_names[ITERATION_COUNT][16];

/*******************************************************************************
 * Mutex and Task Definitions
 ******************************************************************************/
#define SHARED_MUTEX_ID 1

#define HIGH_TASK_ID 0
#define HIGH_TASK_PRIO 5 /* High priority */

#define MED_TASK_ID 1
#define MED_TASK_PRIO 10 /* Medium priority */

#define LOW_TASK_ID 2
#define LOW_TASK_PRIO 20 /* Low priority */

/*******************************************************************************
 * Low Priority Task
 *
 * In MODE_INHERITANCE, this task holds the mutex for a fixed time.
 * In MODE_BASELINE, it releases immediately.
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
         DBG_PRINT("[LowPrioTask] Iteration %d: Mutex acquired.\n", i);
         if (current_test_mode == MODE_INHERITANCE)
         {
            DBG_PRINT("[LowPrioTask] Iteration %d: Holding mutex for %d ticks.\n", i, CRITICAL_SECTION_DELAY_TICKS);
            tm_thread_sleep_ticks(CRITICAL_SECTION_DELAY_TICKS);
         }
         else
         {
            DBG_PRINT("[LowPrioTask] Iteration %d: Releasing mutex immediately (baseline).\n", i);
         }
         DBG_PRINT("[LowPrioTask] Iteration %d: Mutex released.\n", i);
         tm_mutex_put(SHARED_MUTEX_ID);
      }
      else
      {
         DBG_PRINT("[LowPrioTask] Iteration %d: Failed to acquire mutex.\n", i);
      }
      tm_thread_sleep_ticks(100);
   }
   DBG_PRINT("[LowPrioTask] All iterations complete. Suspending task.\n");
   tm_thread_suspend(LOW_TASK_ID);
}

/*******************************************************************************
 * High Priority Task
 *
 * This task measures how long it is blocked waiting for the mutex.
 * It starts a PMU measurement, attempts to acquire the mutex, stops the
 * measurement once the mutex is obtained, prints the result, and then releases the mutex.
 ******************************************************************************/
static void HighPrioTask(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;
   for (int i = 0; i < ITERATION_COUNT; i++)
   {
      DBG_PRINT("[HighPrioTask] Iteration %d: Starting iteration. Sleeping briefly...\n", i);
      tm_thread_sleep_ticks(5); // Allow LowPrioTask to run and acquire the mutex

      DBG_PRINT("[HighPrioTask] Iteration %d: Initiating PMU measurement and attempting to acquire mutex.\n", i);
      tm_pmu_profile_start(pmu_names[i]);

      if (tm_mutex_get(SHARED_MUTEX_ID) == TM_SUCCESS)
      {
         tm_pmu_profile_end(pmu_names[i]);
         DBG_PRINT("[HighPrioTask] Iteration %d: Mutex acquired. Reporting PMU result.\n", i);
         tm_pmu_profile_print(pmu_names[i]);
         tm_mutex_put(SHARED_MUTEX_ID);
      }
      else
      {
         DBG_PRINT("[HighPrioTask] Iteration %d: Failed to acquire mutex.\n", i);
      }
      tm_thread_sleep_ticks(100);
   }
   DBG_PRINT("[HighPrioTask] All iterations complete. Test finished.\n");
   tm_thread_suspend(HIGH_TASK_ID);
}

/*******************************************************************************
 * Medium Priority Task
 *
 * This task continuously runs to simulate interference. Its work should not
 * delay the low-priority task if inheritance is working properly.
 ******************************************************************************/
static void MedPrioTask(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;
   for (int i = 0; i < ITERATION_COUNT * 20; i++)
   {
      DBG_PRINT("[MedPrioTask] Iteration %d: Running interference.\n", i);
      tm_thread_sleep_ticks(5);
   }
   DBG_PRINT("[MedPrioTask] Interference complete. Suspending task.\n");
   tm_thread_suspend(MED_TASK_ID);
}

/*******************************************************************************
 * Test Initialization
 *
 * Precomputes PMU names, creates the mutex and tasks, and resumes them.
 ******************************************************************************/
static void tm_priority_inheritance_initialize(void)
{
   int i;
   tm_setup_pmu();

   /* Precompute PMU names for each iteration (using same name for start, end, and print) */
   for (i = 0; i < ITERATION_COUNT; i++)
   {
      snprintf(pmu_names[i], sizeof(pmu_names[i]), "S%02d", i);
   }

   tm_mutex_create(SHARED_MUTEX_ID);

   tm_thread_create(LOW_TASK_ID, LOW_TASK_PRIO, LowPrioTask);
   tm_thread_create(MED_TASK_ID, MED_TASK_PRIO, MedPrioTask);
   tm_thread_create(HIGH_TASK_ID, HIGH_TASK_PRIO, HighPrioTask);

   tm_thread_resume(LOW_TASK_ID);
   tm_thread_resume(MED_TASK_ID);
   tm_thread_resume(HIGH_TASK_ID);

   DBG_PRINT("[Init] Priority Inheritance test started for %d iterations in %s mode.\n", ITERATION_COUNT,
             current_test_mode == MODE_INHERITANCE ? "Inheritance" : "Baseline");
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
