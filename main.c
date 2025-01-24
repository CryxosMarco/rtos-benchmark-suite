/*[CR]**************************************************************************
Copyright (c) 2024 IBV - Echtzeit- und Embedded GmbH & Co. KG
SPDX-License-Identifier: Apache-2.0
.
*/
/*[FH]**************************************************************************
PROJECT: MASTER THESIS
MODULE: Entry Point
CONTENTS: Main entry point for the test application
*
/*[CL]**************************************************************************
21-01-2025 MMI Initial creation of the file

---
MMI: Marco Milenkovic, IBV, Milenkovic@ibv-augsburg.de
*/
/*[MP]**************************************************************************
 * Entry point for the test application
 ******************************************************************************/

#include "rtos_config.h"
#include "tm_api.h"

int main(void)
{
#ifdef USING_ZEPHYR
   extern int rtos_main_zephyr(void);
   return rtos_main_zephyr();
#elif defined(USING_FREERTOS)
   extern int rtos_main_freertos(void);
   return rtos_main_freertos();
#elif defined(USING_THREADX)
   extern int rtos_main_threadx(void);
   return rtos_main_threadx();
#else
#error "No RTOS defined! Define one in rtos_config.h."
#endif
}
