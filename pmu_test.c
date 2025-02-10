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

/********************************************************************************
 * MAIN ENTRY POINT
 ********************************************************************************/
int main_pmu(void)
{
   printf("[Main] Starting PMU Test Test.\n");
   tm_setup_pmu();

   tm_pmu_profile_start("PMU_Test");
   // tm_thread_sleep(1);
   tm_pmu_profile_end("PMU_Test");
   tm_pmu_profile_print("PMU_Test");

   printf("[Main] Finished Test.\n");

   return 0;
}

/*[EOF]************************************************************************/
