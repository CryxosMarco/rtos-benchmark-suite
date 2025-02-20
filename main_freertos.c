/*[CR]**************************************************************************
Copyright (C) 2018-2024 Texas Instruments Incorporated
Copyright (c) 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
SPDX-License-Identifier: Apache-2.0*/
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
#ifdef USING_FREERTOS

#include "ti_board_config.h"
#include "ti_drivers_config.h"
#include "tm_api.h"
#include <HwiP.h>
#include <kernel/dpl/DebugP.h>
#include <kernel/dpl/TimerP.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

#define SOFTWARE_INTERRUPT_ID 10

#define MAIN_TASK_PRI (30)

#define MAIN_TASK_STACK_SIZE (8192U)

/* Define the time interval in seconds. This can be changed with a -D compiler option.  */
#ifndef TM_TEST_DURATION
#define TM_TEST_DURATION 30
#endif

uint8_t main_thread_stack[MAIN_TASK_STACK_SIZE] __attribute__((aligned(32)));

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

   /* Initialize interrupt parameters */
   HwiP_Params_init(&hwiParams);

   /* Set the interrupt parameters */
   hwiParams.intNum = SOFTWARE_INTERRUPT_ID;  /* Chosen interrupt ID */
   hwiParams.callback = tm_interrupt_handler; /* Interrupt handler */
   hwiParams.priority = 1;                    /* Set a valid priority (lower is higher priority) */
   hwiParams.isFIQ = false;                   /* This is an IRQ, not an FIQ */

   static HwiP_Object hwiObj; /* Use static to ensure the object persists */

   /* Construct the interrupt */
   if (HwiP_construct(&hwiObj, &hwiParams) != SystemP_SUCCESS)
   {
      printf("Failed to register interrupt\n");
      while (1)
         ; /* Halt if interrupt registration fails */
   }

   /* Enable the specific interrupt and global interrupts */
   HwiP_enableInt(SOFTWARE_INTERRUPT_ID);
   HwiP_enable();
}

void main_task(void* pvParameters)
{

   /* Start Thread-Metric tests */
   printf("Starting Thread-Metric tests...\n");
   test_interrupt_handler = tm_isr_message_handler;
   setup_interrupt();
   main_pmu();

   /* Delete this task when finished */
   vTaskDelete(NULL);
}

int rtos_main_freertos(void)
{
   printf("Initializing FreeRTOS system...\n");
   /* Initialize board and system */
   System_init();
   Board_init();

   /* Create main task */
   BaseType_t status = xTaskCreate(main_task, "MainTask", MAIN_TASK_STACK_SIZE, NULL, MAIN_TASK_PRI, NULL);

   if (status != pdPASS)
   {
      /* Handle task creation failure */
      DebugP_assert(status == pdPASS);
   }

   /* Start the FreeRTOS scheduler */
   vTaskStartScheduler();

   /* If the scheduler returns, it indicates an error */
   for (;;)
   {
      /* printf("Scheduler returned unexpectedly\n"); */
   }
}

#endif /* USING_FREERTOS */
