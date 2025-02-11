/*[CR]******************************************************************************
 *   Message Queue ISR-to-Task Benchmark
 *
 * Copyright (c) 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 *
 * This program tests the perofrmace of message queues used in a RTOS from a
 * ISR or IRQ.
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

/*---------------------------------------------------------------
  Configuration Macros
---------------------------------------------------------------*/
#define TM_TEST_DURATION 3 // Report interval in seconds

/*---------------------------------------------------------------
  Global Counters and Message Buffers
---------------------------------------------------------------*/
/* Global counter: number of messages successfully processed */
volatile unsigned long tm_isr_to_task_counter = 0;

/* Global counter: number of interrupts (i.e. ISR invocations) */
volatile unsigned long tm_isr_counter = 0;

/* Message buffers (adjust size as needed – here we use 16 bytes, which may be 32 bytes on some systems) */
unsigned long tm_message_sent[4];
unsigned long tm_message_received[4];

/*---------------------------------------------------------------
  Thread and ISR Prototypes
---------------------------------------------------------------*/
void tm_receiver_thread_entry(void* p1, void* p2, void* p3);
void tm_interrupt_simulator_thread_entry(void* p1, void* p2, void* p3);
void tm_report_thread_entry(void* p1, void* p2, void* p3);
void tm_isr_message_handler(void);
void tm_message_isr_to_task_initialize(void);

/*---------------------------------------------------------------
  Main Entry Point
---------------------------------------------------------------*/
int main_message_test(void)
{
   /* Initialize the RTOS and start the ISR-to-Task test */
   tm_initialize(tm_message_isr_to_task_initialize);
   return 0;
}

/*---------------------------------------------------------------
  Initialization Function
---------------------------------------------------------------*/
void tm_message_isr_to_task_initialize(void)
{
   /* Initialize the PMU for low-overhead cycle counting */
   tm_setup_pmu();

   /* Create a message queue with id 0 */
   tm_queue_create(0);

   /* Create and resume the Receiver Thread (moderate priority) */
   tm_thread_create(0, 10, tm_receiver_thread_entry);
   tm_thread_resume(0);

   /* Create and resume the Interrupt Simulator Thread (high priority) */
   tm_thread_create(1, 1, tm_interrupt_simulator_thread_entry);
   tm_thread_resume(1);

   /* Create and resume the Reporting Thread (intermediate priority) */
   tm_thread_create(2, 5, tm_report_thread_entry);
   tm_thread_resume(2);

   printf("[Init] ISR-to-Task Message Queue Benchmark started.\n");
}

/*---------------------------------------------------------------
  Receiver Thread
  - Blocks on the message queue.
  - Immediately after receiving a message, stops the PMU latency measurement.
  - Increments the message-processed counter.
---------------------------------------------------------------*/
void tm_receiver_thread_entry(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;

   while (1)
   {
      /* Block until a message is available from queue 0 */
      if (tm_queue_receive(0, tm_message_received))
      {
         /* Stop the PMU measurement that was started in the ISR */
         tm_pmu_profile_end("msg_latency");

         /* Increment the count of messages processed */
         tm_isr_to_task_counter++;
      }
   }
}

/*---------------------------------------------------------------
  Interrupt Simulator Thread
  - Initializes the message content.
  - Periodically raises a software interrupt that triggers the ISR.
---------------------------------------------------------------*/
void tm_interrupt_simulator_thread_entry(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;

   /* Initialize the message content with a known pattern */
   tm_message_sent[0] = 0xDEADBEEF;
   tm_message_sent[1] = 0xCAFEBABE;
   tm_message_sent[2] = 0xBAADF00D;
   tm_message_sent[3] = 0xFEEDFACE;

   while (1)
   {
      /* Sleep for a tick (or adjust as needed) to control the interrupt frequency */
      tm_thread_sleep(1);

      /* Raise a software interrupt. This will call tm_isr_message_handler() */
      tm_interrupt_raise();
   }
}

/*---------------------------------------------------------------
  ISR Message Handler
  - Called in interrupt context.
  - Increments the interrupt counter.
  - Starts the PMU latency measurement.
  - Sends the pre-defined message to the message queue.
  Note: Only ISR-safe RTOS functions should be used here.
---------------------------------------------------------------*/
void tm_isr_message_handler(void)
{
   /* Increment the ISR invocation counter. Just a sanity check*/
   tm_isr_counter++;

   /* Start PMU profiling for message latency.
      The measurement will run from here until the receiver thread stops it. */
   tm_pmu_profile_start("msg_latency");

   /* Send the message to the queue (non-blocking call) */
   tm_queue_send(0, tm_message_sent);
}

/*---------------------------------------------------------------
  Reporting Thread
  - Wakes up every TM_TEST_DURATION seconds.
  - Reports the number of messages and interrupts processed.
  - Prints the PMU profile result for the message latency ("msg_latency").
---------------------------------------------------------------*/
void tm_report_thread_entry(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;

   unsigned long last_message_count = 0;
   unsigned long last_interrupt_count = 0;
   unsigned long relative_time = 0;

   while (1)
   {
      /* Sleep for the defined test duration (in seconds) */
      tm_thread_sleep(TM_TEST_DURATION);
      relative_time += TM_TEST_DURATION;

      /* Calculate the number of messages and interrupts in this period */
      unsigned long messages = tm_isr_to_task_counter - last_message_count;
      unsigned long interrupts = tm_isr_counter - last_interrupt_count;

      /* Report the throughput and timing measurements */
      printf("**** ISR-to-Task Message Benchmark **** Relative Time: %lu seconds\n", relative_time);
      printf("Messages processed in period: %lu\n", messages);
      printf("Interrupts processed in period: %lu\n", interrupts);

      /* Print the PMU profiling result for the message latency.
         (A lower value indicates lower latency.) */
      tm_pmu_profile_print("msg_latency");
      printf("\n");

      /* Update counters for the next reporting period */
      last_message_count = tm_isr_to_task_counter;
      last_interrupt_count = tm_isr_counter;
   }
}

/*[EOF]************************************************************************/
