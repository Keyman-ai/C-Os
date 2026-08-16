#include "kernel/sem.hpp"
#include "kernel/sched.hpp"

/* 关/开 IRQ：本项目实测 #2 操作的就是 IRQ 屏蔽位（与 boot.S/trampoline 一致）。
 * DAIFSet = 屏蔽，DAIFClr = 解除。 */
#define IRQ_OFF() __asm__ __volatile__("msr DAIFSet, #2")
#define IRQ_ON()  __asm__ __volatile__("msr DAIFClr, #2")

void sem_init(Semaphore *s, int count)
{
    s->count = count;
    s->wait_head = nullptr;
    s->wait_tail = nullptr;
}

/* P 操作：拿不到资源就把当前任务挂到等待队列并阻塞。
 * 原子性：关中断包住「检查 count → 减 count / 挂队列」整段，
 * 防止丢失唤醒——否则 post 可能先执行（唤醒了一个还没挂进队列的"幽灵"），
 * 然后自己才阻塞，永远睡死。 */
void sem_wait(Semaphore *s)
{
    IRQ_OFF();
    if (s->count > 0) {
        s->count--;            /* 有资源：直接拿走 */
        IRQ_ON();
        return;
    }
    /* 资源不足：阻塞当前任务，挂到等待队列尾（FIFO，公平） */
    TCB *t = sched_current();
    if (!t) {                  /* 防御：kmain 早期无 current，理论到不了 */
        IRQ_ON();
        return;
    }
    t->state = TASK_BLOCKED;
    t->next  = nullptr;
    if (s->wait_tail) {
        s->wait_tail->next = t;   /* 尾插 */
        s->wait_tail = t;
    } else {
        s->wait_head = s->wait_tail = t;
    }
    IRQ_ON();                  /* 开中断后再交权！
                               * switch_to 不保存 DAIF，若关着中断切走，
                               * 新任务会带着"中断屏蔽"跑，整机失去抢占。 */
    schedule();                /* 主动让出 CPU；被唤醒切回时，即视为已拿到资源 */
}

/* V 操作：有等待者则把资源移交给队首并唤醒；无人等待才 count++。
 * 移交语义：wait 时 count 没减（减的是 post 这次），唤醒者直接从 sem_wait
 * 返回拿到资源，count 保持不变。 */
void sem_post(Semaphore *s)
{
    IRQ_OFF();
    if (s->wait_head) {
        TCB *w = s->wait_head;         /* FIFO：唤醒最早等的那个 */
        s->wait_head = w->next;
        if (!s->wait_head) {
            s->wait_tail = nullptr;
        }
        w->next = nullptr;
        w->state = TASK_READY;         /* 唤醒：回到就绪队列，等时间片 */
    } else {
        s->count++;
    }
    IRQ_ON();
}
