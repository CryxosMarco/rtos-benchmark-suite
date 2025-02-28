/*[CR]**************************************************************************
Copyright (c) 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
SPDX-License-Identifier: Apache-2.0
*/
/*[FH]**************************************************************************
PROJECT: MASTER THESIS
MODULE: ThreadX Main Thread
CONTENTS: Starts Main Thread and system initialization
*/
/*[CL]**************************************************************************
14-11-2024 Initial creation of the file

---
MMI: Marco Milenkovic, IBV, Milenkovic@ibv-augsburg.de
*/
/*[MP]**************************************************************************
 * ThreadX entry point for the test application
 *
 *
 ******************************************************************************/
/*******************************************************************************
 * includes
 ******************************************************************************/
#include "rtos_config.h"
#ifdef USING_THREADX

#include "ti_board_config.h"
#include "ti_drivers_config.h"
#include "tm_api.h"
#include <HwiP.h>
#include <kernel/dpl/DebugP.h>
#include <stdio.h>
#include <stdlib.h>

/* ThreadX includes */
#include <tx_api.h>

#define MAIN_TASK_PRI (1)

/* Choose an appropriate interrupt number */
#define SOFTWARE_INTERRUPT_ID 10

#define MAIN_TASK_STACK_SIZE (8192U)

/* Define the time interval in seconds. This can be changed with a -D compiler
 * option.  */
#ifndef TM_TEST_DURATION
#define TM_TEST_DURATION 30
#endif

uint8_t main_thread_stack[MAIN_TASK_STACK_SIZE] __attribute__((aligned(32)));

TX_THREAD main_thread;

void* test_interrupt_handler = NULL;

/* Define the interrupt handler */
void tm_interrupt_handler(void* args)
{
   if (test_interrupt_handler != NULL)
   {
      /* Call the assigned handler function */
      ((void (*)(void)) test_interrupt_handler)();
   }
}

/* Function to trigger the software interrupt */
void tm_interrupt_raise(void)
{
   /* Trigger the software interrupt */
   HwiP_post(SOFTWARE_INTERRUPT_ID);
}

void setup_interrupt(void)
{
   HwiP_Params hwiParams;
   HwiP_Params_init(&hwiParams);

   hwiParams.intNum = SOFTWARE_INTERRUPT_ID;  /* Chosen interrupt ID */
   hwiParams.callback = tm_interrupt_handler; /* Interrupt handler, change for
                                                 test accordingly */
   hwiParams.priority = 1;                    /* Set a valid priority */
   hwiParams.isFIQ = false;

   HwiP_Object hwiObj;
   if (HwiP_construct(&hwiObj, &hwiParams) != SystemP_SUCCESS)
   {
      printf("Failed to register interrupt\r\n");
      while (1)
         ;
   }

   HwiP_enableInt(SOFTWARE_INTERRUPT_ID); /* Enable this interrupt */
   HwiP_enable();                         /* Enable global interrupts */
}

void threadx_main(ULONG arg)
{
   main_message_isr_test(); /* Startet den Benchmark-Test */
}

int rtos_main_threadx(void)
{
   /* init SOC specific modules */
   System_init();
   Board_init();
   Drivers_open();
   Board_driversOpen();
   /* Initialize our UART application parameters */
   /* enable this when interrupts are needed. */
   test_interrupt_handler = tm_isr_message_handler;
   setup_interrupt();

   /* Enter the ThreadX kernel.  */
   tx_kernel_enter();
   return 0;
}

void tx_application_define(void* first_unused_memory)
{
   UINT status;

   printf("Initializing ThreadX system...\r\n");

   printf("Starting Main Thread...\r\n");

   status = tx_thread_create(&main_thread,         /* Pointer to the main thread object. */
                             "main_thread",        /* Name of the task for debugging purposes. */
                             threadx_main,         /* Entry function for the main thread. */
                             0,                    /* Arguments passed to the entry function. */
                             main_thread_stack,    /* Main thread stack. */
                             MAIN_TASK_STACK_SIZE, /* Main thread stack size in bytes. */
                             MAIN_TASK_PRI,        /* Main task priority. */
                             MAIN_TASK_PRI,        /* Highest priority level of disabled preemption. */
                             TX_NO_TIME_SLICE,     /* No time slice. */
                             TX_AUTO_START);       /* Start immediately. */

   DebugP_assertNoLog(status == TX_SUCCESS);
}

#endif /* USING_THREADX */
