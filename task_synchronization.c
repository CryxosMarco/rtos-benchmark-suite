/*[CR]**************************************************************************
Copyright (c) 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
SPDX-License-Identifier: Apache-2.0
*/
/*[FH]**************************************************************************
PROJECT: MASTER THESIS
MODULE: SYNCHRONISATION TEST
CONTENTS: Short description of the file content
*/
/*[CL]**************************************************************************
21-01-2025 MMI Initial creation of the file

---
MMI: Marco Milenkovic, IBV, Milenkovic@ibv-augsburg.de
*/
/*[MP]**************************************************************************
 * Synchronistation Promblem example using tm_api.h
 *
 * Demonstrates:
 *   - How to set up a simple "UART" usage protected by a mutex
 *   - Creating/resuming tasks
 *   - Printing output with printf (simulating UART output)
 *
 *
 ******************************************************************************/
/*******************************************************************************
 * includes
 ******************************************************************************/

#include "tm_api.h"
#include <stdio.h>
#include <string.h>

/* We define a unique mutex ID for the UART resource. */
#define MUTEX_ID 1
#define SEM_A 1
#define SEM_B 2

unsigned long global_thread_counter = 0;

/********************************************************************************
 * OPEN/CLOSE/PUTCH: Abstract "UART" usage
 ********************************************************************************/

/* Acquire the UART mutex so only one task prints at a time. */
static int lock_critical(void)
{
   /* Ensure the mutex is available.  */
   return tm_mutex_get(MUTEX_ID) == TM_SUCCESS ? TM_SUCCESS : TM_ERROR;
}

/* Release the UART mutex after finishing the print. */
static void unlock_critical(void)
{
   tm_mutex_put(MUTEX_ID);
}

/********************************************************************************
 * TEST TASKS
 * Two tasks that get synchrinized by a semaphore and access the UART as shared resource
 * TODO: YIELD or SLEEP to allow other tasks to run. TEST both
 ********************************************************************************/

/* First writer task: prints "Hello from writer_task1" a few times. */
static void writer_task1(void* arg1, void* arg2, void* arg3)
{
   (void) arg1;
   (void) arg2;
   (void) arg3;

   while (1)
   {
      /* wait on SEM_A to put from Task2 */
      tm_semaphore_wait(SEM_A);
      tm_pmu_profile_end("SEM_A_perf");

      if (lock_critical() == TM_SUCCESS)
      {
         global_thread_counter++;
         unlock_critical();
      }
      /* Release the second semaphore */
      tm_semaphore_put(SEM_B);
   }
}

/* Second writer task: prints a different message. */
static void writer_task2(void* arg1, void* arg2, void* arg3)
{
   (void) arg1;
   (void) arg2;
   (void) arg3;

   while (1)
   {
      /* wait on SEM_B put form Task1 */
      tm_pmu_profile_start("SEM_A_perf");
      tm_semaphore_wait(SEM_B);

      if (lock_critical() == TM_SUCCESS)
      {
         global_thread_counter++;
         unlock_critical();
      }
      /* Release the first semaphore */
      tm_semaphore_put(SEM_A);
   }
}

static void reporting_thread(void* arg1, void* arg2, void* arg3)
{
   unsigned long total;
   unsigned long relative_time;
   unsigned long last_total;

   /* Initialize the last total.  */
   last_total = 0;

   /* Initialize the relative time.  */
   relative_time = 0;

   while (1)
   {

      /* Sleep to allow the test to run.  */
      tm_thread_sleep(3);

      /* Increment the relative time.  */
      relative_time = relative_time + 3;

      /* Print results to the stdio window.  */
      printf("**** Task Synchronistation Test **** Relative Time: %lu\n", relative_time);

      /* Calculate the total of all the counters.  */
      total = global_thread_counter;

      /* Show the time period total.  */
      printf("Time Period Total:  %lu\n\n", total - last_total);

      /* Print the PMU Report */
      tm_pmu_profile_print("SEM_A_perf");

      /* Save the last total.  */
      last_total = total;
   }
}

/********************************************************************************
 * TEST INITIALIZATION FUNCTION
 * Called by tm_initialize() from main().
 * This function sets up the mutex and creates/resumes the tasks.
 ********************************************************************************/
static void task_synchronisation_initialize(void)
{
   /* initialze PMU */
   tm_setup_pmu();
   /* Create the UART mutex. */
   tm_mutex_create(MUTEX_ID);
   /* Create the semaphore to snychronize tasks.
      The implementation calls a semaphore_get()
      to make it available from the start */
   tm_semaphore_create(SEM_A);
   tm_semaphore_create(SEM_B);
   /* guarantee that the semaphore A is blocked so Task2 can start first.*/
   tm_semaphore_get(SEM_A);

   /* Create two tasks, each with a unique ID. Priority is arbitrary.
      Without synchronisation Task1 would start operating before task2.
      we use synchronisation to ensure that task2 can finish printing first*/
   tm_thread_create(1, 5, writer_task1);
   tm_thread_create(2, 5, writer_task2);
   tm_thread_create(3, 1, reporting_thread);

   /* Start (resume) both tasks. */
   tm_thread_resume(1);
   tm_thread_resume(2);
   tm_thread_resume(3);
}

/********************************************************************************
 * MAIN ENTRY POINT
 ********************************************************************************/
int main_sync(void)
{
   printf("[Main] Starting Synchronisation Test.\n");

   /* Call tm_initialize(), passing our task_synchronisation_initialize.
    * The real implementation of tm_initialize() will do RTOS setup,
    * then call task_synchronisation_initialize(), then start scheduling tasks.
    */
   tm_initialize(task_synchronisation_initialize);

   /* In many RTOSes, tm_initialize() might not return. If it does here,
    * we just print a message. */
   printf("[Main] tm_initialize returned, threads started.\n");

   return 0;
}

/*[EOF]************************************************************************/
