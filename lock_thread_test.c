/*[CR]******************************************************************************
 * Thread Locking Benchmark
 *
 * This test uses RTOS-specific thread locking functions
 * (tm_scheduler_stop() and tm_scheduler_start()) to measure the time required
 * to lock a thread for execution, ensuring it is not preempted by higher priority
 * tasks. The test is driven by a single benchmark thread that repeatedly locks
 * and unlocks the scheduler while the PMU records the locked duration. This
 * benchmark is intended for comparing multiple RTOSes using a common API.
 *
 * Copyright (c) 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 *
 ******************************************************************************/

#include "tm_api.h"
#include <stdio.h>
#include <string.h>

#define ITERATION_COUNT 32

/* Global counter for the number of locking operations */
volatile unsigned long thread_locking_counter = 0;

/* Precomputed PMU name array for each iteration */
char pmu_lock_numbers[ITERATION_COUNT][16];

/* Function prototypes */
void tm_thread_locking_benchmark_thread(void* p1, void* p2, void* p3);
void tm_thread_locking_reporting_thread(void* p1, void* p2, void* p3);
void tm_thread_locking_test_initialize(void);

/* Initialization function */
void tm_thread_locking_test_initialize(void)
{
   int i;
   tm_setup_pmu();

   /* Precompute PMU names for each iteration to avoid runtime formatting overhead */
   for (i = 0; i < ITERATION_COUNT; i++)
   {
      snprintf(pmu_lock_numbers[i], sizeof(pmu_lock_numbers[i]), "L%02d", i);
   }

   /* Create the benchmark thread that drives the locking test */
   tm_thread_create(0, 5, tm_thread_locking_benchmark_thread);
   tm_thread_resume(0);

   /* Create a reporting thread to output benchmark results */
   tm_thread_create(1, 1, tm_thread_locking_reporting_thread);
   tm_thread_resume(1);
}

/* Benchmark thread that tests thread locking */
void tm_thread_locking_benchmark_thread(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;

   while (1)
   {
      if (thread_locking_counter < ITERATION_COUNT)
      {
         tm_pmu_profile_start(pmu_lock_numbers[thread_locking_counter]);
      }

      /* Lock the thread by stopping the scheduler */
      tm_suspend_scheduler();
      thread_locking_counter++;
      /* busy work loop to have some blocked time*/
      for (int i = 0; i < 1000; i++)
      {
         __asm__ volatile("nop");
      }
      /* Unlock the thread by restarting the scheduler */
      tm_resume_scheduler();

      if (thread_locking_counter < ITERATION_COUNT + 1)
      {
         tm_pmu_profile_end(pmu_lock_numbers[thread_locking_counter - 1]);
      }
   }
}

/* Reporting thread to display PMU measurements and counter progress */
void tm_thread_locking_reporting_thread(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;
   unsigned long last_counter = 0;
   unsigned long relative_time = 0;

   while (1)
   {
      tm_thread_sleep(TM_TEST_DURATION);
      relative_time += TM_TEST_DURATION;
      printf("**** Thread Locking Benchmark **** Relative Time: %lu\r\n", relative_time);

      if (thread_locking_counter == last_counter)
      {
         printf("ERROR: No progress in thread locking counter!\r\n");
      }
      else
      {
         printf("Locking Operations in Period: %lu\r\n\r\n", thread_locking_counter - last_counter);
      }

      /* Print PMU profile results on the first reporting interval */
      if (last_counter == 0)
      {
         for (int i = 0; i < ITERATION_COUNT; i++)
         {
            tm_pmu_profile_print(pmu_lock_numbers[i]);
         }
      }
      last_counter = thread_locking_counter;
   }
}

/* Main entry point for the thread locking test */
int main_thread_locking_test(void)
{
   tm_initialize(tm_thread_locking_test_initialize);
   return 0;
}
