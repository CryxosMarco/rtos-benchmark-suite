/*[CR]******************************************************************************
 *   Priority Inheritance Test Program
 *
 * Copyright (c) 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 *
 * This program demonstrates how three tasks of different priorities (High,
 * Medium, Low) share a mutex-protected resource to trigger and observe
 * priority inheritance on an RTOS that implements it (e.g., FreeRTOS).
 *
 * Optional: The code also includes a rudimentary placeholder for measuring
 * performance or timing differences across different RTOSes, if desired.
 *
 * You can compile and run this test via one of the supported RTOS platforms
 * that implement the TM API (tm_api.h) for tasks, mutexes, etc.
 *
 ******************************************************************************/
/*******************************************************************************
 * includes
 ******************************************************************************/
#include "tm_api.h"
#include <stdio.h>

/* Define a unique mutex ID for our shared resource. */
#define SHARED_MUTEX_ID 1

/* Define IDs and priorities for our three tasks. */
#define HIGH_TASK_ID 0
#define HIGH_TASK_PRIO 5 /* relatively high */

#define MED_TASK_ID 1
#define MED_TASK_PRIO 10 /* medium */

#define LOW_TASK_ID 2
#define LOW_TASK_PRIO 20 /* relatively low */

/* Globals for measurement */
static unsigned long start_time = 0;
static unsigned long end_time = 0;

/*******************************************************************************
 * TEST TASKS
 * Three tasks that share a mutex to demonstrate priority inheritance.
 * LowPrioTask holds the mutex, HighPrioTask tries to lock it,
 * MediumPrioTask runs to show preemption, etc.
 ******************************************************************************/

/*----------------------------------------------------------------------------*/
/* Low Priority Task: acquires the mutex first, simulates "long" work while
   holding it, to force the High Priority Task to block → triggers inheritance. */
/*----------------------------------------------------------------------------*/
static void LowPrioTask(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;

   printf("[LowPrioTask] Attempting to get mutex...\n");
   if (tm_mutex_get(SHARED_MUTEX_ID) == TM_SUCCESS)
   {
      printf("[LowPrioTask] Mutex acquired. Doing work...\n");

      /* Optional: record start time for performance measure. */
      start_time = tm_time_get();

      /* Simulate some "long" operation while holding the mutex. */
      volatile int computation_result = 0; // Prevent compiler optimization

      for (volatile int i = 0; i < 10000000; i++)
      {
         /* Some arithmetic that the compiler cannot optimize out easily */
         computation_result += (i % 10) * (i % 3);

         /* Ensure the compiler does not assume computation_result is unused */
         __asm__ volatile("" : "+r"(computation_result));

         /* Meanwhile, if HighPrioTask tries to get the mutex,
            the RTOS should boost this task's priority. */
      }

      /* Optional: record end time. */
      end_time = tm_time_get();

      printf("[LowPrioTask] Done working. Releasing mutex now.\n");
      tm_mutex_put(SHARED_MUTEX_ID);
   }
   else
   {
      printf("[LowPrioTask] Failed to get mutex?\n");
   }

   /* After finishing, suspend itself */
   printf("[LowPrioTask] Finished. Suspending.\n");
   tm_thread_suspend(LOW_TASK_ID);
}

/*----------------------------------------------------------------------------*/
/* High Priority Task: tries to acquire the mutex. If LowPrioTask is holding it,
   this task will block—and the RTOS should raise the LowPrioTask's priority
   to avoid priority inversion. */
/*----------------------------------------------------------------------------*/
static void HighPrioTask(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;

   /* Delay/sleep slightly so LowPrioTask can lock the mutex first. */
   printf("[HighPrioTask] Sleeping a bit so Low can get mutex...\n");
   tm_thread_sleep(1);

   printf("[HighPrioTask] Trying to get mutex.\n");
   if (tm_mutex_get(SHARED_MUTEX_ID) == TM_SUCCESS)
   {
      printf("[HighPrioTask] Successfully got mutex. Priority Inheritance test.\n");
      /* We hold it briefly... */
      tm_thread_sleep(1);

      /* Release it. */
      printf("[HighPrioTask] Releasing mutex.\n");
      tm_mutex_put(SHARED_MUTEX_ID);
   }
   else
   {
      printf("[HighPrioTask] Failed to get mutex?\n");
   }

   /* Optionally measure or just suspend. */
   printf("[HighPrioTask] Finished. Suspending.\n");
   tm_thread_suspend(HIGH_TASK_ID);
}

/*----------------------------------------------------------------------------*/
/* Medium Priority Task: runs in between Low and High. If Low wasn't boosted,
   Medium might preempt Low, causing potential priority inversion. But with
   priority inheritance, Low should be boosted above Medium while holding mutex. */
/*----------------------------------------------------------------------------*/
static void MedPrioTask(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;

   /* This task just increments a counter or prints to show it's alive. */
   int count = 0;

   while (1)
   {
      printf("[MedPrioTask] Running (count=%d). Yielding.\n", ++count);

      /* Tested RTOSes only yield to tasks with same or higher priority, 
         therefore we need to block the task */
      tm_thread_sleep(1);

      if (count > 4)
      {
         printf("[MedPrioTask] Enough demonstration, suspending.\n");
         tm_thread_suspend(MED_TASK_ID);
      }
   }
}

/*******************************************************************************
 * REPORTING & INITIALIZATION
 ******************************************************************************/

/* (Optional) Print measurement results. Could be used to compare RTOS performance. */
static void print_measurement(void)
{
   if (end_time >= start_time)
   {
      unsigned long duration = end_time - start_time;
      printf("[Measurement] LowPrioTask 'work' with priority inheritance took: %lu time units.\n", duration);
   }
   else
   {
      printf("[Measurement] Time measurement is invalid.\n");
   }
}

/*----------------------------------------------------------------------------
 * Priority Inheritance Test Initialization:
 * Creates the mutex, spawns the tasks with different priorities, and resumes them.
 * Called by tm_initialize() from tm_main_priority_inheritance_test().
----------------------------------------------------------------------------*/
static void tm_priority_inheritance_initialize(void)
{
   /* Create the shared mutex. */
   tm_mutex_create(SHARED_MUTEX_ID);

   /* Create and resume the tasks.
      Priority 5 = high, 10 = medium, 20 = low, as an example. */

   tm_thread_create(LOW_TASK_ID, LOW_TASK_PRIO, LowPrioTask);
   tm_thread_create(MED_TASK_ID, MED_TASK_PRIO, MedPrioTask);
   tm_thread_create(HIGH_TASK_ID, HIGH_TASK_PRIO, HighPrioTask);

   tm_thread_resume(LOW_TASK_ID);
   tm_thread_resume(MED_TASK_ID);
   tm_thread_resume(HIGH_TASK_ID);

   printf("[Init] Priority Inheritance test started.\n");
}

/*----------------------------------------------------------------------------
 * Main entry point for the Priority Inheritance Test.
 * This function is typically called from user code or an existing test suite.
 * It calls tm_initialize with our init function, launching tasks, etc.
----------------------------------------------------------------------------*/
int main_inheritance(void)
{
   /* Initialize the RTOS & start the test. */
   tm_initialize(tm_priority_inheritance_initialize);

   return 0;
}
