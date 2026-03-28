# README

this is a small educational. It demonstrates how a Cortex-M4 can run multiple independent tasks using:

- `SysTick` as the system tick source
- `PendSV` for deferred context switching
- `PSP` for per-task stacks
- `MSP` for handler mode and kernel-level exception handling

This is not a full RTOS. It is a compact bare-metal kernel demo focused on the core mechanics of task creation, blocking delays, and context switching.

## Scheduler Design

The kernel uses a fixed-size task table:

```c
typedef struct {
    uint32_t psp_value;
    uint32_t block_count;
    task_state_t state;
    void (*task_handler)(void);
} tcb_t;
```

Each task has:

- a saved process stack pointer
- a wake-up tick in `block_count`
- a state: `READY` or `BLOCKED`
- an entry function

Tasks are stored in:

```c
tcb_t user_tasks[MAX_TASKS];
```

`MAX_TASKS` is configured in `RTOS/inc/osmcal_conf.h` and currently set to `5`.

## Stack Model

The project uses separate stacks for thread mode and handler mode:

- `MSP` is used by exceptions and kernel-side exception entry
- `PSP` is used by tasks

Each task gets a fixed stack of `1024` bytes:

```c
#define TASK_STACK_SIZE 1024
```

The scheduler reserves the top part of SRAM for task stacks and moves `MSP` below them:

```c
#define SCHED_TASK_STACK ((SRAM_END) - (MAX_TASKS * TASK_STACK_SIZE))
```

Conceptually:

```text
High RAM
+---------------------------+  SRAM_END
| Task 0 stack (idle)       |
| Task 1 stack              |
| Task 2 stack              |
| Task 3 stack              |
| Task 4 stack              |
+---------------------------+
| Scheduler / MSP region    |  <- MSP moved here
+---------------------------+
| .data / .bss / heap       |
+---------------------------+
Low RAM
```

## Task Creation

`os_task_create()` allocates a stack for the task and manually builds an initial exception frame. That frame includes:

- `xPSR = 0x01000000` to set the Thumb state bit
- `PC = task_handler`
- `LR = 0xFFFFFFFD` to return to thread mode using `PSP`
- zeroed general-purpose registers

This allows the scheduler to restore a task as if it had already been interrupted once.

## Why SysTick And PendSV

The scheduler uses two Cortex-M exceptions with different jobs:

- `SysTick` is the periodic time source
- `PendSV` is the lowest-priority deferred exception used for context switching

`SysTick_Handler()` does lightweight timing work:

- increment `global_tick_count`
- unblock tasks whose `block_count` has expired
- pend `PendSV`

`PendSV_Handler()` does the heavy work:

- read current `PSP`
- save `R4-R11` to the current task stack
- store the updated `PSP` in the current task's TCB
- choose the next task
- load that task's `PSP`
- restore `R4-R11`
- return from exception

This split is important because it keeps the timer interrupt short and lets the actual task switch happen only after higher-priority interrupts are finished.

## Delay And Blocking

Tasks block themselves with:

```c
task_delay(uint32_t blocking_time);
```

When called:

1. the current task enters a critical section
2. its wake-up time is stored in `block_count`
3. its state becomes `BLOCKED`
4. `PendSV` is triggered

The task stays blocked until `SysTick_Handler()` advances the global tick to the matching wake-up count.

## Current Demo Behavior

Assuming a 1 ms tick, the LED timing is:

```text
PA15: toggle every 125 ms
PA12: toggle every 250 ms
PA11: toggle every 500 ms
PA10: toggle every 1000 ms
```

This makes the project a useful visual demo for:

- task timing
- blocking and wake-up behavior
- scheduler-driven concurrency

## Important Notes

This project is intentionally minimal:

- no dynamic memory allocation for tasks
- no priorities
- no semaphores, mutexes, or queues
- no sleep modes
- no user/kernel privilege separation
- no stack overflow checking

It is best understood as a learning project for Cortex-M scheduling internals rather than a production-ready RTOS.

## Known Limitations

- The scheduler currently uses a fixed task table and fixed stack sizes.
- The first task is started directly from `main()` after switching to `PSP`, rather than being entered through the same restore path as later context switches.
- The task selection logic in `update_next_task()` should be reviewed carefully; it advances `current_task`, but the readiness check uses the loop index, which can cause incorrect selection behavior.
