#include "kernel/sched.hpp"
#include "kernel/page_alloc.hpp"
#include "arch/sysreg.hpp"

static TCB tasks[MAX_TASKS];   /* 任务表：最多 8 个线程 */
static int  task_count;        /* 已创建的线程数 */
static TCB *current;           /* 当前正在运行的线程（调度器维护） */
static TCB boot;               /* 停车场：存 kmain 的现场，不参与轮转 */
static volatile u64 ticks_ms;  /* 系统已运行毫秒数（timer 中断累计，volatile 防优化） */

#define TICK_MS 200            /* 一个时间片 200ms，必须与 timer.cpp 的 freq/5 一致 */

/* idle 任务：永远 READY，调度器兜底——就绪队列永不空，schedule() 永远切得到任务。
 * 没活干就 wfi 睡 CPU，等下一个中断（定时器）来唤醒。 */
static void idle_main()
{
    for (;;) __asm__ __volatile__("wfi");
}

/* 线程入口包装：第一次切入新线程时，从假现场跳到这里 */
extern "C" void thread_trampoline()
{
    __asm__ __volatile__("msr DAIFClr, #2");  /* 新线程第一次进来，解除 IRQ 屏蔽 */
    current->entry();                         /* 调用这个线程真正的入口函数 */
    for (;;) __asm__ __volatile__("wfi");     /* 入口返回后死循环兜底 */
}

void sched_init()
{
    task_count = 0;
    current = nullptr;
    ticks_ms = 0;
    /* idle 最先创建（id=0），永远就绪，保证后面无论谁阻塞都有得切 */
    task_create(idle_main);
}

int task_create(void (*entry)())
{
    TCB *t = &tasks[task_count];
    u64 stack = (u64)alloc_page();         /* 每个线程一块独立 4KB 栈 */
    /* 在栈顶往下 96 字节布置完整假现场（和 switch_to 恢复的 6 组对齐） */
    u64 *frame = (u64 *)(stack + 4096 - 96);
    frame[0] = 0;                          /* x29(FP)，第一次进入不需要 */
    frame[1] = (u64)thread_trampoline;     /* x30：首次切入时 ret 到 trampoline */

    t->sp    = (u64)frame;
    t->stack = stack;
    t->state = TASK_READY;      /* 新任务默认就绪 */
    t->sleep_until = 0;         /* 未在睡眠 */
    t->entry = entry;
    t->next  = nullptr;         /* 尚未挂任何等待队列 */
    t->id    = task_count;
    return task_count++;        /* 返回线程号，然后 task_count 自增 */
}

void schedule()
{
    if (task_count == 0) {
        return;
    }

    TCB *prev = current;
    /* 当前任务若还在 RUNNING（被抢占/主动交权），先放回就绪队列；
     * 若已是 BLOCKED（睡眠/等信号量），保持不动，绝不能被选中 */
    if (prev && prev->state == TASK_RUNNING) {
        prev->state = TASK_READY;
    }

    /* 轮转：从 prev 的下一个 id 开始绕一圈，找第一个就绪任务 */
    int start = prev ? (prev->id + 1) : 0;
    TCB *next = nullptr;
    for (int i = 0; i < task_count; i++) {
        TCB *t = &tasks[(start + i) % task_count];
        if (t->state == TASK_READY) {
            next = t;
            break;
        }
    }
    if (!next) {
        return;                 /* 防御：idle 永远 READY，理论到不了这里 */
    }
    if (next == prev) {
        next->state = TASK_RUNNING;   /* 就绪的只有自己：原地继续，省一次切换 */
        return;
    }
    current = next;             /* 先改 current（trampoline 要读它） */
    next->state = TASK_RUNNING;
    switch_to(prev ? prev : &boot, next);  /* 有 prev 存 prev，否则存 boot */
}

/* 当前任务睡 ms 毫秒：置 BLOCKED 后主动交权，由 timer 中断到点唤醒。
 * 关键：先写 sleep_until 再写 state —— 反了的话，中断窗口里
 * tick_update 会拿旧 sleep_until 误判（先写 sleep_until 的窗口无害，
 * 因为 state 还没变 BLOCKED，唤醒逻辑不会碰这个任务）。 */
void task_sleep(u64 ms)
{
    if (!current) {
        return;
    }
    u64 freq = READ_SYSREG(CNTFRQ_EL0);
    u64 now  = READ_SYSREG(CNTPCT_EL0);
    current->sleep_until = now + (freq / 1000) * ms;  /* 绝对时间（抗漂移） */
    current->state = TASK_BLOCKED;
    schedule();                /* 主动交权，回来时睡眠已结束 */
}

u64 uptime_ms()
{
    return ticks_ms;
}

TCB *sched_current()
{
    return current;
}

/* timer 中断里调用：累计毫秒 + 唤醒所有睡到点的任务。
 * 注意这里在中断上下文：只能改任务状态，绝不能 printf 或碰锁。 */
extern "C" void tick_update()
{
    ticks_ms += TICK_MS;

    u64 now = READ_SYSREG(CNTPCT_EL0);
    for (int i = 0; i < task_count; i++) {
        TCB *t = &tasks[i];
        if (t->state == TASK_BLOCKED && t->sleep_until != 0
            && (long long)(now - t->sleep_until) >= 0) {
            /* 带符号比较：无符号回绕时也不会误判（now 永远"晚于"过去的时刻） */
            t->sleep_until = 0;
            t->state = TASK_READY;   /* 唤醒：回到就绪队列，等时间片 */
        }
    }
}
