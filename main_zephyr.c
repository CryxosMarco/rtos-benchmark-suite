/*[CR]**************************************************************************
Copyright (c) 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
SPDX-License-Identifier: Apache-2.0
*/
/*[FH]**************************************************************************
PROJECT: MASTER THESIS
MODULE: Zephyr Main Thread
CONTENTS: Starts Main Thread and system initialization
*/
/*[CL]**************************************************************************
20-12-2024 Initial creation of the file

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
#ifdef USING_ZEPHYR

#include "tm_api.h"
#include <kernel/dpl/DebugP.h>
#include <kernel/dpl/TimerP.h>
#include <soc.h>
#include <stdlib.h>

/* Zephyr includes */

#define SOFTWARE_INTERRUPT_ID 10

#define MAIN_TASK_PRI (1)

#define MAIN_TASK_STACK_SIZE (8192U)

/* Define the time interval in seconds. This can be changed with a -D compiler option.  */
#ifndef TM_TEST_DURATION
#define TM_TEST_DURATION 30
#endif

LOG_MODULE_REGISTER(soft_irq);

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
   unsigned int irq = SOFTWARE_INTERRUPT_ID;

   /* Software-Interrupt auslösen */
   z_vim_arm_enter_irq(irq);
}

void setup_interrupt(void)
{
   unsigned int irq = SOFTWARE_INTERRUPT_ID;
   unsigned int priority = 1;

   /* Interrupt konfigurieren: Edge-triggered */
   z_vim_irq_priority_set(irq, priority, IRQ_TYPE_EDGE);

   /* Interrupt-Handler registrieren */
   IRQ_CONNECT(SOFTWARE_INTERRUPT_ID, 1, tm_interrupt_handler, NULL, 0);
   irq_enable(SOFTWARE_INTERRUPT_ID);

   /* Interrupt aktivieren */
   z_vim_irq_enable(irq);
}

void main_task(void* pvParameters)
{
   /* Start Thread-Metric tests */
   printk("Starting Thread-Metric tests...\n");

   /* Initialize custom interrupts*/
   test_interrupt_handler = tm_isr_message_handler;
   setup_interrupt();

   /* Call the main Thread-Metric function */
   main_message_test();

   /* Delete thread after completion */
   k_thread_abort(k_current_get());
}

/* Thread definition */
K_THREAD_DEFINE(main_thread, 512 /* STACKSIZE */, main_task, NULL, NULL, NULL, MAIN_TASK_PRI, K_USER, -1);

int rtos_main_zephyr(void)
{
   printk("Initializing Zephyr system...\n");

   /* Create main task */
   k_thread_start(main_thread);

   printk("Main task created and running...\n");

   return 0;
}

#endif
