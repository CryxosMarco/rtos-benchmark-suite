/*[CR]******************************************************************************
 * Critical Section Benchmark
 *
 * This test uses the rtos specific mechanisms to enter and exit a critical section
 * and measures the time spent in the critical section. The test is driven by a
 * single thread that enters and exits the critical section in a loop. The time
 * spent in the critical section is measured using the PMU.
 *
 * Copyright (c) 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 *
 ******************************************************************************/

#include "tm_api.h"
#include <stdio.h>
#include <string.h>

/* Number of iterations the PMU should take measurements */
#define ITERATION_COUNT 32

/* Global counter */
volatile unsigned long critical_section_counter = 0; // Used by ISR as message index

/* Precomputed PMU name array */
char pmu_crit_numbers[ITERATION_COUNT][16];

/* Function prototypes */
void tm_critical_section_benchmark_thread(void* p1, void* p2, void* p3);
void tm_critical_section_reporting_thread(void* p1, void* p2, void* p3);
void tm_critical_section_test_initialize(void);

/* Initialization function */
void tm_critical_section_test_initialize(void)
{
   int i;
   tm_setup_pmu();

   /* Precompute PMU names for each iteration so the ISR can avoid runtime formatting */
   for (i = 0; i < ITERATION_COUNT; i++)
   {
      snprintf(pmu_crit_numbers[i], sizeof(pmu_crit_numbers[i]), "S%02d", i);
   }

   /* Create the single receiver thread that drives the entire test */
   tm_thread_create(0, 5, tm_critical_section_benchmark_thread);
   tm_thread_resume(0);
   tm_thread_create(1, 1, tm_critical_section_reporting_thread);
   tm_thread_resume(1);
}

/* Critical Section thread */
void tm_critical_section_benchmark_thread(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;

   while (1)
   { /* Using the PMU to Measure the first ITERATION_COUNT loops */
      if (critical_section_counter < ITERATION_COUNT)
      {
         tm_pmu_profile_start(pmu_crit_numbers[critical_section_counter]);
      }
      tm_enter_critical_section();
      critical_section_counter++;
      tm_exit_critical_section();
      if (critical_section_counter < ITERATION_COUNT + 1)
      {
         tm_pmu_profile_end(pmu_crit_numbers[critical_section_counter - 1]);
      }
   }
}

/* Reporting thread */
void tm_critical_section_reporting_thread(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;
   unsigned long last_counter;
   unsigned long relative_time;
   last_counter = 0;
   relative_time = 0;
   while (1)
   {
      tm_thread_sleep(TM_TEST_DURATION);
      relative_time = relative_time + TM_TEST_DURATION;
      printf("**** Critical Section Benchmark **** Relative Time: %lu\r\n", relative_time);
      if (critical_section_counter == last_counter)
      {
         printf("ERROR: Invalid counter value(s). Error getting/putting semaphore!\r\n");
      }
      printf("Time Period Total:  %lu\r\n\r\n", critical_section_counter - last_counter);
      if (last_counter == 0)
      {
         for (int i = 0; i < ITERATION_COUNT; i++)
         {
            tm_pmu_profile_print(pmu_crit_numbers[i]);
         }
      }
      last_counter = critical_section_counter;
   }
}

int main_critical_section_test(void)
{
   tm_initialize(tm_critical_section_test_initialize);
   return 0;
}
