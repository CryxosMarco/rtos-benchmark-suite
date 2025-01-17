

#include <soc.h>
#include <stdlib.h>
#include <kernel/dpl/DebugP.h>
#include <kernel/dpl/TimerP.h>
#include "tm_api.h"

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

extern void *test_interrupt_handler = NULL;

/* unused */
// extern volatile uint32_t gTimerCount = 0;
// volatile unsigned long overflow_counter = 0;

/* Define the interrupt handler */
void tm_interrupt_handler(void *args)
{
    if (test_interrupt_handler != NULL)
    {
        // Call the assigned handler function
        ((void (*)(void))test_interrupt_handler)();
    }
}

/* Function to trigger the software interrupt */
void tm_interrupt_raise(void)
{
    unsigned int irq = SOFTWARE_INTERRUPT_ID;

    // Software-Interrupt auslösen
    z_vim_arm_enter_irq(irq);
}

void setup_interrupt(void)
{
    unsigned int irq = SOFTWARE_INTERRUPT_ID;
    unsigned int priority = 1;

    // Interrupt konfigurieren: Edge-triggered
    z_vim_irq_priority_set(irq, priority, IRQ_TYPE_EDGE);

    // Interrupt-Handler registrieren
    IRQ_CONNECT(SOFTWARE_INTERRUPT_ID, 1, tm_interrupt_handler, NULL, 0);
    irq_enable(SOFTWARE_INTERRUPT_ID);

    // Interrupt aktivieren
    z_vim_irq_enable(irq);
}

void main_task(void *pvParameters)
{
    /* Start Thread-Metric tests */
    printk("Starting Thread-Metric tests...");

    /* Initialize custom interrupts*/
    test_interrupt_handler = tm_interrupt_processing_handler;
    setup_interrupt();

    /* Call the main Thread-Metric function */
    tm_main_three();

    /* Delete thread after completion */
    k_thread_abort(k_current_get());
}

// Thread definition
K_THREAD_DEFINE(main_thread, 512 /* STACKSIZE */, main_task, NULL, NULL, NULL, MAIN_TASK_PRI, K_USER, -1);

int main(void)
{
    printk("Initializing Zephyr system...");

    /* Get the CPU frequency */
    // uint32_t cpu_freq_hz = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
    // printk("CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC: %u Hz (%u MHz)\n", cpu_freq_hz, cpu_freq_hz / 1000000);

    /* Get the system tick rate */
    // uint32_t tick_rate_hz = CONFIG_SYS_CLOCK_TICKS_PER_SEC;
    // printk("CONFIG_SYS_CLOCK_TICKS_PER_SEC: %u ticks/second\n", tick_rate_hz);

    /* Create main task */
    k_thread_start(main_thread);

    printk("Main task created and running...\n");

    return 0;
}

// void App_timerCallback(void *args){

//     if (TimerP_isOverflowed(gTimerBaseAddr[CONFIG_TIMER0])) {
//             overflow_counter++;
//             TimerP_clearOverflowInt(gTimerBaseAddr[CONFIG_TIMER0]);
//         }
// }

// void App_timerInit(uint32_t freq)
// {
//     TimerP_Params timerParams;

//     TimerP_Params_init(&timerParams);
//     timerParams.inputPreScaler = 1U;
//     timerParams.inputClkHz     = 25000000U;
//     timerParams.periodInUsec   = (1000000/freq);
//     timerParams.oneshotMode    = 0;
//     timerParams.enableOverflowInt = 1;
//     TimerP_setup(gTimerBaseAddr[CONFIG_TIMER0], &timerParams);
//     TimerP_start(gTimerBaseAddr[CONFIG_TIMER0]);
// }