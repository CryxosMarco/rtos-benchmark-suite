/*[CR]**************************************************************************
Copyright (c) Marco Milenkovic 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
SPDX-License-Identifier: Apache-2.0
*/
/*[FH]**************************************************************************
PROJECT: MASTER THESIS
MODULE: Message Queue from ISR Test
*/
/*[CL]**************************************************************************
 * Message Queue ISR-to-Task Benchmark (Single Threaded Test)
 *
 * This test uses a minimal ISR that generates a message with a fixed layout:
 *    [0] : Producer ID (fixed as 1)
 *    [1] : Message counter (iteration number)
 *    [2..MESSAGE_SIZE-2] : Pattern = 1000 + (iteration * 10) + index
 *    [MESSAGE_SIZE-1] : Checksum over the first (MESSAGE_SIZE-1) words
 *
 * The ISR performs a PMU measurement for the queue send latency using precomputed
 * PMU name strings. No prints or dynamic formatting occur in the ISR.
 *
 * A single thread drives the test: for each iteration it triggers the ISR via
 * tm_interrupt_raise(), then blocks on the message queue to receive the message,
 * measures the receive latency, validates the message, and continues.
 *
 * After all iterations, a summary report with the PMU results is printed.
 *
********************************************************************************/

#include "tm_api.h"
#include <stdio.h>
#include <string.h>

#define ITERATION_COUNT 32

/* Global counters */
volatile unsigned long tm_isr_to_task_counter = 0;
volatile unsigned long tm_isr_counter = 0;
volatile unsigned long isr_message_counter = 0; // Used by ISR as message index

unsigned long message_received_arr[MESSAGE_SIZE];
/* Use a static buffer for the isr to avoid stack corruption/overflow */
static unsigned long isr_message_buffer[MESSAGE_SIZE];

/* Precomputed PMU name arrays for send and receive iterations */
char pmu_send_names[ITERATION_COUNT][16];
char pmu_recv_names[ITERATION_COUNT][16];

/* Function prototypes */
void tm_receiver_thread_entry(void* p1, void* p2, void* p3);
void tm_isr_message_handler(void);
void tm_message_isr_to_task_initialize(void);
unsigned long compute_checksum(unsigned long* msg, int size);

/* Main entry point */
int main_message_isr_test(void)
{
   tm_initialize(tm_message_isr_to_task_initialize);
   return 0;
}

/* Compute additive checksum over 'size' elements */
unsigned long compute_checksum(unsigned long* msg, int size)
{
   unsigned long checksum = 0;
   for (int i = 0; i < size; i++)
   {
      checksum += msg[i];
   }
   return checksum;
}

/* Initialization function */
void tm_message_isr_to_task_initialize(void)
{
   int i;
   tm_setup_pmu();
   tm_queue_create(0);

   /* Precompute PMU names for each iteration so the ISR can avoid runtime formatting */
   for (i = 0; i < ITERATION_COUNT; i++)
   {
      snprintf(pmu_send_names[i], sizeof(pmu_send_names[i]), "S%02d", i);
      snprintf(pmu_recv_names[i], sizeof(pmu_recv_names[i]), "R%02d", i);
   }

   /* Create the single receiver thread that drives the entire test */
   tm_thread_create(0, 5, tm_receiver_thread_entry);
   tm_thread_resume(0);
}

/* Minimal ISR: No prints, no dynamic formatting */
void tm_isr_message_handler(void)
{
   int i;
   tm_isr_counter++;

   unsigned long* message = isr_message_buffer;

   /* Generate message:
      [0] : Producer ID (1)
      [1] : Message counter (isr_message_counter)
      [2..MESSAGE_SIZE-2] : Pattern = 1000 + (isr_message_counter * 10) + index
      [MESSAGE_SIZE-1] : Checksum over first (MESSAGE_SIZE-1) words */
   message[0] = 1;
   message[1] = isr_message_counter;
   for (i = 2; i < MESSAGE_SIZE - 1; i++)
   {
      message[i] = 1000 + (isr_message_counter * 10) + i;
   }
   message[MESSAGE_SIZE - 1] = compute_checksum(message, MESSAGE_SIZE - 1);

   /* Measure send latency using a precomputed PMU name */
   tm_pmu_profile_start(pmu_send_names[isr_message_counter]);
   tm_queue_send_from_isr(0, message);
   // tm_pmu_profile_end(pmu_send_names[isr_message_counter]);

   isr_message_counter++; /* Prepare for next iteration */
}

/* Receiver thread:
   Drives the test by triggering the ISR, measuring the receive latency,
   validating the received message, and finally reporting the results. */
void tm_receiver_thread_entry(void* p1, void* p2, void* p3)
{
   (void) p1;
   (void) p2;
   (void) p3;
   int i, j;
   char valid;

   for (i = 0; i < ITERATION_COUNT; i++)
   {
      /* Trigger the ISR which will generate and send the message */
      tm_interrupt_raise();

      /* Measure receive latency using a precomputed PMU name */
      // tm_pmu_profile_start(pmu_recv_names[i]);
      tm_queue_receive(0, message_received_arr);
      tm_pmu_profile_end(pmu_send_names[i]);
      // tm_pmu_profile_end(pmu_recv_names[i]);

      tm_isr_to_task_counter++;

      /* Validate the received message.
         Expected layout:
            [0] : 1
            [1] : iteration number (i)
            [2..MESSAGE_SIZE-2] : 1000 + (i*10) + index
            [MESSAGE_SIZE-1] : checksum over first (MESSAGE_SIZE-1) words
      */
      valid = 1;

      /* Check fixed fields */
      if (message_received_arr[0] != 1 || message_received_arr[1] != (unsigned long) i)
      {
         valid = 0;
      }

      /* Check the pattern fields */
      for (j = 2; j < MESSAGE_SIZE - 1; j++)
      {
         if (message_received_arr[j] != 1000 + (i * 10) + j)
         {
            valid = 0;
            break;
         }
      }

      /* Check the checksum */
      if (compute_checksum(message_received_arr, MESSAGE_SIZE - 1) != message_received_arr[MESSAGE_SIZE - 1])
      {
         valid = 0;
         printf("Message integrity error in iteration %d: checksum mismatch\r\n", i);
      }

      if (!valid)
      {
         /* Optionally record the error state */
         // For example: error_counter++;
      }
   }

   /* After all iterations, report the results */
   printf("==== ISR-to-Task Benchmark Complete ====\r\n");
   printf("Total messages processed: %lu\r\n", tm_isr_to_task_counter);
   printf("Total interrupts processed: %lu\r\n", tm_isr_counter);

   /* Print PMU results for measurements */
   for (i = 0; i < ITERATION_COUNT; i++)
   {
      // printf("Receive Latency: ");
      // tm_pmu_profile_print(pmu_recv_names[i]);
      printf("Send Latency: ");
      tm_pmu_profile_print(pmu_send_names[i]);
   }

   tm_thread_suspend(0);
}
