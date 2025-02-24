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

#define TM_TEST_DURATION 3

/* Define the counters used in the demo application...  */
unsigned long tm_synchronization_processing_counter;

/* Define the test thread prototypes.  */
void sync_waiting_task(void* p1, void* p2, void* p3);
void sync_signaling_task(void* p1, void* p2, void* p3);

/* Define the reporting function prototype.  */
void preempting_report_thread(void);

/* Define the initialization prototype.  */
void optimized_synchronization_initialize(void);

/* Define the synchronization processing thread A.  */
void sync_waiting_task(void* p1, void* p2, void* p3)
{

   while (1)
   {
      /* Wait for the signal from Task B using RTOS-specific API  */
      rtos_sync_wait();

      /* Increment the number of semaphore get/puts.  */
      tm_synchronization_processing_counter++;
   }
}

/* Define the synchronization processing thread B.  */
void sync_signaling_task(void* p1, void* p2, void* p3)
{

   while (1)
   {
      /* Signal Task A using RTOS-specific API  */
      rtos_sync_signal();
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
             "%lu\n",
             relative_time);

      /* See if there are any errors.  */
      if (tm_synchronization_processing_counter == last_counter)
      {

         printf("ERROR: Invalid counter value(s). Error getting/putting "
                "semaphore!\n");
      }

      /* Show the time period total.  */
      printf("Time Period Total:  %lu\n\n", tm_synchronization_processing_counter - last_counter);

      /* Save the last counter.  */
      last_counter = tm_synchronization_processing_counter;
   }
}

/* Define main entry point.  */
int main_optimized_sync(void)
{
   /* Initialize the test.  */
   tm_initialize(optimized_synchronization_initialize);

   return 0;
}

/* Define the synchronization processing test initialization.  */
void optimized_synchronization_initialize(void)
{

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
