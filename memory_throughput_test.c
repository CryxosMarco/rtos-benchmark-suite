/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/

/**************************************************************************/
/*                                                                        */
/*  Memory Throughput Test for MSRAM Performance under FreeRTOS           */
/*  Using the PMU API for cycle measurement                               */
/*                                                                        */
/**************************************************************************/

#include "tm_api.h"
#include <stdio.h>
#include <string.h>

/* Define test parameters. */
#define BUFFER_SIZE (10 * 1024)     // 10 KB buffer
#define THROUGHPUT_ITERATIONS 10000 // Number of memcpy iterations per test run
#define TM_TEST_DURATION 1          // Sleep period for reporting (in milliseconds)
#define ITERATION_COUNT 32 

/* PMU name used for the throughput measurement. */
#define PMU_NAME "MEMTST"

/* Place buffers in MSRAM using your linker script section. */
unsigned char msram_src[BUFFER_SIZE];  //__attribute__((section("R5F_TCMA")));
unsigned char msram_dest[BUFFER_SIZE]; //__attribute__((section("R5F_TCMA")));

/* Precomputed PMU name arrays for send and receive iterations */
char pmu_memory_test[ITERATION_COUNT][16];

/* Thread prototypes. */
void tm_memory_throughput_thread_entry(void* p1, void* p2, void* p3);
void tm_memory_throughput_thread_report(void* p1, void* p2, void* p3);

/* Initialization prototype. */
void memory_throughput_initialize(void);

/* Main entry point. */
int main_memory_throughput_test(void)
{
   tm_initialize(memory_throughput_initialize);
   return 0;
}

/* Test initialization: setup PMU and create throughput test and reporting threads. */
void memory_throughput_initialize(void)
{
   /* Setup the PMU */
   tm_setup_pmu();

   for (int i = 0; i < ITERATION_COUNT; i++)
   {
      snprintf(pmu_memory_test[i], sizeof(pmu_memory_test[i]), "S%02d", i);
   }
   /* Create and resume the memory throughput test thread (ID: 0). */
   tm_thread_create(0, 10, tm_memory_throughput_thread_entry);
   tm_thread_resume(0);

   /* Create and resume the throughput reporting thread (ID: 1). */
   tm_thread_create(1, 10, tm_memory_throughput_thread_report);
   tm_thread_resume(1);
}

/* Memory Throughput Test Thread:
   Uses the PMU API to measure the duration of repeated memcpy operations.
   Each cycle, the PMU measurement is started with PMU_NAME, the memcpy loop
   is executed, and then the measurement is ended. */
void tm_memory_throughput_thread_entry(void* p1, void* p2, void* p3)
{
   unsigned long i;

   /* Initialize the source buffer with known data. */
   memset(msram_src, 0xAA, BUFFER_SIZE);

   while (1)
   {
      
      for (i = 0; i < THROUGHPUT_ITERATIONS; i++)
      {
         /* Start PMU profiling with the designated name. */
         tm_pmu_profile_start(pmu_memory_test[i]);
         memcpy(msram_dest, msram_src, BUFFER_SIZE);
         /* End PMU profiling. */
         tm_pmu_profile_end(pmu_memory_test[i]);
      }

      /* Sleep before running the next throughput test cycle. */
      tm_thread_sleep(TM_TEST_DURATION);
   }
}

/* Reporting thread for the memory throughput test.
   This thread periodically prints the PMU profiling results using the same PMU name.
   The PMU API is expected to output the measured cycle count/duration for the memcpy loop. */
void tm_memory_throughput_thread_report(void* p1, void* p2, void* p3)
{
   unsigned long relative_time = 0;

   while (1)
   {
      tm_thread_sleep(TM_TEST_DURATION);
      relative_time += TM_TEST_DURATION;
      printf("**** Memory Throughput Test **** Relative Time: %lu ms\r\n", relative_time);
      /* Print the PMU profiling results for the throughput test */
      for (int i = 0; i < ITERATION_COUNT; i++)
      {
         tm_pmu_profile_print(pmu_memory_test[i]);
      }
      printf("\r\n");
   }
}
