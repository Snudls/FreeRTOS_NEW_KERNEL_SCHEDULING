/*
 * Timeline Scheduler - Automated Test Suite
 *
 * Tests the scheduler's correctness and robustness with:
 * - Stress tests (overlapping HRT tasks)
 * - Edge-case tests (minimal time gaps)
 * - Preemption and timing consistency checks
 *
 * Copyright (C) 2024 - Released under FreeRTOS MIT License
 */

#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include <stdint.h>
#include <stdio.h>
#include <timeline.h>
// /*-----------------------------------------------------------
//  * Test Selection (1-5)
//  *----------------------------------------------------------*/
#ifndef TEST_NUMBER
#define TEST_NUMBER 3
#endif

/*-----------------------------------------------------------
 * Forward Declarations
 *----------------------------------------------------------*/
__attribute__((unused)) static void vTest1_TaskA(void *pv);
__attribute__((unused)) static void vTest1_TaskB(void *pv);
__attribute__((unused)) static void vTest2_TaskA(void *pv);
__attribute__((unused)) static void vTest2_TaskB(void *pv);
__attribute__((unused)) static void vTest2_TaskC(void *pv);
__attribute__((unused)) static void vTest3_SoftTask(void *pv);
__attribute__((unused)) static void vTest3_HardTask(void *pv);
__attribute__((unused)) static void vTest4_TimingTask(void *pv);
__attribute__((unused)) static void vTest5_SeqTask(void *pv);
__attribute__((unused)) static void vTest6_DeadlineTask(void *pv);
__attribute__((unused)) static void vTest7_SemaphoreReleasing(void *pv);
__attribute__((unused)) static void vTest7_SemaphoreReleasing2(void *pv);
__attribute__((unused)) static void vTest8_TaskA(void *pv);
__attribute__((unused)) static void vTest8_TaskB(void *pv);

/*-----------------------------------------------------------
 * Shared State
 *----------------------------------------------------------*/
static volatile BaseType_t xTaskADone = pdFALSE;
static volatile BaseType_t xTaskBDone = pdFALSE;
static volatile TickType_t xTaskAStart = 0;
static volatile TickType_t xTaskAEnd = 0;
static volatile TickType_t xTaskBStart = 0;
static volatile TickType_t xTaskBEnd = 0;
static volatile uint8_t ucTasksCompleted = 0;

/*-----------------------------------------------------------
 * Shared Semaphore
 *----------------------------------------------------------*/
static SemaphoreHandle_t xSem;

int xHardDone;

int xHardIsAllDone[4] = {0, 0, 0, 0};

TickType_t tGlobalCounter = 0;
/*-----------------------------------------------------------
 * Simulated Work
 *----------------------------------------------------------*/
static void vDoWork(uint32_t ulTicks) {
  volatile uint32_t i;
  for (i = 0; i < ulTicks * 10000; i++) {
  }
}

/*-----------------------------------------------------------
 * Result Reporter - Runs at end of major cycle
 *----------------------------------------------------------*/
static void vResultReporter(void *pv) {
  (void)pv;
  char buf[120];
  BaseType_t xPass = pdFALSE;

  /* Wait for test tasks to complete */
  volatile int i;
  for (i = 0; i < 50000; i++) {
  }

#if TEST_NUMBER == 1
  /*
   * TEST 1 - Overlapping HRT Tasks (Stress Test)
   * Two HARD tasks with overlapping windows [0-100] and [50-150]
   * Expected: Both complete, Task_B starts after Task_A's window ends
   */
  UART_printf("\r\n========================================\r\n");
  UART_printf("Test 1 - Overlapping HRT Tasks\r\n");

  snprintf(buf, sizeof(buf), "  Task_A [0-100]: %s (end=%lu)\r\n",
           xTaskADone ? "COMPLETE" : "INCOMPLETE",
           (unsigned long)(xTaskAEnd % 1000));
  UART_printf(buf);
  snprintf(buf, sizeof(buf), "  Task_B [50-150]: %s (start=%lu)\r\n",
           xTaskBDone ? "COMPLETE" : "INCOMPLETE",
           (unsigned long)(xTaskBStart % 1000));
  UART_printf(buf);

  /* Pass if both completed and B started after A's window */
  if (xTaskADone && xTaskBDone) {
    xPass = pdTRUE;
  }

  UART_printf("----------------------------------------\r\n");
  snprintf(buf, sizeof(buf), "Test 1 - Overlapping HRT Tasks: %s\r\n",
           xPass ? "PASSED" : "FAILED");
  UART_printf(buf);

#elif TEST_NUMBER == 2
  /*
   * TEST 2 - Minimal Time Gaps (Edge Case)
   * Adjacent windows [0-100] and [100-200] with zero gap
   * Expected: Task_B starts exactly at tick 100
   */
  UART_printf("\r\n========================================\r\n");
  UART_printf("Test 2 - Minimal Time Gaps\r\n");

  TickType_t xBStartRel = xTaskBStart % 1000;
  snprintf(buf, sizeof(buf), "  Task_A [0-100]: end=%lu\r\n",
           (unsigned long)(xTaskAEnd % 1000));
  UART_printf(buf);
  snprintf(buf, sizeof(buf), "  Task_B [100-200]: start=%lu (expected 100)\r\n",
           (unsigned long)xBStartRel);
  UART_printf(buf);

  /* Pass if both complete and B starts at/after 100 */
  if (xTaskADone && xTaskBDone && xBStartRel >= 100 && xBStartRel < 110) {
    xPass = pdTRUE;
  }

  UART_printf("----------------------------------------\r\n");
  snprintf(buf, sizeof(buf), "Test 2 - Minimal Time Gaps: %s\r\n",
           xPass ? "PASSED" : "FAILED");
  UART_printf(buf);

#elif TEST_NUMBER == 3

  xHardDone = 0;

  /*
   * TEST 3 - SRT Preemption (Preemption Check)
   * SOFT task [0-200], HARD task [100-150]
   * Expected: HARD preempts SOFT at tick 100
   */
  UART_printf("\r\n========================================\r\n");
  UART_printf("Test 3 - SRT Preemption\r\n");

  TickType_t xHardStart = xTaskBStart % 1000;
  snprintf(buf, sizeof(buf), "  SOFT [0-200]: started at %lu\r\n",
           (unsigned long)(xTaskAStart % 1000));
  UART_printf(buf);
  snprintf(buf, sizeof(buf),
           "  HARD [100-150]: started at %lu (expected ~100)\r\n",
           (unsigned long)xHardStart);
  UART_printf(buf);

  /* Pass if HARD started within its window [100-150] */
  if (xTaskBDone && xHardStart >= 100 && xHardStart < 160) {
    xPass = pdTRUE;
  }

  UART_printf("----------------------------------------\r\n");
  snprintf(buf, sizeof(buf), "Test 3 - SRT Preemption: %s\r\n",
           xPass ? "PASSED" : "FAILED");
  UART_printf(buf);

#elif TEST_NUMBER == 4
  /*
   * TEST 4 - Timing Consistency
   * Task [50-150] should start at exactly tick 50
   * Expected: Variance < 5 ticks
   */
  UART_printf("\r\n========================================\r\n");
  UART_printf("Test 4 - Timing Consistency\r\n");

  TickType_t xStartRel = xTaskAStart % 1000;
  TickType_t xVariance =
      (xStartRel >= 50) ? (xStartRel - 50) : (50 - xStartRel);

  snprintf(buf, sizeof(buf), "  Task [50-150]: started at tick %lu\r\n",
           (unsigned long)xStartRel);
  UART_printf(buf);
  snprintf(buf, sizeof(buf), "  Variance from expected: %lu ticks\r\n",
           (unsigned long)xVariance);
  UART_printf(buf);

  /* Pass if variance is minimal (< 5 ticks) */
  if (xTaskADone && xVariance <= 5) {
    xPass = pdTRUE;
  }

  UART_printf("----------------------------------------\r\n");
  snprintf(buf, sizeof(buf), "Test 4 - Timing Consistency: %s\r\n",
           xPass ? "PASSED" : "FAILED");
  UART_printf(buf);

#elif TEST_NUMBER == 5
  /*
   * TEST 5 - Multiple Sequential HRT (Stress Test)
   * 5 sequential HARD tasks in windows [0-100], [100-200], etc.
   * Expected: All 5 complete in order
   */
  UART_printf("\r\n========================================\r\n");
  UART_printf("Test 5 - Multiple Sequential HRT\r\n");

  snprintf(buf, sizeof(buf), "  Tasks completed: %d/5\r\n", ucTasksCompleted);
  UART_printf(buf);

  /* Pass if all 5 tasks completed */
  if (ucTasksCompleted >= 5) {
    xPass = pdTRUE;
  }

  UART_printf("----------------------------------------\r\n");
  snprintf(buf, sizeof(buf), "Test 5 - Multiple Sequential HRT: %s\r\n",
           xPass ? "PASSED" : "FAILED");
  UART_printf(buf);

#elif TEST_NUMBER == 6
  /*
   * TEST 6 - Deadline Miss (Violation Test)
   * Task attempts to run for 40 ticks in a 20-tick window [0-20].
   * Expected: Task should be KILLED at tick 20.
   * If it completes, the DEADLINE ENFORCEMENT FAILED.
   */
  UART_printf("\r\n========================================\r\n");
  UART_printf("Test 6 - Deadline Miss Violation\r\n");

  snprintf(buf, sizeof(buf), "  Task [0-20] (Job=40): %s\r\n",
           xTaskADone ? "COMPLETED (BAD)" : "KILLED/INCOMPLETE (GOOD)");
  UART_printf(buf);

  if (xTaskADone) {
    /* Task completed -> It overrun its deadline -> KERNEL FAILURE */
    xPass = pdFALSE;
    UART_printf("  FAILURE: Task exceeded deadline and finished!\r\n");
    UART_printf(" TEST PASSED \r\n");
  } else {
    /* Task did determine -> KERNEL SUCCESS (Enforcement worked) */
    xPass = pdTRUE;
    UART_printf("  PASSED : Task was terminated or did not finish.\r\n");
  }

  UART_printf("----------------------------------------\r\n");
  snprintf(buf, sizeof(buf), "Test 6 - Deadline Miss: %s\r\n",
           xPass ? "PASSED" : "DEADLINE ENFORCEMENT FAILED");
  UART_printf(buf);

#elif TEST_NUMBER == 7
  UART_printf("\r\n========================================\r\n");
  UART_printf("Test 7 - Semaphore release\r\n");

  UART_printf("[check log]: log printed from kernel + not accesing / accesing "
              "resources");

  UART_printf("----------------------------------------\r\n");
  snprintf(buf, sizeof(buf), "Test 7 - Semaphore release %s\r\n",
           xPass ? "PASSED" : "DEADLINE ENFORCEMENT FAILED");
  UART_printf(buf);

#elif TEST_NUMBER == 8
  UART_printf("\r\n========================================\r\n");
  UART_printf("Test 8 - POLLING \r\n");

  UART_printf("[check log]: get global non critical resources");

  UART_printf("----------------------------------------\r\n");
#endif

  UART_printf("========================================\r\n");
  UART_printf("\r\nTest complete.\r\n");
  vTaskSuspend(NULL);
}

/*-----------------------------------------------------------
 * Test 6: Deadline Violator
 *----------------------------------------------------------*/
__attribute__((unused)) static void vTest6_DeadlineTask(void *pv) {
  (void)pv;
  xTaskAStart = xTaskGetTickCount();
  UART_printf("[T6] Deadline Task [0-20] START (will try 40 ticks work)\r\n");

  /* Try to do 40 ticks of work in a 20 tick window */
  vTaskDelay(50);

  xTaskAEnd = xTaskGetTickCount();
  xTaskADone = pdTRUE; /* Should NOT be reached if deadline is enforced */

  UART_printf(
      "[T6] Deadline Task COMPLETE (Error: Should have been killed!)\r\n");
  vTaskSuspend(NULL);
}

/*-----------------------------------------------------------
 * Test 1: Overlapping HRT Tasks
 *----------------------------------------------------------*/
__attribute__((unused)) static void vTest1_TaskA(void *pv) {
  (void)pv;
  xTaskAStart = xTaskGetTickCount();
  UART_printf("[T1] Task_A [0-100] running\r\n");
  vDoWork(3000);
  xTaskAEnd = xTaskGetTickCount();
  UART_printf("[T1] Task_A complete\r\n");

  vTaskSuspend(NULL);
}

__attribute__((unused)) static void vTest1_TaskB(void *pv) {
  (void)pv;
  xTaskBStart = xTaskGetTickCount();
  UART_printf("[T1] Task_B [50-150] running\r\n");
  char b[64];
  snprintf(b, sizeof(b), " TASK B START AT --> %d\r\n",
           (unsigned long)(xTaskBStart % 1000));
  UART_printf(b);
  xTaskADone = pdTRUE;
  vDoWork(50);
  xTaskBEnd = xTaskGetTickCount();
  xTaskBDone = pdTRUE;
  UART_printf("[T1] Task_B complete\r\n");
  vTaskSuspend(NULL);
}

/*-----------------------------------------------------------
 * Test 2: Minimal Time Gaps
 *----------------------------------------------------------*/
__attribute__((unused)) static void vTest2_TaskA(void *pv) {
  (void)pv;
  UART_printf("[T2] Task_A [0-100] running\r\n");
  char b[64];
  snprintf(b, sizeof(b), " TASK A HARD START AT --> %d\r\n",
           (unsigned long)(xTaskGetTickCount() % 1000));
  UART_printf(b);
  vTaskDelay(100);
  snprintf(b, sizeof(b), " TASK A HARD END AT --> %d\r\n",
           (unsigned long)(xTaskGetTickCount() % 1000));
  UART_printf(b);
  xTaskAEnd = xTaskGetTickCount();
  xTaskADone = pdTRUE;
  UART_printf("[T2] Task_A complete\r\n");
  xHardIsAllDone[0] = 1;
  vTaskSuspend(NULL);
}

__attribute__((unused)) static void vTest2_TaskB(void *pv) {
  (void)pv;
  xTaskBStart = xTaskGetTickCount();
  char b[64];
  snprintf(b, sizeof(b), " TASK B HARD START AT --> %d\r\n",
           (unsigned long)(xTaskGetTickCount() % 1000));
  UART_printf(b);
  UART_printf("[T2] Task_B [100-200] running\r\n");
  vTaskDelay(99);
  snprintf(b, sizeof(b), " TASK B HARD END AT --> %d\r\n",
           (unsigned long)(xTaskGetTickCount() % 1000));
  UART_printf(b);
  xTaskBEnd = xTaskGetTickCount();
  xTaskBDone = pdTRUE;
  UART_printf("[T2] Task_B complete\r\n");
  xHardIsAllDone[1] = 1;
  vTaskSuspend(NULL);
}

__attribute__((unused)) static void vTest2_TaskC(void *pv) {
  (void)pv;
  for (;;) {

    char b[64];
    snprintf(b, sizeof(b), " TASK C SOFT AT --> %d\r\n",
             (unsigned long)(xTaskGetTickCount() % 1000));
    UART_printf(b);
    vDoWork(700);
  }
}

__attribute__((unused)) static void vTest2_TaskE(void *pv) {
  (void)pv;
  char b[64];
  snprintf(b, sizeof(b), " TASK E HARD START AT --> %d\r\n",
           (unsigned long)(xTaskGetTickCount() % 1000));
  UART_printf(b);
  UART_printf("[T5] Task_E [500-600] running\r\n");
  vTaskDelay(100);
  snprintf(b, sizeof(b), " TASK E HARD END AT --> %d\r\n",
           (unsigned long)(xTaskGetTickCount() % 1000));
  UART_printf(b);
  xTaskBDone = pdTRUE;
  UART_printf("[T5] Task_E complete\r\n");
  xHardIsAllDone[3] = 1;
  if (xHardIsAllDone[0] == 1) {
    if (xHardIsAllDone[1] == 1) {
      if (xHardIsAllDone[2] == 1) {
        if (xHardIsAllDone[3] == 1) {
          UART_printf("PASSED\r\n");
          xHardIsAllDone[0] = 1;
          xHardIsAllDone[1] = 1;
          xHardIsAllDone[2] = 1;
          xHardIsAllDone[3] = 1;
        }
      }
    }
  }
  vTaskSuspend(NULL);
}

__attribute__((unused)) static void vTest2_TaskD(void *pv) {
  (void)pv;
  char b[64];
  snprintf(b, sizeof(b), " TASK D HARD START AT --> %d\r\n",
           (unsigned long)(xTaskGetTickCount() % 1000));
  UART_printf(b);
  UART_printf("[T4] Task_D [450-500] running\r\n");
  vTaskDelay(50);
  snprintf(b, sizeof(b), " TASK D HARD END AT --> %d\r\n",
           (unsigned long)(xTaskGetTickCount() % 1000));
  UART_printf(b);
  xTaskBDone = pdTRUE;
  UART_printf("[T4] Task_D complete\r\n");
  xHardIsAllDone[2] = 1;

  vTaskSuspend(NULL);
}

/*-----------------------------------------------------------
 * Test 3: SRT Preemption
 *----------------------------------------------------------*/
__attribute__((unused)) static void vTest3_SoftTask(void *pv) {
  (void)pv;
  char b[64];
  snprintf(b, sizeof(b), "[T3] SOFT task starting --> %d\r\n",
           (unsigned long)(xTaskGetTickCount() % 1000));
  UART_printf(b);
  vTaskDelay(107);
  vDoWork(50);
  if (xHardDone == 1) {
    UART_printf("Hard preempted Soft! \r\n");
    UART_printf("PASSED\r\n");
    xHardDone = 0;
  }
  snprintf(b, sizeof(b), "[T3] SOFT task end --> %d\r\n",
           (unsigned long)(xTaskGetTickCount() % 1000));
  UART_printf(b);
  vTaskSuspend(NULL);
}

__attribute__((unused)) static void vTest3_HardTask(void *pv) {
  (void)pv;
  xTaskBStart = xTaskGetTickCount();
  char b[64];
  snprintf(b, sizeof(b), "[T3] HARD task starting --> %d\r\n",
           (unsigned long)(xTaskGetTickCount() % 1000));
  UART_printf(b);
  vDoWork(50);
  xHardDone = 1;
  xTaskBDone = pdTRUE;
  snprintf(b, sizeof(b), "[T3] HARD task end --> %d\r\n",
           (unsigned long)(xTaskGetTickCount() % 1000));
  UART_printf(b);
  vTaskSuspend(NULL);
}

/*-----------------------------------------------------------
 * Test 4: Timing Consistency
 *----------------------------------------------------------*/
__attribute__((unused)) static void vTest4_TimingTask(void *pv) {
  (void)pv;
  char b[64];
  xTaskAStart = xTaskGetTickCount();
  snprintf(b, sizeof(b), "[T4] task starting --> %d\r\n",
           (unsigned long)(xTaskGetTickCount() % 1000));
  UART_printf(b);
  UART_printf("[T4] Timing task running\r\n");
  vDoWork(30);
  xTaskAEnd = xTaskGetTickCount();
  xTaskADone = pdTRUE;
  UART_printf("[T4] Timing task complete\r\n");
  vTaskSuspend(NULL);
}

/*-----------------------------------------------------------
 * Test 5: Multiple Sequential HRT
 *----------------------------------------------------------*/
__attribute__((unused)) static void vTest5_SeqTask(void *pv) {
  uint32_t id = (uint32_t)(uintptr_t)pv;
  char buf[40];
  snprintf(buf, sizeof(buf), "[T5] Task_%lu running --> %d\r\n", id,
           xTaskGetTickCount() % 1000);
  UART_printf(buf);
  vTaskDelay(100);
  ucTasksCompleted++;
  snprintf(buf, sizeof(buf), "[T5] Task_%lu complete --> %d\r\n", id,
           xTaskGetTickCount() % 1000);
  UART_printf(buf);
  vTaskSuspend(NULL);
}

/*-----------------------------------------------------------
 * Test 7: semaphore handling
 *----------------------------------------------------------*/
int xCnt = 0;
__attribute__((unused)) static void vTest7_SemaphoreReleasing(void *pv) {
  /*TAKE SEMAPORE*/
  (void)pv;

  if (xSemaphoreTake(xSem, portMAX_DELAY) == pdTRUE) {
    UART_printf("[ TASK F ] SEMAPHORE TAKEN\r\n");
    if (xCnt != 0) {
      UART_printf("[ TASK F ] PASSED\r\n");
    }
    xCnt++;
  } else {
    UART_printf("[ TASK F ] STILL TAKEN\r\n");
    UART_printf("[ TASK F ] NOT\r\n");
  }

  vTaskSuspend(NULL);
}
__attribute__((unused)) static void vTest7_SemaphoreReleasing2(void *pv) {
  (void)pv;
  if (xSemaphoreTake(xSem, pdMS_TO_TICKS(10)) == pdTRUE) {
    UART_printf(" [ TASK G ]I IPOTETICALLY NOT RUN\r\n");
    UART_printf("[ TASK F ] ERROR\r\n");
    xSemaphoreGive(xSem);
  } else {
    UART_printf("[ TASK G ] OK SEMAPHORE SILL TAKEN\r\n");
  }

  vTaskSuspend(NULL);
}

__attribute__((unused)) static void vTest8_TaskA(void *pv) {

  if (tGlobalCounter == 2) {
    UART_printf("[LOG]: PASSED\n");
    tGlobalCounter--;
    tGlobalCounter--;
  }

  tGlobalCounter++;
  char buf[40];
  snprintf(buf, sizeof(buf), "[T8A]: running COUNTER VALUE --> %d\r\n",
           tGlobalCounter);
  UART_printf(buf);
  UART_printf("END\r\n");
  vTaskSuspend(NULL);
}
__attribute__((unused)) static void vTest8_TaskB(void *pv) {

  if (tGlobalCounter == 1) {
    UART_printf("[T8A]: ok received correct counter, i'll decrement it");
  }
  tGlobalCounter++;
  char buf[40];
  snprintf(buf, sizeof(buf), "[T8B]: running COUNTER VALUE --> %d\r\n",
           tGlobalCounter);
  UART_printf(buf);
  UART_printf("END\r\n");
  vTaskSuspend(NULL);
}
/*-----------------------------------------------------------
 * Main Entry Point
 *----------------------------------------------------------*/
int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  char buf[60];

  UART_init();

  snprintf(buf, sizeof(buf), "Timeline Scheduler Test Suite - Test %d\r\n",
           TEST_NUMBER);
  UART_printf(buf);

#if TEST_NUMBER == 1
  /* Overlapping HRT: [0-100] and [50-150] */
  const TimelineTaskConfig_t xTest1Schedule[] = {
      {"T1A", vTest1_TaskA, NULL, 256, 0, 100, TASK_TYPE_HARD},
      {"T1B", vTest1_TaskB, NULL, 256, 50, 150, TASK_TYPE_HARD},
      {"Rep", vResultReporter, NULL, 512, 800, 900, TASK_TYPE_HARD},
      {NULL}};
  vConfigureScheduler(xTest1Schedule);

#elif TEST_NUMBER == 2
  /* Minimal gaps: [0-100] then [100-200] */
  const TimelineTaskConfig_t xTest2Schedule[] = {
      {"T2A", vTest2_TaskA, NULL, 256, 0, 100, TASK_TYPE_HARD},
      {"T2B", vTest2_TaskB, NULL, 256, 100, 200, TASK_TYPE_HARD},
      {"T2C", vTest2_TaskC, NULL, 256, 50, 700, TASK_TYPE_SOFT},
      {"T2D", vTest2_TaskD, NULL, 256, 450, 500, TASK_TYPE_HARD},
      {"T2E", vTest2_TaskE, NULL, 256, 500, 600, TASK_TYPE_HARD},

      {"Rep", vResultReporter, NULL, 512, 800, 900, TASK_TYPE_HARD},
      {NULL}};
  vConfigureScheduler(xTest2Schedule);

#elif TEST_NUMBER == 3
  /* SRT Preemption: SOFT [0-200], HARD [100-150] */
  const TimelineTaskConfig_t xTest3Schedule[] = {
      {"Soft", vTest3_SoftTask, NULL, 256, 0, 200, TASK_TYPE_SOFT},
      {"Hard", vTest3_HardTask, NULL, 256, 100, 150, TASK_TYPE_HARD},
      {"Rep", vResultReporter, NULL, 512, 800, 900, TASK_TYPE_HARD},
      {NULL}};
  vConfigureScheduler(xTest3Schedule);

#elif TEST_NUMBER == 4
  /* Timing consistency: task at [50-150] */
  const TimelineTaskConfig_t xTest4Schedule[] = {
      {"Tim", vTest4_TimingTask, NULL, 256, 50, 150, TASK_TYPE_HARD},
      {"Rep", vResultReporter, NULL, 512, 800, 900, TASK_TYPE_HARD},
      {NULL}};
  vConfigureScheduler(xTest4Schedule);

#elif TEST_NUMBER == 5
  /* Multiple Sequential: 5 tasks back-to-back */
  const TimelineTaskConfig_t xTest5Schedule[] = {
      {"S0", vTest5_SeqTask, (void *)0, 256, 0, 100, TASK_TYPE_HARD},
      {"S1", vTest5_SeqTask, (void *)1, 256, 100, 200, TASK_TYPE_HARD},
      {"S2", vTest5_SeqTask, (void *)2, 256, 200, 300, TASK_TYPE_HARD},
      {"S3", vTest5_SeqTask, (void *)3, 256, 300, 400, TASK_TYPE_HARD},
      {"S4", vTest5_SeqTask, (void *)4, 256, 400, 500, TASK_TYPE_HARD},
      {"Rep", vResultReporter, NULL, 512, 800, 900, TASK_TYPE_HARD},
      {NULL}};
  vConfigureScheduler(xTest5Schedule);

#elif TEST_NUMBER == 6
  /*
   * TEST 6 - Deadline Miss (Violation Test)
   * Task has window [0-20], but tries to run for 40 ticks.
   * Expected:
   *  - If enforcement works: Task killed at tick 20. Reporter sees incomplete.
   *  - If enforcement fails: Task completes at tick ~40. Reporter sees complete
   * (FAILURE).
   */
  const TimelineTaskConfig_t xTest6Schedule[] = {
      {"Viol", vTest6_DeadlineTask, NULL, 256, 0, 20, TASK_TYPE_HARD},
      {"Rep", vResultReporter, NULL, 512, 100, 200, TASK_TYPE_HARD},
      {NULL}};
  vConfigureScheduler(xTest6Schedule);

#elif (TEST_NUMBER == 7)
  xSem = xSemaphoreCreateBinary();
  xSemaphoreGive(xSem);
  const TimelineTaskConfig_t xTest7Schedule[] = {
      {"Take", vTest7_SemaphoreReleasing, NULL, 256, 0, 400, TASK_TYPE_SOFT,
       &xSem},
      {"Check", vTest7_SemaphoreReleasing2, NULL, 256, 500, 900,
       TASK_TYPE_HARD},
      {NULL}};
  vConfigureScheduler(xTest7Schedule);

#elif (TEST_NUMBER == 8)
  const TimelineTaskConfig_t xTest7Schedule[] = {
      {"Take", vTest8_TaskA, NULL, 256, 0, 400, TASK_TYPE_SOFT, NULL},
      {"Check", vTest8_TaskB, NULL, 256, 500, 900, TASK_TYPE_HARD, NULL},
      {NULL}};
  vConfigureScheduler(xTest7Schedule);

#endif

  vTaskStartScheduler();
  for (;;) {
  }
}
