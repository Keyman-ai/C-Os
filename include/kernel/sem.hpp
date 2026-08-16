#ifndef SEM_HPP
#define SEM_HPP

#include "types.hpp"

struct TCB;   /* 前置声明：避免 sched.hpp <-> sem.hpp 循环包含 */

/* 计数信号量：count = 可用资源数，count=1 即为互斥锁 */
struct Semaphore {
    int  count;        /* 可用资源数 */
    TCB *wait_head;    /* 等待队列队首（FIFO，用 TCB::next 串联） */
    TCB *wait_tail;    /* 等待队列队尾（尾插 O(1)） */
};

void sem_init(Semaphore *s, int count);
void sem_wait(Semaphore *s);   /* P 操作：拿不到资源就阻塞当前任务 */
void sem_post(Semaphore *s);   /* V 操作：释放资源；有等待者则唤醒队首 */

#endif
