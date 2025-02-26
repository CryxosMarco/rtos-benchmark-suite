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
/*  11423 West Bernardo Court               http://www.expresslogic.com   */
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
/* Define the number of messages per queue */
#define MSGQ_NUM_MESSAGES 8
/* Calculate the size of each message in bytes */
#define MSGQ_MESSAGE_SIZE (MESSAGE_SIZE * sizeof(int32_t))
/* Calculate the total size of the message queue buffer */
#define MSGQ_BUFFER_SIZE (MSGQ_NUM_MESSAGES * MSGQ_MESSAGE_SIZE)

#if (CONFIG_MP_MAX_NUM_CPUS > 1)
#error "*** Tests are only designed for single processor systems! ***"
#endif

static struct k_thread test_thread[TM_TEST_NUM_THREADS];
static K_THREAD_STACK_ARRAY_DEFINE(test_stack, TM_TEST_NUM_THREADS, TM_TEST_STACK_SIZE);

static struct k_sem test_sem[TM_TEST_NUM_SEMAPHORES];

static struct k_msgq test_msgq[TM_TEST_NUM_MESSAGE_QUEUES];
/* Allocate the message queue buffer dynamically using our macros */
static char test_msgq_buffer[TM_TEST_NUM_MESSAGE_QUEUES][MSGQ_NUM_MESSAGES][MSGQ_MESSAGE_SIZE];

static struct k_mem_slab test_slab[TM_TEST_NUM_SLABS];
static char __aligned(4) test_slab_buffer[TM_TEST_NUM_SLABS][8 * 128];
/* Define an array of mutexes for Zephyr. */
struct k_mutex tm_mutex_array[TM_TEST_NUM_SEMAPHORES];
/* Global poll signal and poll event array */
static struct k_poll_event tm_sync_poll_event[1];
static struct k_poll_signal tm_sync_poll_signal;

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
 * file in the underlying RTOS. Valid priorities range from 1 through 31,
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
 * This function takes a thread ID and priority and attempts to create the
 * file in the underlying RTOS. Valid priorities range from 1 through 31,
 * where 1 is the highest priority and 31 is the lowest. It also passes the parameter
 * as pointer to the underlying thread. If successful, the function should return TM_SUCCESS.
 * Otherwise, TM_ERROR should be returned.
 */
int tm_thread_create_param(int thread_id, int priority, void (*entry_function)(void*, void*, void*), void* param)
{
   k_tid_t tid = k_thread_create(&test_thread[thread_id], test_stack[thread_id], TM_TEST_STACK_SIZE, entry_function,
                                 param, NULL, NULL, /* pass param as p1 */
                                 priority, 0, K_FOREVER);

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
 * This function suspends the specified thread.  If successful, the function should
 * return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
void tm_thread_exit(int thread_id)
{
   k_thread_suspend(&test_thread[thread_id]);
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

/* Version of above that only sleeps for defined period of ticks */
void tm_thread_sleep_ticks(int ticks)
{
   k_sleep(K_TICKS(ticks));
}

/*
 * This function creates the specified queue.
 * It initializes the Zephyr message queue to hold 8 messages,
 * each of size: MESSAGE_SIZE * sizeof(int32_t).
 * If successful, TM_SUCCESS is returned; otherwise, TM_ERROR.
 */
int tm_queue_create(int queue_id)
{
   k_msgq_init(&test_msgq[queue_id], &test_msgq_buffer[queue_id][0][0], MESSAGE_SIZE * sizeof(int32_t), 8);

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

/* This function sends a message to the specified queue from an ISR.  If successful,
   the function should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.  */
int tm_queue_send_from_isr(int queue_id, unsigned long* message_ptr)
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

/**
 * tm_task_priority_get - Returns the effective priority of the task given its id.
 * @thread_id: The index of the task in the OS-specific thread array.
 *
 * This function returns the current (effective) priority for the task corresponding
 * to the provided thread_id. The prototype is the same across all supported RTOSes.
 *
 * Return: The effective priority as an integer, or -1 if not supported.
 */
int tm_task_priority_get(int thread_id)
{
   /* Get the priority from the thread structure. */
   return (int) k_thread_priority_get(&test_thread[thread_id]);
}

void init_rtos_sync(void)
{
   /* Initialize the poll signal */
   k_poll_signal_init(&tm_sync_poll_signal);

   /* Manually initialize the poll event for the signal */
   tm_sync_poll_event[0].type = K_POLL_TYPE_SIGNAL;
   tm_sync_poll_event[0].mode = K_POLL_MODE_NOTIFY_ONLY;
   tm_sync_poll_event[0].obj = &tm_sync_poll_signal;
   tm_sync_poll_event[0].state = K_POLL_STATE_NOT_READY;
}

/*
 * rtos_sync_wait()
 *
 * Waits indefinitely until the poll signal is raised.
 * After k_poll() returns (with state K_POLL_STATE_SIGNALED), it resets the poll event's state.
 */
int rtos_sync_wait(void)
{
   int ret = k_poll(tm_sync_poll_event, 1, K_FOREVER);

   if (ret == 0 && tm_sync_poll_event[0].state == K_POLL_STATE_SIGNALED)
   {
      /* Reset the signal state so the next poll will block */
      tm_sync_poll_event[0].signal->signaled = 0;
      tm_sync_poll_event[0].state = K_POLL_STATE_NOT_READY;
      return TM_SUCCESS;
   }

   return TM_ERROR;
}

/*
 * rtos_sync_signal()
 *
 * Signals the waiting thread by raising the poll signal.
 */
int rtos_sync_signal(void)
{
   k_poll_signal_raise(&tm_sync_poll_signal, 1);
   return TM_SUCCESS;
}

/* This function enters a critical section. */
void tm_enter_critical_section()
{
   k_sched_lock();
}

/* This function exits a critical section. */
void tm_exit_critical_section()
{
   k_sched_unlock();
}

/*-----------------------------------------------------------
 * Suspend the Zephyr scheduler
 *-----------------------------------------------------------*/
void tm_suspend_scheduler()
{
   k_sched_lock();
}

/*-----------------------------------------------------------
 * Resume the Zephyr scheduler
 *-----------------------------------------------------------*/
void tm_resume_scheduler()
{
   k_sched_unlock();
}

/* ----------------------------------------------------------*/
/*                        Driver Code                        */
/* ----------------------------------------------------------*/
/*
 * Example PMU Driver & Profiling Implementation for Zephyr (Cortex-R5)
 *
 * This code sets up the PMU at system initialization and provides
 * an API to profile multiple code segments by name. Start/End calls
 * store data in a global profile log. You can then print results later.
 * TODO: Place this driver Code inside Zephyr and make just the callings here.
 */

/* --------------------------------------------------------------------------
 * Basic event configuration structure
 * -------------------------------------------------------------------------- */
typedef struct
{
   const char* name; /* e.g. "DCache Miss" */
   uint32_t type;    /* e.g. 0x03 for DCache Miss event ID */
} PMU_EventCfg;

typedef struct
{
   bool bCycleCounter;
   uint32_t numEventCounters;
   PMU_EventCfg* eventCounters;
} PMU_Config;

/* --------------------------------------------------------------------------
 * Our event configuration (3 events) - adjust as needed
 * -------------------------------------------------------------------------- */
#define PMU_MAX_EVENT_COUNTERS (3U)
static PMU_EventCfg gPmuEventCfg[PMU_MAX_EVENT_COUNTERS] = {
   {"ICache Miss", 0x01},
   {"DCache Access", 0x04},
   {"DCache Miss", 0x03},
};

static PMU_Config gPmuConfig = {
   .bCycleCounter = true, .numEventCounters = PMU_MAX_EVENT_COUNTERS, .eventCounters = gPmuEventCfg};

/* prototype here for later use*/
void pmu_init_profile(void);
/* --------------------------------------------------------------------------
 * PMU Initialization (called at system boot)
 * -------------------------------------------------------------------------- */
int tm_setup_pmu(void)
{
   printk("Initializing PMU...\n");

   /* Disable all counters (PMCR.E=0) */
   uint32_t pmcr = pmu_read_pmcr();
   pmcr &= ~0x1;
   pmu_write_pmcr(pmcr);

   /* Clear (disable) all counters at once */
   pmu_write_cntenclr(0xFFFFFFFF);

   /*
    * Reset cycle and event counters (bits [2]=C, [1]=P), no divider (bit[3]=0)
    *    If you need a 64x divider, do: pmcr |= (1 << 3);
    */
   pmcr = (1 << 2) | (1 << 1); /* C=1 (reset cycle), P=1 (reset events), D=0 => no divider */
   pmu_write_pmcr(pmcr);

   /* Reset cycle counter to 0 */
   pmu_write_pmccntr(0);

   /* Configure event counters */
   for (uint32_t i = 0; i < gPmuConfig.numEventCounters; i++)
   {
      pmu_select_event_counter(i);
      pmu_write_evtyper(gPmuConfig.eventCounters[i].type);
      pmu_write_evcounter(0);
   }

   /* Enable cycle counter & event counters */
   /*    bit31 => cycle counter, plus bits [0..(numEventCounters-1)] => event counters */
   pmu_write_cntenset((1 << 31) | ((1 << gPmuConfig.numEventCounters) - 1));

   /* Enable counters in PMCR (bit[0] = E=1) */
   pmcr = pmu_read_pmcr();
   pmcr |= 0x1;
   pmu_write_pmcr(pmcr);

   /* Allow user mode access to PMU */
   pmu_enable_user_access();
   pmu_init_profile();
   printk("PMU Initialized.\n");
   return 0; /* success */
}

/* Register this init function to run at PRE_KERNEL_1 */
SYS_INIT(tm_setup_pmu, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/* --------------------------------------------------------------------------
 * Data structures for multi-point profiling
 * -------------------------------------------------------------------------- */

/* Max number of log entries we can store */
#define PMU_MAX_LOG_ENTRIES (64U)

typedef struct
{
   const char* name;
   uint32_t type;
   uint32_t value; /* final delta of events */
} TM_PMUEvent;

/* Each profile point: name, cycle counters, event counters */
typedef struct
{
   const char* name;

   /* We store separate start/end for computing a delta */
   uint32_t cycleCountStart;
   uint32_t cycleCountEnd;
   uint32_t cycleCountValue; /* (End - Start) */

   TM_PMUEvent events[PMU_MAX_EVENT_COUNTERS];
   uint32_t eventStart[PMU_MAX_EVENT_COUNTERS];
   uint32_t eventEnd[PMU_MAX_EVENT_COUNTERS];

} TM_PMUProfilePoint;

typedef struct
{
   uint32_t logIndex;
   TM_PMUProfilePoint point[PMU_MAX_LOG_ENTRIES];

   uint32_t numEvents;
   uint8_t bCycleCounter;
} TM_PMUProfileObject;

/* Global object storing the logged profiling data */
static TM_PMUProfileObject gProfileObject = {0};

/* --------------------------------------------------------------------------
 * Init for the Chache Hits/Misses profile structure
 * -------------------------------------------------------------------------- */
void pmu_init_profile(void)
{
   memset(&gProfileObject, 0, sizeof(gProfileObject));
   gProfileObject.logIndex = 0;
   gProfileObject.numEvents = PMU_MAX_EVENT_COUNTERS;
   gProfileObject.bCycleCounter = 1; /* We use cycle counter */
}

/* --------------------------------------------------------------------------
 * tm_pmu_profile_start(name)
 * - Reset PMU counters
 * - Read "start" values
 * - Store them in the next free slot
 * -------------------------------------------------------------------------- */
void tm_pmu_profile_start(const char* name)
{
   uint32_t idx = gProfileObject.logIndex;
   if (idx >= PMU_MAX_LOG_ENTRIES)
   {
      /* no more space */
      return;
   }

   TM_PMUProfilePoint* p = &gProfileObject.point[idx];
   p->name = name;

   for (uint32_t i = 0; i < gProfileObject.numEvents; i++)
   {
      pmu_select_event_counter(i);
      /* Reset the counters to 0 */
      pmu_write_evcounter(0);
      p->eventStart[i] = pmu_read_evcounter();

      /* Also store name & type */
      p->events[i].name = gPmuEventCfg[i].name;
      p->events[i].type = gPmuEventCfg[i].type;
   }
   /* Immediately read them as "start" values */
   p->cycleCountStart = pmu_read_pmccntr();
}

/* --------------------------------------------------------------------------
 * tm_pmu_profile_end(name)
 * - Read PMU Registers for "end" values
 * - Compute delta
 * - Increase log index
 * -------------------------------------------------------------------------- */
void tm_pmu_profile_end(const char* name)
{
   uint32_t idx = gProfileObject.logIndex;
   if (idx >= PMU_MAX_LOG_ENTRIES)
   {
      return;
   }

   TM_PMUProfilePoint* p = &gProfileObject.point[idx];

   /* Optional: check name matches the start */
   // if (p->name == NULL || strcmp(p->name, name) != 0)
   // {
   //    /* mismatch => error or skip */
   //    return;
   // }

   /* Read end counters */
   p->cycleCountEnd = pmu_read_pmccntr();
   for (uint32_t i = 0; i < gProfileObject.numEvents; i++)
   {
      pmu_select_event_counter(i);
      p->eventEnd[i] = pmu_read_evcounter();
   }

   /* Compute deltas */
   p->cycleCountValue = p->cycleCountEnd - p->cycleCountStart;
   for (uint32_t i = 0; i < gProfileObject.numEvents; i++)
   {
      uint32_t diff = p->eventEnd[i] - p->eventStart[i];
      p->events[i].value = diff;
   }

   /* Move to next log slot for future profileStart() */
   gProfileObject.logIndex++;
}

/* --------------------------------------------------------------------------
 * tm_pmu_profile_print_entry(name)
 * - Search for an entry with the given name
 * - Print results
 * -------------------------------------------------------------------------- */
void tm_pmu_profile_print(const char* name)
{
   for (uint32_t i = 0; i < gProfileObject.logIndex; i++)
   {
      TM_PMUProfilePoint* p = &gProfileObject.point[i];
      if (p->name != NULL && strcmp(p->name, name) == 0)
      {
         printk("Profile Entry: %s\n", p->name);
         printk("Cycle Count: %u\n", p->cycleCountValue);

         for (uint32_t j = 0; j < gProfileObject.numEvents; j++)
         {
            printk("%s Count: %u\n", p->events[j].name, p->events[j].value);
         }
         printk("\n");
         return;
      }
   }
   printk("No profile entry found for name: %s\n", name);
}

/* --------------------------------------------------------------------------
 * tm_pmu_profile_print_all()
 * - Dump all stored profiling results
 * -------------------------------------------------------------------------- */
void tm_pmu_profile_print_all(void)
{
   for (uint32_t i = 0; i < gProfileObject.logIndex; i++)
   {
      TM_PMUProfilePoint* p = &gProfileObject.point[i];
      printk("Profile Entry #%u: %s\n", i, p->name);
      printk("Cycle Count: %u\n", p->cycleCountValue);

      for (uint32_t j = 0; j < gProfileObject.numEvents; j++)
      {
         printk("Event %s Count: %u\n", p->events[j].name, p->events[j].value);
      }
      printk("\n");
   }
}

/* --------------------------------------------------------------------------
 * tm_pmu_profile_reset()
 * - Clear all stored profile data
 * -------------------------------------------------------------------------- */
void tm_pmu_profile_reset(void)
{
   memset(&gProfileObject, 0, sizeof(gProfileObject));
   /* Reinitialize counts if needed */
   gProfileObject.numEvents = PMU_MAX_EVENT_COUNTERS;
   gProfileObject.bCycleCounter = 1;
   gProfileObject.logIndex = 0;
}

#endif /* USING_ZEPHYR */
