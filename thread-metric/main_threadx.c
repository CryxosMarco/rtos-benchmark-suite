/*
 *  Copyright (C) 2018-2024 Texas Instruments Incorporated
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifdef USING_THREADX

#include <stdlib.h>
#include <kernel/dpl/DebugP.h>
#include "ti_drivers_config.h"
#include "ti_board_config.h"
#include <tm_api.h>
#include <HwiP.h>
#include <stdio.h>

/* ThreadX includes */
#include <tx_api.h>

#define MAIN_TASK_PRI (1)

// Choose an appropriate interrupt number
#define SOFTWARE_INTERRUPT_ID 10 // Example interrupt number

#define MAIN_TASK_STACK_SIZE (8192U)

/* Define the time interval in seconds. This can be changed with a -D compiler option.  */
#ifndef TM_TEST_DURATION
#define TM_TEST_DURATION 30
#endif

uint8_t main_thread_stack[MAIN_TASK_STACK_SIZE] __attribute__((aligned(32)));

TX_THREAD main_thread;

extern void *test_interrupt_handler = NULL;

// Define the interrupt handler
void tm_interrupt_handler(void *args)
{
    if (test_interrupt_handler != NULL)
    {
        // Call the assigned handler function
        ((void (*)(void))test_interrupt_handler)();
    }
}

// Function to trigger the software interrupt
void tm_interrupt_raise(void)
{
    // Trigger the software interrupt
    HwiP_post(SOFTWARE_INTERRUPT_ID);
}

void setup_interrupt(void)
{
    HwiP_Params hwiParams;
    HwiP_Params_init(&hwiParams);

    hwiParams.intNum = SOFTWARE_INTERRUPT_ID;             // Chosen interrupt ID
    hwiParams.callback = tm_interrupt_preemption_handler; // Interrupt handler, change for test accordingly
    hwiParams.priority = 1;                               // Set a valid priority
    hwiParams.isFIQ = false;

    HwiP_Object hwiObj;
    if (HwiP_construct(&hwiObj, &hwiParams) != SystemP_SUCCESS)
    {
        printf("Failed to register interrupt\n");
        while (1)
            ;
    }

    HwiP_enableInt(SOFTWARE_INTERRUPT_ID); // Enable this interrupt
    HwiP_enable();                         // Enable global interrupts
}

void benchmark_main(ULONG arg)
{
    tm_main_two(); // Startet den Benchmark-Test
}

void threadx_main(ULONG arg)
{
    benchmark_main(arg);
}

int rtos_main_threadx(void)
{
    /* init SOC specific modules */
    System_init();
    Board_init();
    /* enable this when interrupts are needed. */
    // test_interrupt_handler = tm_interrupt_processing_handler;
    // setup_interrupt();

    /* Enter the ThreadX kernel.  */
    tx_kernel_enter();
}

void tx_application_define(void *first_unused_memory)
{
    UINT status;

    printf("Initializing ThreadX system...\n");

    printf("Starting Main Thread...\n");

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