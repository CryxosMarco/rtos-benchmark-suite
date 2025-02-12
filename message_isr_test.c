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
  Global Counters and Message Buffers
---------------------------------------------------------------*/
/* Global counter: number of messages successfully processed */
volatile unsigned long tm_isr_to_task_counter = 0;

/* Global counter: number of interrupts (i.e. ISR invocations) */
volatile unsigned long tm_isr_counter = 0;

/* Message buffers */
unsigned long message_sent_arr[4];
unsigned long message_received_arr[4];

/*---------------------------------------------------------------
  Thread and ISR Prototypes
---------------------------------------------------------------*/
void tm_receiver_thread_entry(void* p1, void* p2, void* p3);
void tm_interrupt_simulator_thread_entry(void* p1, void* p2, void* p3);
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

   /* Initialize the message content with a known pattern */
   message_sent_arr[0] = 0xDEADBEEF;
   message_sent_arr[1] = 0xCAFEBABE;
   message_sent_arr[2] = 0xBAADF00D;
   message_sent_arr[3] = 0xFEEDFACE;

   /* Create a message queue with id 0 */
   tm_queue_create(0);

   /* Create and resume the Receiver Thread (moderate priority, id 0) */
   tm_thread_create(0, 5, tm_receiver_thread_entry);
   /* Create and resume the Interrupt Simulator Thread (high priority, id 1) */
   tm_thread_create(1, 1, tm_interrupt_simulator_thread_entry);

   tm_thread_resume(0);
   tm_thread_resume(1);

   printf("[Init] ISR-to-Task Message Queue Benchmark started.\n");
}

/*---------------------------------------------------------------
  Receiver Thread
  - Blocks on the message queue.
  - Immediately after receiving a message, stops the PMU latency measurement.
  - Increments the message-processed counter.
  - Compares the received message to the known send pattern.
  - If they match, suspends the interrupt simulator thread and outputs the final report.
---------------------------------------------------------------*/
void tm_receiver_thread_entry(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;

   while (1)
   {

      /* Block until a message is available from queue 0 */
      tm_queue_receive(0, message_received_arr);

      /* Stop the PMU measurement started in the ISR */
      tm_pmu_profile_end("msg_latency");

      /* Increment the processed message count */
      tm_isr_to_task_counter++;

      /* Check if the received message matches the expected pattern */
      int match = 1;
      for (int i = 0; i < 4; i++)
      {
         if (message_received_arr[i] != message_sent_arr[i])
         {
            match = 0;
            break;
         }
      }
      if (match)
      {

         /* Report the final benchmark results */
         printf("==== OneShot Benchmark Complete ====\n");
         printf("Total messages processed: %lu\n", tm_isr_to_task_counter);
         printf("Total interrupts processed: %lu\n", tm_isr_counter);
         tm_pmu_profile_print("msg_latency");
         printf("\n");

         break;
      }
   }

   /* Suspend this thread after finishing the report */
   tm_thread_suspend(0);
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

   /* Raise a software interrupt. This will call tm_isr_message_handler() */
   tm_interrupt_raise();
   /* Suspend the Interrupt Simulator Thread (assumed thread id 1) */
   tm_thread_exit(1);
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
   tm_isr_counter++;

   tm_pmu_profile_start("msg_latency");

   if (tm_queue_send(0, message_sent_arr) != 0)
   {
      printf("Message send gone wrong! \n");
   }
}

/*[EOF]************************************************************************/
