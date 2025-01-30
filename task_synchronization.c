/*[CR]**************************************************************************
Copyright (c) 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
SPDX-License-Identifier: Apache-2.0
*/
/*[FH]**************************************************************************
PROJECT: MASTER THESIS
MODULE: SYNCHRONISATION TEST
CONTENTS: Short description of the file content
*/
/*[CL]**************************************************************************
21-01-2025 MMI Initial creation of the file

---
MMI: Marco Milenkovic, IBV, Milenkovic@ibv-augsburg.de
*/
/*[MP]**************************************************************************
 * Synchronistation Promblem example using tm_api.h
 *
 * Demonstrates:
 *   - How to set up a simple "UART" usage protected by a mutex
 *   - Creating/resuming tasks
 *   - Printing output with printf (simulating UART output)
 *
 *
 ******************************************************************************/
/*******************************************************************************
 * includes
 ******************************************************************************/

#include "tm_api.h"
#include <stdio.h>
#include <string.h>

/* We define a unique mutex ID for the UART resource. */
#define UART_MUTEX_ID 1
#define SEM_A 1
#define SEM_B 2

/********************************************************************************
 * OPEN/CLOSE/PUTCH: Abstract "UART" usage
 ********************************************************************************/

/* Acquire the UART mutex so only one task prints at a time. */
static int OpenUART(void)
{
   /* Ensure the mutex is available.  */
   return tm_mutex_get(UART_MUTEX_ID) == TM_SUCCESS ? TM_SUCCESS : TM_ERROR;
}

/* Write one character (simulated) to the UART by printing. */
static void putch(char c)
{
#ifndef USING_ZEPHYR /* when using ThreadX or FreeRTOS via CCS */
   printf("%c", c);
#else
   printk("%c", c); /* using Zephyr api for UART */
#endif
}

/* Release the UART mutex after finishing the print. */
static void CloseUART(void)
{
   tm_mutex_put(UART_MUTEX_ID);
}

/********************************************************************************
 * TEST TASKS
 * Two tasks that get synchrinized by a semaphore and access the UART as shared resource
 * TODO: YIELD or SLEEP to allow other tasks to run. TEST both
 ********************************************************************************/

/* First writer task: prints "Hello from WriterTask1" a few times. */
static void WriterTask1(void* arg1, void* arg2, void* arg3)
{
   static int callCount = 0;
   while (1)
   {
      /* Wait for semaphore to be available */
      tm_semaphore_wait(SEM_A);

      if (callCount < 3)
      {
         if (OpenUART() == TM_SUCCESS)
         {
            printf("[WriterTask1] Printing...\n");
            const char* msg = "Hello from WriterTask1!\r\n";
            for (const char* p = msg; *p; p++)
            {
               putch(*p);
            }
            CloseUART();
            callCount++;
         }
         /* Release the second semaphore */
         tm_semaphore_put(SEM_B);
      }
      else
      {
         printf("[WriterTask1] Done. Suspending.\n");
         tm_semaphore_put(SEM_B);
         tm_thread_suspend(1); /* Suspends itself (thread ID 1). */
      }
   }
}

/* Second writer task: prints a different message. */
static void WriterTask2(void* arg1, void* arg2, void* arg3)
{
   (void) arg1;
   (void) arg2;
   (void) arg3;

   static int callCount = 0;

   while (1)
   {
      /* Block on the same semaphore. */
      tm_semaphore_wait(SEM_B);

      if (callCount < 3)
      {
         if (OpenUART() == TM_SUCCESS)
         {
            printf("[WriterTask2] Printing...\n");
            const char* msg = "WriterTask2 says hi!\r\n";
            for (const char* p = msg; *p; p++)
            {
               putch(*p);
            }
            CloseUART();
            callCount++;
         }
         /* Release the first semaphore */
         tm_semaphore_put(SEM_A);
      }
      else
      {
         printf("[WriterTask2] Done. Suspending.\n");
         tm_semaphore_put(SEM_A);
         tm_thread_suspend(2);
      }
   }
}

/********************************************************************************
 * TEST INITIALIZATION FUNCTION
 * Called by tm_initialize() from main().
 * This function sets up the mutex and creates/resumes the tasks.
 ********************************************************************************/
static void task_synchronisation_initialize(void)
{
   /* Create the UART mutex. */
   tm_mutex_create(UART_MUTEX_ID);
   /* Create the semaphore to snychronize tasks.
      The implementation calls a semaphore_get()
      to make it available from the start */
   tm_semaphore_create(SEM_A);
   tm_semaphore_create(SEM_B);
   /* guarantee that the semaphore A is blocked so Task2 can start first.*/
   tm_semaphore_get(SEM_A);

   /* Create two tasks, each with a unique ID. Priority is arbitrary.
      Without synchronisation Task1 would start operating before task2.
      we use synchronisation to ensure that task2 can finish printing first*/
   tm_thread_create(1, 5, WriterTask1);
   tm_thread_create(2, 5, WriterTask2);

   /* Start (resume) both tasks. */
   tm_thread_resume(1);
   tm_thread_resume(2);
}

/********************************************************************************
 * MAIN ENTRY POINT
 ********************************************************************************/
int main_sync(void)
{
   printf("[Main] Starting Synchronisation Test.\n");

   /* Call tm_initialize(), passing our task_synchronisation_initialize.
    * The real implementation of tm_initialize() will do RTOS setup,
    * then call task_synchronisation_initialize(), then start scheduling tasks.
    */
   tm_initialize(task_synchronisation_initialize);

   /* In many RTOSes, tm_initialize() might not return. If it does here,
    * we just print a message. */
   printf("[Main] tm_initialize returned, threads started.\n");

   return 0;
}

/*[EOF]************************************************************************/
