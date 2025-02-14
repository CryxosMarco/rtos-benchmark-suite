/*[CR]******************************************************************************
 * Message Queue Task-to-Task Benchmark
 *
 * Copyright (c) 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 *
 * This program tests the perofrmace of message queues used in a RTOS from a
 * Task to another Task.
 *
 ******************************************************************************/
/*******************************************************************************
 * includes
 ******************************************************************************/
#include "tm_api.h"
#include <stdio.h>
#define TM_TEST_DURATION 3

/*---------------------------------------------------------------
  Global Counters and Message Buffers
---------------------------------------------------------------*/
/* Global counter: number of messages successfully processed */
volatile unsigned long tm_receiver_counter = 0;

/* Global counter: number of interrupts (i.e. ISR invocations) */
volatile unsigned long tm_sender_counter = 0;

/* Message buffers */
unsigned long message_sent_arr[4];
unsigned long message_received_arr[4];

/*---------------------------------------------------------------
  Thread and ISR Prototypes
---------------------------------------------------------------*/
void tm_task_receiver_thread_entry(void* p1, void* p2, void* p3);
void tm_task_sender_thread_entry(void* p1, void* p2, void* p3);
void tm_reporting_thread_entry(void* p1, void* p2, void* p3);
void tm_isr_message_handler(void);
void tm_message_task_to_task_initialize(void);

/*---------------------------------------------------------------
  Main Entry Point
---------------------------------------------------------------*/
int main_message_task_test(void)
{
   /* Initialize the RTOS and start the ISR-to-Task test */
   tm_initialize(tm_message_task_to_task_initialize);
   return 0;
}

/*---------------------------------------------------------------
  Initialization Function
---------------------------------------------------------------*/
void tm_message_task_to_task_initialize(void)
{
   /* Initialize the PMU for low-overhead cycle counting */
   tm_setup_pmu();

   /* Initialize the message content with a known pattern */
   message_sent_arr[0] = 0xDEADBEEF;
   message_sent_arr[1] = 0xCAFEBABE;
   message_sent_arr[2] = 0xBAADF00D;
   message_sent_arr[3] = 0xFEEDFACE;

   /* Create a message queue with id 0 */
   tm_queue_create(0);

   /* Create and resume the Receiver Thread */
   tm_thread_create(0, 2, tm_task_receiver_thread_entry);
   /* Create and resume the Sender Thread */
   tm_thread_create(1, 5, tm_task_sender_thread_entry);
   /* Create reporting thread with highest priority*/
   tm_thread_create(2, 1, tm_reporting_thread_entry);

   tm_thread_resume(0);
   tm_thread_resume(1);
   tm_thread_resume(2);

   printf("[Init] Task-to-Task Message Queue Benchmark started.\n");
}

/*---------------------------------------------------------------
  Receiver Thread
  - Blocks on the message queue.
  - Immediately after receiving a message, stops the PMU latency measurement.
  - Increments the message-processed counter.
  - Compares the received message to the known send pattern.
  - If they match, suspends the interrupt simulator thread and outputs the final report.
---------------------------------------------------------------*/
void tm_task_receiver_thread_entry(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;

   while (1)
   {
      /* Block until a message is available from queue 0 */
      tm_queue_receive(0, message_received_arr);

      /* Stop the PMU measurement started in the ISR */
      // tm_pmu_profile_end("msg_latency");

      /* Increment the processed message count */
      tm_receiver_counter++;
   }

   /* Suspend this thread after finishing the report */
   tm_thread_suspend(0);
}

/*---------------------------------------------------------------
  Message sending thread
---------------------------------------------------------------*/
void tm_task_sender_thread_entry(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;

   while (1)
   {
      // tm_pmu_profile_start("msg_latency");
      if (tm_queue_send(0, message_sent_arr) != 0)
      {
         printf("Message send gone wrong! \n");
      }
      tm_sender_counter++;
   }
}

void tm_reporting_thread_entry(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;

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
      printf("**** Task-To-Task Message Queue Test **** Relative Time: %lu\n", relative_time);

      /* Calculate the total of all the counters. */
      total = tm_sender_counter + tm_receiver_counter;

      /* Show the time period total.  */
      printf("Time Period Total:  %lu\n\n", total - last_total);

      /* Save the last total.  */
      last_total = total;
   }
}

/*[EOF]************************************************************************/
