#include "kernel/sched.hpp"
#include "kernel/page_alloc.hpp"

static TCB tasks[MAX_TASKS];  /* 任务表：最多 8 个线程 */
static int  task_count;       /* 已创建的线程数 */
static TCB *current;          /* 当前正在运行的线程（调度器维护） */
static TCB boot;              /* 停车场：存 kmain 的现场，不参与轮转 */

/* 线程入口包装：第一次切入新线程时，从假现场跳到这 */
extern "C" void thread_trampoline()
{
    __asm__ __volatile__("msr DAIFClr, #2");  /* 新线程第一次进来，解除 IRQ 屏蔽 */
    current->entry();                         /* 调用这个线程真正的入口函数 */
    for (;;) __asm__ __volatile__("wfi");     /* 入口返回后死循环兜底，绝不掉进垃圾地址 */
}

void sched_init()
{
    task_count = 0;
    current = nullptr;
}

int task_create(void (*entry)())
{
    TCB *t = &tasks[task_count];
    u64 stack = (u64)alloc_page();         /* 每个线程一块独立 4KB 栈 */
    /* 在栈顶往下 96 字节布置完整假现场 */
    u64 *frame = (u64 *)(stack + 4096 - 96); /* 96 字节 = 6 组，和 switch_to 恢复对齐 */
    frame[0] = 0;                           /* x29(FP)，第一次进入不需要 */
    frame[1] = (u64)thread_trampoline;      /* x30 = 返回地址：首次切入时 ret 到这 */

    t->sp    = (u64)frame;                  /* 现场指针指向假现场 */
    t->stack = stack;
    t->entry = entry;
    t->id    = task_count;
    return task_count++;                    /* 返回线程号，然后 task_count 自增 */
}

void schedule()
{
    if (task_count == 0) {
        return;
    }

    TCB *prev = current;
    TCB *next;
    if(prev == nullptr) {
        next = &tasks[0];
    } else {
        next = &tasks[(prev->id + 1) % task_count];
    }
    if (prev == next) {
        return;
    }
    current = next;                        /* 先改 current（trampoline 要读它） */
    switch_to(prev ? prev : &boot, next);  /* 有 prev 存 prev，否则存 boot */
}