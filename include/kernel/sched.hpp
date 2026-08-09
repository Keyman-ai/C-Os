#ifndef SCHED_HPP
#define SCHED_HPP

#include "types.hpp"

#define MAX_TASKS 8

struct TCB {
    u64 sp;        /* 现场栈指针：不运行时的现场存在哪（核心字段） */
    u64 stack;     /* 该线程的独立内核栈（从页分配器拿的 4KB） */
    u32 id;        /* 线程编号 */
    void (*entry)(); /* 线程的入口函数：被切进来后从哪开始跑 */
};

void sched_init();
int  task_create(void (*entry)());
void schedule();
extern "C" void switch_to(TCB *prev, TCB *next);

#endif