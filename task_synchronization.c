/*[CR]**************************************************************************
Copyright (c) IBV - Echtzeit- und Embedded GmbH & Co. KG
All Rights reserved.
*/
/*[FH]**************************************************************************
PROJECT: MASTER THESIS
MODULE: SYNCHRONISATION TEST
CONTENTS: Short description of the file content
*
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
#if defined(USING_FREERTOS) || defined(USING_THREADX)
#include "ti_board_open_close.h"
#include "ti_drivers_open_close.h"
#endif
#include "tm_api.h"
#include <stdio.h>
#include <string.h>

/* We define a unique mutex ID for the UART resource. */
#define UART_MUTEX_ID 1

/********************************************************************************
 * OPEN/CLOSE/PUTCH: Abstract "UART" usage
 * TODO: Replace these with real UART driver calls in a real RTOS environment.
 ********************************************************************************/

/* Acquire the UART mutex so only one task prints at a time. */
static int OpenUART(void) {
  /* Ensure the mutex is available. If not, we'd potentially block or return
   * error. */
  if (tm_mutex_get(UART_MUTEX_ID) == TM_SUCCESS) {
    return TM_SUCCESS;
  } else {
    return TM_ERROR;
  }
}

/* Write one character (simulated) to the UART by printing. */
static void putch(char c) {
#ifndef USING_ZEPHYR /* when using ThreadX or FreeRTOS via CCS */
  Drivers_open();
  Board_driversOpen();

  DebugP_log("%c", c);

  Board_driversClose();
  Drivers_close();
#else
  printk("%c", c); /* using Zephyr api for UART */
#endif
}

/* Release the UART mutex after finishing the print. */
static void CloseUART(void) { tm_mutex_put(UART_MUTEX_ID); }

/********************************************************************************
 * EXAMPLE TASKS
 * Two tasks that each open the UART, print some text, then close it.
 * TODO: YIELD or SLEEP to allow other tasks to run. TEST both
 ********************************************************************************/

/* First writer task: prints "Hello from WriterTask1" a few times. */
static void vWriterTask1(void *arg1, void *arg2, void *arg3) {
  static int callCount = 0;

  if (callCount < 3) {
    if (OpenUART() == TM_SUCCESS) {
      printf("[WriterTask1] Printing...\n");
      const char *msg = "Hello from WriterTask1!\r\n";
      for (const char *p = msg; *p; p++) {
        putch(*p);
      }
      CloseUART();
      callCount++;
    }

    /* Optional: relinquish or sleep so other tasks can run. */
    tm_thread_relinquish();
  } else {
    printf("[WriterTask1] Done. Suspending.\n");
    tm_thread_suspend(1); /* Suspends itself (thread ID 1). */
  }
}

/* Second writer task: prints a different message. */
static void vWriterTask2(void *arg1, void *arg2, void *arg3) {
  static int callCount = 0;

  if (callCount < 3) {
    if (OpenUART() == TM_SUCCESS) {
      printf("[WriterTask2] Printing...\n");
      const char *msg = ">>> WriterTask2 says hi.\r\n";
      for (const char *p = msg; *p; p++) {
        putch(*p);
      }
      CloseUART();
      callCount++;
    }

    /* Optional: relinquish or sleep. */
    tm_thread_relinquish();
  } else {
    printf("[WriterTask2] Done. Suspending.\n");
    tm_thread_suspend(2);
  }
}

/********************************************************************************
 * TEST INITIALIZATION FUNCTION
 * Called by tm_initialize() from main().
 * This function sets up the mutex and creates/resumes the tasks.
 ********************************************************************************/
static void task_synchronisation_initialize(void) {
  /* Create the UART mutex. */
  tm_mutex_create(UART_MUTEX_ID);

  /* Create two tasks, each with a unique ID. Priority is arbitrary. */
  tm_thread_create(1, 5, vWriterTask1);
  tm_thread_create(2, 5, vWriterTask2);

  /* Start (resume) both tasks. */
  tm_thread_resume(1);
  tm_thread_resume(2);
}

/********************************************************************************
 * MAIN ENTRY POINT
 ********************************************************************************/
int main_sync(void) {
  printf("[Main] Starting Abstract Example.\n");

  /* Call tm_initialize(), passing our task_synchronisation_initialize.
   * The real implementation of tm_initialize() will do RTOS setup,
   * then call task_synchronisation_initialize(), then start scheduling tasks.
   */
  tm_initialize(task_synchronisation_initialize);

  /* In many RTOSes, tm_initialize() might not return. If it does here,
   * we just print a message. */
  printf("[Main] tm_initialize returned, example end.\n");

  return 0;
}

/*[EOF]************************************************************************/
