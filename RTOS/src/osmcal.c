#include  "../../RTOS/inc/osmcal.h"
#include  "../../RTOS/inc/osmcal_conf.h"
#include "../../Core/Inc/main.h"
#include <stdint.h>


uint32_t current_task = 1; // starting from task one, leaving index 0 to the idle task
uint32_t global_tick_count = 0;


typedef enum {
	READY,
	BLOCKED
} task_state_t;

/* Task/Process Control block typedef */
typedef struct {
	uint32_t psp_value;
	uint32_t block_count;
	task_state_t state;
	void (*task_handler)(void);
} tcb_t;

tcb_t user_tasks[MAX_TASKS];

static uint8_t s_num_tasks = 0;
static uint32_t s_next_stack_addr = SRAM_END;

// uint32_t task_psp[MAX_TASKS] = {T1_STACK_START, T2_STACK_START, T3_STACK_START, T4_STACK_START};


void systick_init(uint32_t tickHz) {
	// calculate reload value --> sysclk / reload value
	uint32_t reload = (SystemCoreClock / tickHz) - 1; // decrement by one to take multishot in consideration

	SysTick->LOAD = reload;

	// enable systick exception
	SysTick->CTRL |= (1 << 1);
	// enable clk source
	SysTick->CTRL |= (1 << 2);
	// enable systick
	SysTick->CTRL |= (1 << 0);
}

__attribute__ ((naked)) void sched_stack_init(uint32_t stacktop) {
	// move the argument of the function 'stacktop' to the MSP stack pointer through Regsiter R0
	__asm volatile("MSR MSP, R0");
	// we can also access the argument like so:__asm volatile("MSR MSP, %0"::"r"(stacktop));
	__asm volatile("BX LR");
}


uint8_t os_task_create(void (*task_handler)(void)) {
	if (s_num_tasks >= MAX_TASKS) {
		return 255;
	}

	uint8_t task_id = s_num_tasks;
	s_num_tasks++;

	uint32_t stack_start = s_next_stack_addr;
	s_next_stack_addr -= TASK_STACK_SIZE;

	uint32_t *local_taskpsp;

	// task PSP initialization
	user_tasks[task_id].psp_value = stack_start;

	// task handler initialization
	user_tasks[task_id].task_handler = task_handler;

	// initialize task as ready
	user_tasks[task_id].state = READY;

	local_taskpsp = (uint32_t *)user_tasks[task_id].psp_value;
	local_taskpsp--; // point to the end of the stack and the beginning of the stacked registers (XPSR register)
	*local_taskpsp = DUMMY_XPSR;

	local_taskpsp--; // point to the next register in the stack, which is the PC
	*local_taskpsp = (uint32_t)user_tasks[task_id].task_handler;

	local_taskpsp--; // point to LR
	*local_taskpsp = DUMMY_LR;

	// initialize 13 GPRs with zeros
	for (int j = 0; j < 13; j++) {
		local_taskpsp--;
		*local_taskpsp = 0x0;
	}

	// update the actual PSP
	user_tasks[task_id].psp_value = (uint32_t)local_taskpsp;
    
    return task_id;
}


void sysfaults_enable(void) {
	// enable memfault, Bus fault, and usage faults
	SCB->SHCSR |= (SCB_SHCSR_MEMFAULTENA_Msk);
	SCB->SHCSR |= (SCB_SHCSR_BUSFAULTENA_Msk);
	SCB->SHCSR |= (SCB_SHCSR_USGFAULTENA_Msk);
}

void task_delay(uint32_t blocking_time) {
	if (current_task != 0) {
		ENTER_CRITICAL_SECTION();
		// update the blocking period
		user_tasks[current_task].block_count = blocking_time + global_tick_count;

		// change the task's state
		user_tasks[current_task].state = BLOCKED;

		EXIT_CRITICAL_SECTION();
		// call scheduler (trigger pendsv)
		SCB->ICSR |= (1 << 28);
	}
}

static uint32_t get_current_psp(void) {
	// the return value is by default stored in R0
	return user_tasks[current_task].psp_value;
}

static void save_psp(uint32_t current_psp) {
	user_tasks[current_task].psp_value = current_psp;
}

static void update_next_task(void) {
	for (int i = 0; i < MAX_TASKS; i++) {
		current_task++;
		current_task %= MAX_TASKS;
		if ((user_tasks[i].state == READY) && (i != 0)) {
			// check if non-idle tasks are ready
			break;
		}
	}
	if (user_tasks[current_task].state != READY) { // all tasks are blocked
		current_task = 0; // switch to idle task
	}
}

// switching from MSP to PSP when calling a task (MSP is used by default)
__attribute__((naked)) void switch_to_psp(void) {
	// push LR to stack to preserve the value of the caller function before calling get_current_psp
	__asm volatile("PUSH {LR}");
	// initialize PSP with the task stack
	// branch and link to the get_current_psp function
	__asm volatile("BL get_current_psp");
	__asm volatile("MSR PSP, R0");
	// POP the value of the address of the main function to go back into the LR register
	__asm volatile("POP {LR}");
	// switch to PSP pointer using CONTROL register
	__asm volatile("MOV R0, #0x02"); // 0x00000010
	__asm volatile("MSR CONTROL, R0");
	// go back to the caller
	__asm volatile("BX LR");
}

// where the context switching happens
__attribute__((naked)) void PendSV_Handler(void) {
	// save context of the current task
	// 1. get current value of PSP
	__asm volatile("MRS R0, PSP");
	// 2. using the PSP value we store SF2 (R4-->R11)
	// we can't use PUSH instruction because inside handlers we use MSP not PSP
	__asm volatile("STMDB R0!, {R4-R11}");   // store memory and decrement (similar to PUSH), final address value is stored in R0
	// 3. save current value of PSP
	__asm volatile("PUSH {LR}");
	__asm volatile("BL save_psp");

	// retrieve context of the next task
	// 1. decide which is the next task to run
	__asm volatile("BL update_next_task");
	// 2. get its PSP value
	__asm volatile("BL get_current_psp");
	__asm volatile("POP {LR}");
	// 3. retrieve SF2 (R4-->R11) using PSP value and store them in the processor registers
	__asm volatile("LDMIA R0!, {R4-R11}"); // load values from R0 (which is the value of the PSP that points to R4 of the next task)
	// +--> then increment (simulate POP instruction) and store in the processor registers (from R4 to R11)
	// update PSP value
	__asm volatile("MSR PSP, R0");
	// exit
	__asm volatile("BX LR");
}

static void unblock_tasks(void) {
	for (int i = 1; i < MAX_TASKS; i++) {
		if (user_tasks[i].state == BLOCKED) {
			if (user_tasks[i].block_count == global_tick_count) {
				user_tasks[i].state = READY;
			}
		}
	}
}

void SysTick_Handler(void) {
	// update global tick count
	global_tick_count++;
	// unblock tasks
	unblock_tasks();
	// trigger pendsv for context switching
	SCB->ICSR |= (1 << 28);
}

void idle_task_handler(void) {
	while(1);
}
