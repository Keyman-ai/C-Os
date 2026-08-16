#ifndef SCHED_HPP
#define SCHED_HPP

#include "types.hpp"

#define MAX_TASKS 8

/* 任务状态机：调度器的核心数据结构 */
enum TaskState {
    TASK_RUNNING = 0,   /* 正在运行：当前持有 CPU */
    TASK_READY   = 1,   /* 就绪：想跑，等时间片（就绪队列 = 所有 state==READY 的任务） */
    TASK_BLOCKED = 2,   /* 阻塞：睡眠中或等信号量，不可调度，等被唤醒 */
};

struct TCB {
    u64 sp;             /* 现场栈指针：不运行时现场存在哪（核心字段） */
    u64 stack;          /* 该线程的独立内核栈（页分配器 4KB） */
    u32 id;             /* 线程编号 */
    u32 state;          /* 任务状态：TaskState 枚举 */
    u64 sleep_until;    /* 睡眠唤醒时刻（CNTPCT 绝对时间），0 = 未在睡眠 */
    void (*entry)();    /* 线程入口：被切进来后从哪开始跑 */
    TCB *next;          /* 信号量等待队列链表节点（FIFO 串联） */
};

void sched_init();
int  task_create(void (*entry)());
void schedule();
void task_sleep(u64 ms);        /* 当前任务睡 ms 毫秒，到点由定时器唤醒 */
u64  uptime_ms();               /* 系统已运行毫秒数（由 timer 中断累计） */
TCB *sched_current();           /* 返回当前任务指针（sem.cpp 需要拿它挂队列） */
extern "C" void tick_update();  /* timer 中断回调：累计时间 + 唤醒睡到点的任务 */
extern "C" void switch_to(TCB *prev, TCB *next);

#endif
