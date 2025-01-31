/**************************************************************************/
/*                                                                        */
/*            Copyright (c) 1996-2016 by Express Logic Inc.               */
/*                                                                        */
/*  This Original Work may be modified, distributed, or otherwise used in */
/*  any manner with no obligations other than the following:              */
/*                                                                        */
/*    1. This legend must be retained in its entirety in any source code  */
/*       copies of this Work.                                             */
/*                                                                        */
/*    2. This software may not be used in the development of an operating */
/*       system product.                                                  */
/*                                                                        */
/*  This Original Work is hereby provided on an "AS IS" BASIS and WITHOUT */
/*  WARRANTY, either express or implied, including, without limitation,   */
/*  the warranties of NON-INFRINGEMENT, MERCHANTABILITY or FITNESS FOR A  */
/*  PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY OF this         */
/*  ORIGINAL WORK IS WITH the user.                                       */
/*                                                                        */
/*  Express Logic, Inc. reserves the right to modify this software        */
/*  without notice.                                                       */
/*                                                                        */
/*  Express Logic, Inc.                     info@expresslogic.com         */
/*  11423 West Bernardo Court               http:/*www.expresslogic.com   */
/*  San Diego, CA  92127                                                  */
/*                                                                        */
/**************************************************************************/

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** Thread-Metric Component                                               */
/**                                                                       */
/**   Porting Layer (Must be completed with RTOS specifics)               */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/* Include necessary files.  */
#include "rtos_config.h"
#ifdef USING_ZEPHYR

#include "tm_api.h"

#define TM_TEST_NUM_THREADS 10
#define TM_TEST_STACK_SIZE 1024
#define TM_TEST_NUM_SEMAPHORES 4
#define TM_TEST_NUM_MESSAGE_QUEUES 4
#define TM_TEST_NUM_SLABS 4

#if (CONFIG_MP_MAX_NUM_CPUS > 1)
#error "*** Tests are only designed for single processor systems! ***"
#endif

static struct k_thread test_thread[TM_TEST_NUM_THREADS];
static K_THREAD_STACK_ARRAY_DEFINE(test_stack, TM_TEST_NUM_THREADS, TM_TEST_STACK_SIZE);

static struct k_sem test_sem[TM_TEST_NUM_SEMAPHORES];

static struct k_msgq test_msgq[TM_TEST_NUM_MESSAGE_QUEUES];
static char test_msgq_buffer[TM_TEST_NUM_MESSAGE_QUEUES][8][16];

static struct k_mem_slab test_slab[TM_TEST_NUM_SLABS];
static char __aligned(4) test_slab_buffer[TM_TEST_NUM_SLABS][8 * 128];
/* Define an array of mutexes for Zephyr. */
struct k_mutex tm_mutex_array[TM_TEST_NUM_SEMAPHORES];

/*
 * This function called from main performs basic RTOS initialization,
 * calls the test initialization function, and then starts the RTOS function.
 */
void tm_initialize(void (*test_initialization_function)(void))
{
   test_initialization_function();
}

/*
 * This function takes a thread ID and priority and attempts to create the
 * file in the underlying RTOS.  Valid priorities range from 1 through 31,
 * where 1 is the highest priority and 31 is the lowest. If successful,
 * the function should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_thread_create(int thread_id, int priority, void (*entry_function)(void*, void*, void*))
{
   k_tid_t tid;

   tid = k_thread_create(&test_thread[thread_id], test_stack[thread_id], TM_TEST_STACK_SIZE, entry_function, NULL, NULL,
                         NULL, priority, 0, K_FOREVER);

   /* Thread started in sleeping state. Switch to suspended state */

   k_thread_suspend(&test_thread[thread_id]);
   k_wakeup(&test_thread[thread_id]);

   return (tid == &test_thread[thread_id]) ? TM_SUCCESS : TM_ERROR;
}

/*
 * This function resumes the specified thread.  If successful, the function should
 * return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_thread_resume(int thread_id)
{
   k_thread_resume(&test_thread[thread_id]);

   return TM_SUCCESS;
}

/*
 * This function suspends the specified thread.  If successful, the function should
 * return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_thread_suspend(int thread_id)
{
   k_thread_suspend(&test_thread[thread_id]);

   return TM_SUCCESS;
}

/*
 * This function relinquishes to other ready threads at the same
 * priority.
 */
void tm_thread_relinquish(void)
{
   k_yield();
}

/*
 * This function suspends the specified thread for the specified number
 * of seconds.
 */
void tm_thread_sleep(int seconds)
{
   k_sleep(K_SECONDS(seconds));
}

/*
 * This function creates the specified queue.  If successful, the function should
 * return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_queue_create(int queue_id)
{
   k_msgq_init(&test_msgq[queue_id], &test_msgq_buffer[queue_id][0][0], 16, 8);

   return TM_SUCCESS;
}

/*
 * This function sends a 16-byte message to the specified queue.  If successful,
 * the function should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_queue_send(int queue_id, unsigned long* message_ptr)
{
   return k_msgq_put(&test_msgq[queue_id], message_ptr, K_FOREVER);
}

/*
 * This function receives a 16-byte message from the specified queue.  If successful,
 * the function should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_queue_receive(int queue_id, unsigned long* message_ptr)
{
   return k_msgq_get(&test_msgq[queue_id], message_ptr, K_FOREVER);
}

/*
 * This function creates the specified semaphore.  If successful, the function should
 * return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_semaphore_create(int semaphore_id)
{
   /* Create an available semaphore with max count of 1 */
   return k_sem_init(&test_sem[semaphore_id], 1, 1);
}

/*
 * This function gets the specified semaphore.  If successful, the function should
 * return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_semaphore_get(int semaphore_id)
{
   return k_sem_take(&test_sem[semaphore_id], K_NO_WAIT);
}

/*
 * This funtion waits for the specified semaphore.
 * if successful return TM_SUCCESS. Otherwithe, TM_ERROR should be returned.
 */
int tm_semaphore_wait(int semaphore_id)
{
   int rc = k_sem_take(&test_sem[semaphore_id], K_FOREVER);
   if (rc == 0)
   {
      return TM_SUCCESS;
   }
   else
   {
      return TM_ERROR;
   }
}

/*
 * This function puts the specified semaphore.  If successful, the function should
 * return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_semaphore_put(int semaphore_id)
{
   k_sem_give(&test_sem[semaphore_id]);
   return TM_SUCCESS;
}

/* This function is defined by the benchmark. */
extern void tm_interrupt_handler(const void*);

void tm_cause_interrupt(void)
{
   irq_offload(tm_interrupt_handler, NULL);
}

/* Mutex create function. */
int tm_mutex_create(int mutex_id)
{
   /* Validate the mutex ID. */
   if (mutex_id < 0 || mutex_id >= TM_TEST_NUM_SEMAPHORES)
   {
      return TM_ERROR;
   }

   /* Initialize the mutex. */
   k_mutex_init(&tm_mutex_array[mutex_id]);

   /* Return success. */
   return TM_SUCCESS;
}

/* Mutex get function. */
int tm_mutex_get(int mutex_id)
{
   int status;

   /* Validate the mutex ID. */
   if (mutex_id < 0 || mutex_id >= TM_TEST_NUM_SEMAPHORES)
   {
      return TM_ERROR;
   }

   /* Acquire the mutex (wait forever). */
   status = k_mutex_lock(&tm_mutex_array[mutex_id], K_FOREVER);

   /* Return appropriate status. */
   return (status == 0) ? TM_SUCCESS : TM_ERROR;
}

/* Mutex put function. */
int tm_mutex_put(int mutex_id)
{
   /* Validate the mutex ID. */
   if (mutex_id < 0 || mutex_id >= TM_TEST_NUM_SEMAPHORES)
   {
      return TM_ERROR;
   }

   /* Release the mutex. */
   k_mutex_unlock(&tm_mutex_array[mutex_id]);

   /* Return success. */
   return TM_SUCCESS;
}

/*
 * This function creates the specified memory pool that can support one or more
 * allocations of 128 bytes.  If successful, the function should
 * return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_memory_pool_create(int pool_id)
{
   int status;

   status = k_mem_slab_init(&test_slab[pool_id], &test_slab_buffer[pool_id][0], 128, 8);

   return (status == 0) ? TM_SUCCESS : TM_ERROR;
}

/*
 * This function allocates a 128 byte block from the specified memory pool.
 * If successful, the function should return TM_SUCCESS. Otherwise, TM_ERROR
 * should be returned.
 */
int tm_memory_pool_allocate(int pool_id, unsigned char** memory_ptr)
{
   int status;

   status = k_mem_slab_alloc(&test_slab[pool_id], (void**) memory_ptr, K_NO_WAIT);

   return (status == 0) ? TM_SUCCESS : TM_ERROR;
}

/*
 * This function releases a previously allocated 128 byte block from the specified
 * memory pool. If successful, the function should return TM_SUCCESS. Otherwise, TM_ERROR
 * should be returned.
 */
int tm_memory_pool_deallocate(int pool_id, unsigned char* memory_ptr)
{
   k_mem_slab_free(&test_slab[pool_id], (void*) memory_ptr);

   return TM_SUCCESS;
}

/* This function returns the number of ticks to estimate a time*/
unsigned long tm_time_get(void)
{
   return 5; /* dummy value */
}


LOG_MODULE_REGISTER(pmu, CONFIG_PMU_LOG_LEVEL);

#define PMU_MAX_EVENT_COUNTERS 3

typedef struct {
    const char* name;
    uint32_t type;
} PMU_EventCfg;

typedef struct {
    bool bCycleCounter;
    uint32_t numEventCounters;
    PMU_EventCfg* eventCounters;
} PMU_Config;

/*-----------------------------------------------------------
 * Event Configuration
 *-----------------------------------------------------------*/
static PMU_EventCfg gPmuEventCfg[PMU_MAX_EVENT_COUNTERS] = {
   { "ICache Miss", 0x01 },  /* CSL_ARM_R5_PMU_EVENT_TYPE_ICACHE_MISS */
   { "DCache Access", 0x04 },  /* CSL_ARM_R5_PMU_EVENT_TYPE_DCACHE_ACCESS */
   { "DCache Miss", 0x03 }  /* CSL_ARM_R5_PMU_EVENT_TYPE_DCACHE_MISS */
};

static PMU_Config gPmuConfig = {
   .bCycleCounter = true,
   .numEventCounters = PMU_MAX_EVENT_COUNTERS,
   .eventCounters = gPmuEventCfg
};

/*-----------------------------------------------------------
 * PMU Initialization
 *-----------------------------------------------------------*/
int tm_setup_pmu(void)
{
    printk("Initializing PMU...\n");

    /* 1) Disable PMU */
    uint32_t pmcr = pmu_read_pmcr();
    pmcr &= ~0x1;
    pmu_write_pmcr(pmcr);

    /* 2) Clear all counters */
    pmu_write_cntenclr(0xFFFFFFFF);

    /* 3) Reset cycle and event counters, configure no divider */
    pmcr = (1 << 2) | (1 << 1);  /* Reset Cycle & Event Counter */
   // pmcr |= (1 << 3);            /* D = 1 => 64er Divider enabled */
    pmu_write_pmcr(pmcr);

    /* 4) Reset cycle counter */
    pmu_write_pmccntr(0);

    /* 5) Configure event counters */
    for (uint32_t i = 0; i < gPmuConfig.numEventCounters; i++) {
        pmu_select_event_counter(i);
        pmu_write_evtyper(gPmuConfig.eventCounters[i].type);
        pmu_write_evcounter(0);
    }

    /* 6) Enable cycle counter & event counters */
    pmu_write_cntenset((1 << 31) | ((1 << gPmuConfig.numEventCounters) - 1));

    /* 
      E=1 => „Enable all counters“
      P=1 => „Reset all event counters“ 
      C=1 => „Reset cycle counter“ 
      D=0 => „Kein 64er Divider“ */

    /* 7) Enable PMU */
    pmcr = pmu_read_pmcr();
    pmcr |= 0x1;
    pmu_write_pmcr(pmcr);

    printk("PMU Initialized.\n");
    return 1;
}

SYS_INIT(tm_setup_pmu, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/*-----------------------------------------------------------
 * Start Profiling
 *-----------------------------------------------------------*/
void tm_pmu_profile_start(const char* name)
{
    printk("PMU Profiling Start: %s \n", name);
    pmu_write_pmccntr(0);  /* Reset cycle counter
    for (uint32_t i = 0; i < gPmuConfig.numEventCounters; i++) {
        pmu_select_event_counter(i);
        pmu_write_evcounter(0);
    }
}

/*-----------------------------------------------------------
 * End Profiling
 *-----------------------------------------------------------*/
void tm_pmu_profile_end(const char* name)
{
    printk("PMU Profiling End: %s \n", name);
}

/*-----------------------------------------------------------
 * Print Profiling Results
 *-----------------------------------------------------------*/
void tm_pmu_profile_print(const char* name)
{
    uint32_t cycles = pmu_read_pmccntr();
    printk("Profiling Results for: %s \n", name);
    printk("Cycle Count: %u \n", cycles);
    
    for (uint32_t i = 0; i < gPmuConfig.numEventCounters; i++) {
        pmu_select_event_counter(i);
        uint32_t count = pmu_read_evcounter();
        printk("Event [%s]: %u \n", gPmuConfig.eventCounters[i].name, count);
    }
}

#endif /* USING_ZEPHYR */
