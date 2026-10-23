/*
 * Lesson 22: FreeRTOS.
 *
 *   sensor  (priority 2)  every 200 ms puts a reading into a queue
 *   logger  (priority 1)  blocks on the queue and prints each reading
 *   button  (priority 3)  blocks on a semaphore that the EXTI0 interrupt gives
 *   monitor (priority 1)  after 1.2 s prints the task list and stack usage
 *   timer   (software)    toggles the blue LED every 250 ms
 *
 * printf is shared by several tasks, so it is protected by a mutex.
 */
#include <stdio.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#include "board.h"
#include "uart.h"

typedef struct {
    uint32_t seq;
    TickType_t when;
    int32_t value;
} reading_t;

static QueueHandle_t g_readings;
static SemaphoreHandle_t g_button_sem;
static SemaphoreHandle_t g_print_mutex;

#define LOG(...)                                         \
    do {                                                 \
        xSemaphoreTake(g_print_mutex, portMAX_DELAY);    \
        printf(__VA_ARGS__);                             \
        xSemaphoreGive(g_print_mutex);                   \
    } while (0)

/* ---- tasks --------------------------------------------------------------- */
static void sensor_task(void *arg)
{
    (void)arg;
    TickType_t last_wake = xTaskGetTickCount();
    for (uint32_t seq = 0;; seq++) {
        reading_t r = {seq, xTaskGetTickCount(), 200 + (int32_t)(seq * 7 % 13)};
        if (xQueueSend(g_readings, &r, 0) != pdPASS) {
            LOG("sensor: queue full, reading %lu dropped\n", (unsigned long)seq);
        }
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(200));   /* fixed period, no drift */
    }
}

static void logger_task(void *arg)
{
    (void)arg;
    reading_t r;
    for (;;) {
        if (xQueueReceive(g_readings, &r, portMAX_DELAY) == pdPASS) {   /* sleeps until data */
            LOG("[%4lu ms] logger: reading #%lu = %ld (queued at %lu ms)\n",
                (unsigned long)xTaskGetTickCount(), (unsigned long)r.seq, (long)r.value,
                (unsigned long)r.when);
        }
    }
}

static void button_task(void *arg)
{
    (void)arg;
    for (uint32_t presses = 1;; presses++) {
        xSemaphoreTake(g_button_sem, portMAX_DELAY);
        led_toggle(LED_ORANGE);
        LOG("[%4lu ms] button: press %lu handled in task context\n",
            (unsigned long)xTaskGetTickCount(), (unsigned long)presses);
    }
}

static void monitor_task(void *arg)
{
    (void)arg;
    static const char *const state_name[] = {"running", "ready", "blocked", "suspended",
                                             "deleted"};
    static TaskStatus_t tasks[8];
    vTaskDelay(pdMS_TO_TICKS(1200));

    xSemaphoreTake(g_print_mutex, portMAX_DELAY);
    UBaseType_t n = uxTaskGetSystemState(tasks, 8, NULL);
    printf("\n%-8s %-8s %4s %15s\n", "task", "state", "prio", "stack free (B)");
    for (UBaseType_t i = 0; i < n; i++) {
        printf("%-8s %-8s %4lu %15lu\n", tasks[i].pcTaskName,
               state_name[tasks[i].eCurrentState], (unsigned long)tasks[i].uxCurrentPriority,
               (unsigned long)tasks[i].usStackHighWaterMark * sizeof(StackType_t));
    }
    printf("heap: %u bytes free, minimum ever %u\n", (unsigned)xPortGetFreeHeapSize(),
           (unsigned)xPortGetMinimumEverFreeHeapSize());
    xSemaphoreGive(g_print_mutex);

    vTaskDelete(NULL);                                     /* a task may delete itself */
}

static void blink_timer(TimerHandle_t t)
{
    (void)t;
    led_toggle(LED_BLUE);                                  /* runs in the timer service task */
}

/* ---- interrupt ----------------------------------------------------------- */
void EXTI0_IRQHandler(void)
{
    BaseType_t woken = pdFALSE;
    EXTI->PR = EXTI_PR_PR0;
    xSemaphoreGiveFromISR(g_button_sem, &woken);
    portYIELD_FROM_ISR(woken);              /* switch right away if button_task outranks us */
}

static void button_irq_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    gpio_set_mode(GPIOA, 0, GPIO_MODE_INPUT);
    gpio_set_pull(GPIOA, 0, GPIO_PULL_DOWN);
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;            /* PA0 */
    EXTI->IMR |= EXTI_IMR_MR0;
    EXTI->RTSR |= EXTI_RTSR_TR0;
    /* Must be numerically >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5)
     * because the handler calls a FromISR API. */
    NVIC_SetPriority(EXTI0_IRQn, 6);
    NVIC_EnableIRQ(EXTI0_IRQn);
}

/* ---- hooks the kernel calls ---------------------------------------------- */
void vApplicationIdleHook(void)
{
    __WFI();                                /* nothing to run: sleep until the next interrupt */
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    printf("stack overflow in %s\n", name);
    for (;;) {
    }
}

void vApplicationMallocFailedHook(void)
{
    printf("out of heap\n");
    for (;;) {
    }
}

void vAssertCalled(const char *file, int line)
{
    taskDISABLE_INTERRUPTS();
    printf("assert failed: %s:%d\n", file, line);
    for (;;) {
    }
}

int main(void)
{
    board_init_leds();
    uart_init(HSI_HZ, 115200);
    button_irq_init();

    printf("\nlesson 22: FreeRTOS, 4 tasks + 1 software timer\n");

    g_readings = xQueueCreate(4, sizeof(reading_t));
    g_button_sem = xSemaphoreCreateBinary();
    g_print_mutex = xSemaphoreCreateMutex();
    configASSERT(g_readings && g_button_sem && g_print_mutex);

    xTaskCreate(sensor_task, "sensor", 256, NULL, 2, NULL);
    xTaskCreate(logger_task, "logger", 256, NULL, 1, NULL);
    xTaskCreate(button_task, "button", 256, NULL, 3, NULL);
    xTaskCreate(monitor_task, "monitor", 384, NULL, 1, NULL);
    xTimerStart(xTimerCreate("blink", pdMS_TO_TICKS(250), pdTRUE, NULL, blink_timer), 0);

    vTaskStartScheduler();                  /* never returns unless the heap is too small */
    printf("scheduler failed to start\n");
    for (;;) {
    }
}
