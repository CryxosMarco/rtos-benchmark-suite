// SPDX-License-Identifier: Apache-2.0

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
