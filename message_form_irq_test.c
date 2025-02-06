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

#define TM_TEST_DURATION 3
/* Global counter for the number of messages processed (shared between ISR and receiver) */
unsigned long tm_isr_to_task_counter = 0;
/* Global Counter for Interrupt occurrence */
volatile unsigned long tm_isr_counter;

/* 16-byte message */
unsigned long tm_message_sent[4];
unsigned long tm_message_received[4];

/* Thread prototypes */
void tm_receiver_thread_entry(void* p1, void* p2, void* p3);
void tm_interrupt_simulator_thread_entry(void* p1, void* p2, void* p3);
void tm_report_thread_entry(void* p1, void* p2, void* p3);

/* ISR handler prototype */
void tm_isr_message_handler(void);

/* Initialization prototype */
void tm_message_isr_to_task_initialize(void);

/* Main entry point for the ISR-to-Task benchmark */
int main_isr_to_task(void)
{
   /* Initialize the test */
   tm_initialize(tm_message_isr_to_task_initialize);
   return 0;
}

/*
 * Initialization function to create the message queue and the threads.
 * Thread priorities are chosen to simulate the real-time environment:
 *  - Receiver Thread: moderate priority (10)
 *  - Interrupt Simulator Thread: high priority (1) to emulate interrupt context
 *  - Reporting Thread: intermediate priority (5)
 */
void tm_message_isr_to_task_initialize(void)
{
   /* Create a message queue (queue id 0) */
   tm_queue_create(0);

   /* Create and resume the receiver thread */
   tm_thread_create(0, 10, tm_receiver_thread_entry);
   tm_thread_resume(0);

   /* Create and resume the interrupt simulator thread */
   tm_thread_create(1, 1, tm_interrupt_simulator_thread_entry);
   tm_thread_resume(1);

   /* Create and resume the reporting thread */
   tm_thread_create(2, 5, tm_report_thread_entry);
   tm_thread_resume(2);
}

/*
 * Receiver Thread:
 * This thread blocks on the message queue waiting for messages.
 * Each received message increments the global counter.
 */
void tm_receiver_thread_entry(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;

   while (1)
   {
      /* Block until a message is available in queue 0 */
      if (tm_queue_receive(0, tm_message_received))
      {
         /* Increment the message counter upon successful receipt */
         tm_isr_to_task_counter++;
      }
   }
}

/*
 * Interrupt generating Thread:
 * This thread simulates periodic interrupts by sleeping briefly and then
 * raising a software interrupt via tm_interrupt_raise(). When raised,
 * the RTOS will call tm_isr_message_handler().
 */
void tm_interrupt_simulator_thread_entry(void* p1, void* p2, void* p3)
{
   /* Initialize the message contents to a known 16-byte pattern */
   tm_message_sent[0] = 0xDEADBEEF;
   tm_message_sent[1] = 0xCAFEBABE;
   tm_message_sent[2] = 0xBAADF00D;
   tm_message_sent[3] = 0xFEEDFACE;

   while (1)
   {
      /* Sleep for 10 tick to control the interrupt frequency. */
      tm_thread_sleep(0);

      /* Raise a software interrupt that triggers tm_isr_message_handler() */
      tm_interrupt_raise();
   }
}

/*
 * ISR Message Handler:
 * This function runs in interrupt context (triggered via tm_interrupt_raise)
 * and sends the pre-defined message to the queue.
 * It must be non-blocking and use only ISR-safe RTOS functions.
 */
void tm_isr_message_handler(void)
{
   /*Increase a Counter to have a reference */
   tm_isr_counter++;
   /* Send the message to the queue */
   tm_queue_send(0, tm_message_sent);
}

/*
 * Reporting Thread:
 * This thread wakes up every TM_TEST_DURATION seconds to report how many messages
 * have been processed in that period. It calculates the throughput based on the
 * global counter.
 */
void tm_report_thread_entry(void* p1, void* p2, void* p3)
{
   unsigned long last_counter = 0;
   unsigned long relative_time = 0;
   unsigned long last_interrupt_numbers = 0;

   while (1)
   {
      /* Sleep for TM_TEST_DURATION seconds */
      tm_thread_sleep(TM_TEST_DURATION);
      relative_time += TM_TEST_DURATION;

      /* Calculate the number of messages processed during this interval */
      unsigned long messages = tm_isr_to_task_counter - last_counter;
      unsigned long interrupts = tm_isr_counter - last_interrupt_numbers;
      printf("**** ISR-to-Task Message Benchmark **** Relative Time: %lu seconds\n", relative_time);
      printf("Messages processed in period: %lu\n", messages);
      printf("Interrupts processed in period: %lu\n\n", interrupts);

      last_counter = tm_isr_to_task_counter;
      last_interrupt_numbers = tm_isr_counter;
   }
}

/*[EOF]************************************************************************/
