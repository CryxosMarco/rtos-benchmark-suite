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

#define ITERATION_COUNT 30
/* Precomputed PMU name arrays for send and receive iterations */
char pmu_calib_names[ITERATION_COUNT][16];

/********************************************************************************
 * MAIN ENTRY POINT
 ********************************************************************************/
int main_pmu(void)
{
   /* Precompute PMU names for each iteration so the ISR can avoid runtime formatting */
   for (int i = 0; i < ITERATION_COUNT; i++)
   {
      snprintf(pmu_calib_names[i], sizeof(pmu_calib_names[i]), "Run %02d", i);
   }

   printf("[Main] Starting PMU calibration Test.\r\n");
   tm_setup_pmu();

   for (int i = 0; i < ITERATION_COUNT; i++)
   {
      /* Measure send latency using a precomputed PMU name */
      tm_pmu_profile_start(pmu_calib_names[i]);
      // tm_thread_sleep(1); /* comment out for measuring overhead of pmu */
      tm_pmu_profile_end(pmu_calib_names[i]);
   }
   for (int i = 0; i < ITERATION_COUNT; i++)
   {
      printf("[Main] PMU Test: ");
      tm_pmu_profile_print(pmu_calib_names[i]);
   }
   printf("[Main] Finished Calibration Test.\r\n");

   return 0;
}

/*[EOF]************************************************************************/
