/*[CR]**************************************************************************
Copyright (c) 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
SPDX-License-Identifier: Apache-2.0
*/
/*[FH]**************************************************************************
PROJECT: MASTER THESIS
MODULE: Context Switching TEST
*/
/*[CL]**************************************************************************
20-05-2025 MMI Initial creation of the file

---
MMI: Marco Milenkovic, IBV, Milenkovic@ibv-augsburg.de
*/
/*******************************************************************************
 * includes
 ******************************************************************************/

#include "tm_api.h"
#include <stdio.h>
#include <string.h>

#define ITERATION_COUNT 32 // number of PMU measurments to take
int global_pmu_index = 0;
unsigned long context_task1_counter = 0;
unsigned long context_task2_counter = 0;
char pmu_context_names[ITERATION_COUNT][16];

static void high_prio_task(void* arg1, void* arg2, void* arg3)
{
   (void) arg1;
   (void) arg2;
   (void) arg3;

   while (1)
   {

      context_task1_counter++;
      if (global_pmu_index)
      {
         global_pmu_index--;
         tm_pmu_profile_start(pmu_context_names[ITERATION_COUNT - global_pmu_index]);
      }
      tm_thread_relinquish(); // Yield to lower-priority task
   }
}

static void low_prio_task(void* arg1, void* arg2, void* arg3)
{
   (void) arg1;
   (void) arg2;
   (void) arg3;

   while (1)
   {
      if (global_pmu_index)
      {
         tm_pmu_profile_end(pmu_context_names[ITERATION_COUNT - global_pmu_index]);
      }
      context_task2_counter++;
      tm_thread_relinquish(); // Yield back to high-priority task
   }
}

static void reporting_thread(void* arg1, void* arg2, void* arg3)
{
   (void) arg1;
   (void) arg2;
   (void) arg3;
   bool pmu_done = false;
   unsigned long last_count = 0;
   unsigned long relative_time = 0;

   while (1)
   {
      tm_thread_sleep(TM_TEST_DURATION);

      unsigned long delta = context_task1_counter + context_task2_counter - last_count;
      last_count = context_task1_counter + context_task2_counter;
      relative_time = relative_time + TM_TEST_DURATION;
      printf("**** Critical Section Benchmark **** Relative Time: %lu\r\n", relative_time);
      printf("Time Period Total:  %lu\r\n\r\n", delta);

      if (!pmu_done)
      {
         for (int i = 0; i < ITERATION_COUNT; i++)
         {
            tm_pmu_profile_print(pmu_context_names[i]);
         }
         pmu_done = true;
      }
   }
}

static void context_switch_test_initialize(void)
{
   tm_setup_pmu();
   global_pmu_index = ITERATION_COUNT;

   for (int i = 0; i < ITERATION_COUNT; i++)
   {
      snprintf(pmu_context_names[i], sizeof(pmu_context_names[i]), "CSW%02d", i);
   }

   // Task 1: high priority - starts the measurement
   tm_thread_create(1, 5, high_prio_task);
   // Task 2: low priority - ends the measurement
   tm_thread_create(2, 5, low_prio_task);
   // Reporter: highest priority
   tm_thread_create(3, 2, reporting_thread);

   tm_thread_resume(1);
   tm_thread_resume(2);
   tm_thread_resume(3);
}

int main_context_switch(void)
{
   printf("[Main] Starting Continuous Context Switch Test...\r\n");
   tm_initialize(context_switch_test_initialize);
   return 0;
}

/*[EOF]************************************************************************/
