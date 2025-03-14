/*[CR]******************************************************************************
 * Copyright (c) 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 *
 * This program tests the performance of optimized low overhead wait and signal
 * processing in a RTOS.
 *
 *
 *
 *
 ******************************************************************************/
/*******************************************************************************
 * includes
 ******************************************************************************/
#include "tm_api.h"

#define TM_TEST_DURATION 30
/* Enable this to make the same test using standard binary semaphore to sync */
// #define USE_REFERENCE

/* Define the counters used in the demo application...  */
unsigned long synchronozation_counter;

/* Define the test thread prototypes.  */
void sync_waiting_task(void* p1, void* p2, void* p3);
void sync_signaling_task(void* p1, void* p2, void* p3);

/* Define the reporting function prototype.  */
void preempting_report_thread(void);

/* Define the initialization prototype.  */
void specific_synchronization_initialize(void);

/* Define the synchronization processing thread A.  */
void sync_waiting_task(void* p1, void* p2, void* p3)
{

   while (1)
   {
/* Wait for the signal from Task B using RTOS-specific API  */
#ifdef USE_REFERENCE
      tm_semaphore_wait(0);
#else
      rtos_sync_wait();
#endif
      // printf("Task A: Got the signal from Task B\r\n");
      /* Increment the number of semaphore get/puts.  */
      synchronozation_counter++;
   }
}

/* Define the synchronization processing thread B.  */
void sync_signaling_task(void* p1, void* p2, void* p3)
{

   while (1)
   {
#ifdef USE_REFERENCE
      /* Signal Task A by giving semaphore 0 */
      tm_semaphore_put(0);
#else
      rtos_sync_signal();
#endif
      // printf("Task B: signal\r\n");
   }
}

/* Define the synchronization test reporting function.  */
void preempting_report_thread(void)
{

   unsigned long last_counter;
   unsigned long relative_time;

   /* Initialize the last counter.  */
   last_counter = 0;

   /* Initialize the relative time.  */
   relative_time = 0;

   while (1)
   {

      /* Sleep to allow the test to run.  */
      tm_thread_sleep(TM_TEST_DURATION);

      /* Increment the relative time.  */
      relative_time = relative_time + TM_TEST_DURATION;

      /* Print results to the stdio window.  */
      printf("**** Thread-Metric Synchronization Processing Test **** Relative Time: "
             "%lu\r\n",
             relative_time);

      /* See if there are any errors.  */
      if (synchronozation_counter == last_counter)
      {

         printf("ERROR: Invalid counter value(s). Error getting/putting "
                "semaphore!\r\n");
      }
      
      /* Show the time period total.  */
      printf("Time Period Total:  %lu\r\n\r\n", synchronozation_counter - last_counter);

      /* Save the last counter.  */
      last_counter = synchronozation_counter;
   }
}

/* Define main entry point.  */
int main_specific_sync(void)
{
   /* Initialize the test.  */
   tm_initialize(specific_synchronization_initialize);

   return 0;
}

/* Define the synchronization processing test initialization.  */
void specific_synchronization_initialize(void)
{
#ifdef USE_REFERENCE
   /* Create two semaphores:
   - Semaphore 0 (for Task A) starts available.
   - Semaphore 1 (for Task B) starts unavailable.
   */
   tm_semaphore_create(0);
   tm_semaphore_create(1);
   /* Immediately take semaphore 1 so it is empty */
   tm_semaphore_get(1);
#endif
   /* Initialize the optimizzed rtos synchronization mechanism  */
   init_rtos_sync();
   /* Create thread 0 at priority 5.  */
   tm_thread_create(0, 5, sync_waiting_task);
   /* Create thread 1 at priority 10.  */
   tm_thread_create(1, 10, sync_signaling_task);

   /* Resume thread 0.  */
   tm_thread_resume(0);
   /* Resume thread 1.  */
   tm_thread_resume(1);

   /* Create a semaphore for the test.  */
   tm_semaphore_create(0);

   preempting_report_thread();
}
