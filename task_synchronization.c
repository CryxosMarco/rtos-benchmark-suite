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
#define ITERATION_COUNT 32

unsigned long task1_counter = 0;
unsigned long task2_counter = 0;

/* Precomputed PMU name arrays for send and receive iterations */
char pmu_task_sync_names[ITERATION_COUNT][16];

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
      if (task1_counter < ITERATION_COUNT)
      {
         tm_pmu_profile_end(pmu_task_sync_names[task1_counter + 1]);
      }

      task1_counter++;

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
      tm_semaphore_wait(SEM_B);

      task2_counter++;
      if (task2_counter < ITERATION_COUNT)
      {
         tm_pmu_profile_start(pmu_task_sync_names[task2_counter]);
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
      tm_thread_sleep(TM_TEST_DURATION);

      /* Increment the relative time.  */
      relative_time = relative_time + TM_TEST_DURATION;

      /* Print results to the stdio window.  */
      printf("**** Task Synchronistation Test **** Relative Time: %lu\r\n", relative_time);

      /* Calculate the total of all the counters. */
      total = task1_counter + task2_counter;

      /* Show the time period total.  */
      printf("Time Period Total:  %lu\r\n", total - last_total);
      printf("Task1 Counter:  %lu\r\n", task1_counter);
      printf("Task2 Counter:  %lu\r\n", task2_counter);

      /* Print the PMU Report */
      for (int i = 0; i < ITERATION_COUNT; i++)
      {
         tm_pmu_profile_print(pmu_task_sync_names[i]);
      }

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

   /* Precompute PMU names for each iteration so the ISR can avoid runtime formatting */
   for (int i = 0; i < ITERATION_COUNT; i++)
   {
      snprintf(pmu_task_sync_names[i], sizeof(pmu_task_sync_names[i]), "Cycle%02d", i);
   }

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
   tm_thread_create(1, 4, writer_task1);
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
   printf("[Main] Starting Synchronisation Test.\r\n");

   /* Call tm_initialize(), passing our task_synchronisation_initialize.
    * The real implementation of tm_initialize() will do RTOS setup,
    * then call task_synchronisation_initialize(), then start scheduling tasks.
    */
   tm_initialize(task_synchronisation_initialize);

   /* In many RTOSes, tm_initialize() might not return. If it does here,
    * we just print a message. */
   printf("[Main] tm_initialize returned, threads started.\r\n");
   return 0;
}

/*[EOF]************************************************************************/
