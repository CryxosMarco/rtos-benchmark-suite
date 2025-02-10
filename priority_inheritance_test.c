/*[CR]******************************************************************************
 *   Priority Inheritance Test Program
 *
 * Copyright (c) 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 *
 * This program demonstrates how three tasks of different priorities (High,
 * Medium, Low) share a mutex-protected resource to trigger and observe
 * priority inheritance on an RTOS that implements it (e.g., FreeRTOS).
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

/* Define a unique mutex ID for our shared resource */
#define SHARED_MUTEX_ID 1

/* Define Task IDs and their priorities */
#define HIGH_TASK_ID   0
#define HIGH_TASK_PRIO 5    /* High priority */

#define MED_TASK_ID    1
#define MED_TASK_PRIO  10   /* Medium priority */

#define LOW_TASK_ID    2
#define LOW_TASK_PRIO  20   /* Low priority */

/* For real measurments you want to disable prints in critical loops 
   But for ensuring the correct execution we can add these define 
   to have debugging prints */
// #define DEBUG_PRIO_INHERITANCE_ON 

/*******************************************************************************
 * Low Priority Task
 *
 * This task acquires the mutex first and then simulates a long critical section.
 * Its busy loop will delay releasing the mutex, forcing the high‑priority task to block.
 ******************************************************************************/
static void LowPrioTask(void* p1, void* p2, void* p3)
{
    (void)p1;
    (void)p2;
    (void)p3;

    if (tm_mutex_get(SHARED_MUTEX_ID) == TM_SUCCESS)
    {
#ifdef DEBUG_PRIO_INHERITANCE_ON
        printf("[LowPrioTask] Mutex acquired. Performing long work...\n");
#endif
        /* Simulate a long critical section with a busy loop.
         * This loop is intentionally "heavy" to force a delay.
         */
        volatile unsigned long dummy = 0;
        for (volatile unsigned long i = 0; i < 10000000UL; i++)
        {
            dummy += (i % 3);
            __asm__ volatile("" ::: "memory");  // Prevent optimization.
        }
#ifdef DEBUG_PRIO_INHERITANCE_ON
        printf("[LowPrioTask] Work done. Releasing mutex.\n");
#endif
        tm_mutex_put(SHARED_MUTEX_ID);
    }
    else
    {
        printf("[LowPrioTask] Failed to acquire mutex.\n");
    }
    
    tm_thread_suspend(LOW_TASK_ID);
}

/*******************************************************************************
 * High Priority Task
 *
 * This task waits briefly so that the low‑priority task acquires the mutex.
 * Then it starts the PMU profile just before trying to get the mutex.
 * Once the mutex is finally acquired (i.e. after any inheritance boosting),
 * it stops the PMU profile. The difference (reported by the PMU) is the block time.
 ******************************************************************************/
static void HighPrioTask(void* p1, void* p2, void* p3)
{
    (void)p1;
    (void)p2;
    (void)p3;

    /* Sleep briefly to let LowPrioTask acquire the mutex first */
    tm_thread_sleep(1);

    printf("[HighPrioTask] Attempting to acquire mutex. Starting PMU measurement.\n");
#ifndef DEBUG_PRIO_INHERITANCE_ON /* Debug prints are off */
    printf("Measuring Mode enabled: Prints inside critical loop are deactivated.\n");
#endif

    /* Start PMU profiling before attempting to acquire the mutex */
    tm_pmu_profile_start("HP_block");

    if (tm_mutex_get(SHARED_MUTEX_ID) == TM_SUCCESS)
    {
        /* Once acquired, stop the PMU measurement */
        tm_pmu_profile_end("HP_block");

        /* Print the profiling result.
           A lower printed value means the block duration was shorter. */
        tm_pmu_profile_print("HP_block");

        /* Optionally hold the mutex briefly and then release it */
      //   tm_thread_sleep(1);
        tm_mutex_put(SHARED_MUTEX_ID);
    }
    else
    {
        printf("[HighPrioTask] Failed to acquire mutex.\n");
    }

    tm_thread_suspend(HIGH_TASK_ID);
}

/*******************************************************************************
 * Medium Priority Task
 *
 * This task runs concurrently to simulate interference.
 * Without effective priority inheritance, its execution might delay the low‑priority task,
 * resulting in a longer block time for the high‑priority task.
 ******************************************************************************/
static void MedPrioTask(void* p1, void* p2, void* p3)
{
    (void)p1;
    (void)p2;
    (void)p3;

    int count = 0;
    while (1)
    {
        printf("[MedPrioTask] Running... (count=%d)\n", ++count);
        tm_thread_sleep(1);

        if (count > 4)
        {
            printf("[MedPrioTask] Finished. Suspending.\n");
            tm_thread_suspend(MED_TASK_ID);
        }
    }
}

/*******************************************************************************
 * Priority Inheritance Test Initialization
 *
 * This function sets up the PMU, creates the mutex and tasks, and then resumes them.
 ******************************************************************************/
static void tm_priority_inheritance_initialize(void)
{
    /* Initialize and configure the PMU */
    tm_setup_pmu();

    /* Create the shared mutex */
    tm_mutex_create(SHARED_MUTEX_ID);

    /* Create the tasks */
    tm_thread_create(LOW_TASK_ID, LOW_TASK_PRIO, LowPrioTask);
    tm_thread_create(MED_TASK_ID, MED_TASK_PRIO, MedPrioTask);
    tm_thread_create(HIGH_TASK_ID, HIGH_TASK_PRIO, HighPrioTask);

    /* Resume the tasks */
    tm_thread_resume(LOW_TASK_ID);
    tm_thread_resume(MED_TASK_ID);
    tm_thread_resume(HIGH_TASK_ID);

    printf("[Init] Priority Inheritance test started.\n");
}

/*******************************************************************************
 * Main Entry Point
 *
 * Initializes the RTOS and starts the test.
 ******************************************************************************/
int main_inheritance(void)
{
    tm_initialize(tm_priority_inheritance_initialize);
    return 0;
}
