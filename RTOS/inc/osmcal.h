#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>
#include "osmcal_conf.h"

// disable all global interrupts
#define ENTER_CRITICAL_SECTION()  do{__asm volatile("MOV R0, #1");__asm volatile("MSR PRIMASK, R0");} while(0)
#define EXIT_CRITICAL_SECTION()  do{__asm volatile("MOV R0, #0");__asm volatile("MSR PRIMASK, R0");}while(0)


#define DUMMY_XPSR 0x01000000
#define DUMMY_LR   0xFFFFFFFD


#define TASK_STACK_SIZE 1024
#define SRAM_START 0x20000000U
#define SRAM_SIZE  (128 * 1024)
#define SRAM_END   (SRAM_START + SRAM_SIZE)

#define SCHED_TASK_STACK  		((SRAM_END) - (MAX_TASKS * TASK_STACK_SIZE))

void systick_init(uint32_t tickHz);
__attribute__((naked)) void sched_stack_init(uint32_t stacktop);
uint8_t os_task_create(void (*task_handler)(void));
void sysfaults_enable(void);
void task_delay(uint32_t blocking_time);
__attribute__((naked))void switch_to_psp(void);
void idle_task_handler(void);

#endif
