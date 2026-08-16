#include "kernel/demo.hpp"
#include "kernel/sem.hpp"
#include "kernel/sched.hpp"
#include "lib/printf.hpp"

/* 演示任务模块：所有教学演示集中于此。
 * 注意：printf 内部已内置互斥锁（lib/printf.cpp），这里不需要手动加锁。 */

/* ============ 睡眠-唤醒演示：线程 A/B ============ */
/* A/B 拿 CPU 打印后 task_sleep(400) 主动让出，到点由定时器中断唤醒，
 * 展示事件驱动调度（不再靠时间片硬切）。 */

void thread_a()
{
    for (;;) {
        printf("A: 线程A在跑 uptime=%u ms\n", (unsigned int)uptime_ms());
        task_sleep(400);
    }
}

void thread_b()
{
    for (;;) {
        printf("B: 线程B在跑 uptime=%u ms\n", (unsigned int)uptime_ms());
        task_sleep(400);
    }
}

/* ============ 信号量同步演示：ping-pong ============ */
/* 两个信号量初值 1/0：pinger 先动，ponger 先等。
 * 双方各 P 一个、V 另一个，被绑成严格交替——这是真正的同步，
 * 不是靠时间片轮转碰运气。 */

static Semaphore sem_ping;   /* 初值 1：pinger 第一回合直接过 */
static Semaphore sem_pong;   /* 初值 0：ponger 第一回合先阻塞 */

static void pinger()
{
    for (;;) {
        sem_wait(&sem_ping);          /* 等自己的回合 */
        printf("PING\n");
        sem_post(&sem_pong);          /* 放行对方 */
    }
}

static void ponger()
{
    for (;;) {
        sem_wait(&sem_pong);
        printf("pong\n");
        sem_post(&sem_ping);          /* 把回合还回去 */
    }
}

/* ============ 生产者-消费者（环形缓冲区） ============ */
/* 练习见 docs/prodcons-exercise.md，这里是参考答案。
 * empty/full 记录空槽/满槽数，生产者与消费者各 P 一个、V 另一个，
 * 从而保证：缓冲区满时生产者阻塞，空时消费者阻塞，不丢不重。 */

#define BUF_SIZE 8

static int  buf[BUF_SIZE];   /* 环形缓冲区 */
static int  head = 0;        /* 消费者取的位置 */
static int  tail = 0;        /* 生产者放的位置 */

static Semaphore empty;      /* 空槽数，初值 BUF_SIZE */
static Semaphore full;       /* 满槽数，初值 0 */

static void producer()
{
    int item = 0;
    for (;;) {
        item++;
        sem_wait(&empty);            /* P：等空槽（缓冲区满时阻塞） */
        buf[tail] = item;
        tail = (tail + 1) % BUF_SIZE;
        sem_post(&full);             /* V：放行消费者 */
        printf("P: 生产 %d\n", item);
        task_sleep(300);
    }
}

static void consumer()
{
    for (;;) {
        sem_wait(&full);             /* P：等满槽（缓冲区空时阻塞） */
        int item = buf[head];
        head = (head + 1) % BUF_SIZE;
        sem_post(&empty);            /* V：还给生产者一个空槽 */
        printf("C: 消费 %d\n", item);
        task_sleep(300);
    }
}

/* 创建所有演示任务（必须在 sched_init() 之后调用） */
void demo_init()
{
    sem_init(&sem_ping, 1);
    sem_init(&sem_pong, 0);
    sem_init(&empty, BUF_SIZE);
    sem_init(&full, 0);

    task_create(thread_a);
    task_create(thread_b);
    task_create(pinger);
    task_create(ponger);
    task_create(producer);
    task_create(consumer);
}
