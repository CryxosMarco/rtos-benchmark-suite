/*[CR]******************************************************************************
 * Message Queue Task-to-Task Benchmark
 *
 * Copyright (c) 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 *
 * This program tests the performance of message queues used in a RTOS from one
 * task to another.
 *
 ******************************************************************************/
/*******************************************************************************
 * includes
 ******************************************************************************/
#include "tm_api.h"
#include <stdio.h>

/*---------------------------------------------------------------
  Compile-Time Configuration Macros
---------------------------------------------------------------*/
/* Duration of the test in seconds different from tm_api.h */
#define TEST_DURATION 10

/* Uncomment the following line to enable observation of per-thread activity */
// #define OBSERVE

/* Number of producers and consumers */
#define NUM_PRODUCERS 3
#define NUM_CONSUMERS 3
/* Reporting thread ID (should be unique) */
#define REPORTING_THREAD_ID (NUM_PRODUCERS + NUM_CONSUMERS)

/*---------------------------------------------------------------
  Global Counters
---------------------------------------------------------------*/
volatile unsigned long total_sent_counter = 0;
volatile unsigned long total_received_counter = 0;
volatile unsigned long integrity_errors_counter = 0;

#ifdef OBSERVE
/* Per-thread run counts for observation */
volatile unsigned long producer_run_counts[NUM_PRODUCERS] = {0};
volatile unsigned long consumer_run_counts[NUM_CONSUMERS] = {0};
#endif

/*---------------------------------------------------------------
  Global Arrays to hold thread IDs (these will be passed via p1)
---------------------------------------------------------------*/
int producer_ids[NUM_PRODUCERS];
int consumer_ids[NUM_CONSUMERS];

/*---------------------------------------------------------------
  Function Prototypes
---------------------------------------------------------------*/
void message_queue_test_initialize(void);
int main_message_task_test(void);
static unsigned long compute_checksum(unsigned long* msg, int size);

/* Generic Producer and Consumer thread entry functions */
void producer_thread_entry_generic(void* p1, void* p2, void* p3);
void consumer_thread_entry_generic(void* p1, void* p2, void* p3);
void reporting_thread_entry(void* p1, void* p2, void* p3);

/*---------------------------------------------------------------
  Main Entry Point
---------------------------------------------------------------*/
int main_message_task_test(void)
{
   tm_initialize(message_queue_test_initialize);
   return 0;
}

/*---------------------------------------------------------------
  Initialization Function
---------------------------------------------------------------*/
void message_queue_test_initialize(void)
{
   int i;

   /* Initialize the PMU if needed */
   tm_setup_pmu();

   /* Create a message queue with id 0.
      The queue is configured for messages of size:
      MESSAGE_SIZE * sizeof(unsigned long) */
   tm_queue_create(0);

   /* Create and resume producer threads using the generic function */
   for (i = 0; i < NUM_PRODUCERS; i++)
   {
      producer_ids[i] = i; // store the producer id
      tm_thread_create_param(i, 5, producer_thread_entry_generic, &producer_ids[i]);
      tm_thread_resume(i);
   }

   /* Create and resume consumer threads using the generic function */
   for (i = 0; i < NUM_CONSUMERS; i++)
   {
      consumer_ids[i] = i; // store the consumer id
      tm_thread_create_param(NUM_PRODUCERS + i, 5, consumer_thread_entry_generic, &consumer_ids[i]);
      tm_thread_resume(NUM_PRODUCERS + i);
   }

   /* Create and resume the reporting thread with a higher priority (no parameter needed) */
   tm_thread_create(REPORTING_THREAD_ID, 1, reporting_thread_entry);
   tm_thread_resume(REPORTING_THREAD_ID);

   printf("[Init] Multi Producer/Consumer Message Queue Benchmark started.\r\n");
}

/*---------------------------------------------------------------
  Utility: Compute a Simple Checksum
  Computes an additive checksum over 'size' elements of msg.
---------------------------------------------------------------*/
static unsigned long compute_checksum(unsigned long* msg, int size)
{
   unsigned long checksum = 0;
   for (int i = 0; i < size; i++)
   {
      checksum += msg[i];
   }
   return checksum;
}

/*---------------------------------------------------------------
  Producer Thread Generic Function
  Receives its ID via the first parameter, builds and sends messages.
  Message layout:
       [0] : Producer ID
       [1] : Local message counter
       [2..MESSAGE_SIZE-2] : A predictable pattern
       [MESSAGE_SIZE-1] : Checksum over the first (MESSAGE_SIZE-1) words.
---------------------------------------------------------------*/
void producer_thread_entry_generic(void* p1, void* p2, void* p3)
{
   (void) p2;
   (void) p3;

   int producer_id = *((int*) p1);
   unsigned long local_counter = 0;

   while (1)
   {
      unsigned long message[MESSAGE_SIZE];

      /* Build the message */
      message[0] = producer_id;
      message[1] = local_counter;
      for (int i = 2; i < MESSAGE_SIZE - 1; i++)
      {
         message[i] = producer_id * 1000 + i;
      }
      /* Compute and store the checksum */
      message[MESSAGE_SIZE - 1] = compute_checksum(message, MESSAGE_SIZE - 1);

      /* Send the message; print an error if sending fails */
      if (tm_queue_send(0, message) != 0)
      {
         printf("Producer %d: Message send failed!\r\n", producer_id);
      }
      else
      {
         total_sent_counter++;
#ifdef OBSERVE
         producer_run_counts[producer_id]++;
#endif
      }

      local_counter++;
   }
}

/*---------------------------------------------------------------
  Consumer Thread Generic Function
  Receives its ID via the first parameter, then continuously receives messages
  and verifies the checksum.
---------------------------------------------------------------*/
void consumer_thread_entry_generic(void* p1, void* p2, void* p3)
{
   (void) p2;
   (void) p3;

   int consumer_id = *((int*) p1);

   while (1)
   {
      unsigned long message[MESSAGE_SIZE];

      /* Block until a message is available on queue 0 */
      tm_queue_receive(0, message);

      /* Recompute the checksum and verify */
      unsigned long expected_checksum = compute_checksum(message, MESSAGE_SIZE - 1);
      if (expected_checksum != message[MESSAGE_SIZE - 1])
      {
         printf("Consumer %d: Message integrity error. Expected checksum %lu, got %lu\r\n", consumer_id,
                expected_checksum, message[MESSAGE_SIZE - 1]);
         integrity_errors_counter++;
      }
      total_received_counter++;
#ifdef OBSERVE
      consumer_run_counts[consumer_id]++;
#endif
   }
}

/*---------------------------------------------------------------
  Reporting Thread
  Periodically prints throughput, integrity, and per-thread activity statistics.
---------------------------------------------------------------*/
void reporting_thread_entry(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;

   unsigned long last_total_sent = 0;
   unsigned long last_total_received = 0;
   unsigned long relative_time = 0;
   double iteration_time_us_send = 0;
   double iteration_time_us_receive = 0;

   while (1)
   {
      tm_thread_sleep(TEST_DURATION);
      relative_time += TEST_DURATION;

      unsigned long period_sent = total_sent_counter - last_total_sent;
      unsigned long period_received = total_received_counter - last_total_received;
      last_total_sent = total_sent_counter;
      last_total_received = total_received_counter;

      printf("**** Multi Producer/Consumer Message Queue Test **** Time: %lu sec\r\n", relative_time);
      printf("Messages Sent in Period: %lu\r\n", period_sent);
      printf("Messages Received in Period: %lu\r\n", period_received);
      printf("Integrity Errors: %lu\r\n", integrity_errors_counter);

      /* Calculate the average time per iteration using the helper function */
      iteration_time_us_send = calculate_iteration_time(TM_TEST_DURATION, period_sent);
      printf("Average Time per Send: %f us\r\n", iteration_time_us_send);
      iteration_time_us_receive = calculate_iteration_time(TM_TEST_DURATION, period_received);
      printf("Average Time per Receive: %f us\r\n", iteration_time_us_receive);
#ifdef OBSERVE
      /* Print per-thread execution counts */
      printf("\nPer-Producer Execution Counts:\r\n");
      for (int i = 0; i < NUM_PRODUCERS; i++)
      {
         printf("  Producer %d: %lu\r\n", i, producer_run_counts[i]);
      }
      printf("\nPer-Consumer Execution Counts:\r\n");
      for (int i = 0; i < NUM_CONSUMERS; i++)
      {
         printf("  Consumer %d: %lu\r\n", i, consumer_run_counts[i]);
      }
      printf("\r\n");
#endif

      printf("\r\n");
   }
}

/*[EOF]************************************************************************/
