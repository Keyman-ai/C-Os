# 练习：生产者-消费者（PV 填空）

> 目标：用两个信号量 + 一个环形缓冲区，让 producer 和 consumer 两个任务
> 严格配合——缓冲区**不空取、不溢出、不丢数据**。
> 框架代码已给全，**4 个 TODO 由你填**，填完编译运行看效果。
> **参考答案在 `kernel/demo.cpp`（生产者-消费者部分），先自己填，填完再对照。**

## 1. 你要看到的现象

```
P: 生产 1
C: 消费 1
P: 生产 2
C: 消费 2
P: 生产 3
C: 消费 3
...
```

生产者和消费者**节奏一致**（都 `task_sleep(300)`），但顺序必须"先产后消"——靠的就是信号量，而不是运气。

## 2. 关键设计（先想清楚再填）

- **`empty`**：空槽数，初值 `BUF_SIZE`（8）。生产者每放一个，空槽 -1。
- **`full`**：满槽数，初值 `0`。消费者每取一个，满槽 -1。
- 两个信号量**初值不同**，这正是它们的"初始状态"：
  - 开始时缓冲区是空的 → 消费者必须先等（`P(full)` 会阻塞）
  - 生产者有 8 个空槽 → 前 8 次 `P(empty)` 直接通过
- **P/V 必须成对**：生产者 `P(empty)` … `V(full)`；消费者 `P(full)` … `V(empty)`。
  生产者 P 的东西，由消费者 V 回来（反之亦然）——这就是任务间的"暗号"。

## 3. 框架代码（填空）

```cpp
#include "kernel/sem.hpp"
#include "kernel/sched.hpp"
#include "lib/printf.hpp"

#define BUF_SIZE 8

/* 环形缓冲区：head = 消费者取的位置，tail = 生产者放的位置 */
static int  buf[BUF_SIZE];
static int  head = 0;
static int  tail = 0;

static Semaphore empty;      /* 空槽数 */
static Semaphore full;       /* 满槽数 */

static void producer()
{
    int item = 0;
    for (;;) {
        item++;
        /* TODO 1: 等一个空槽（P empty） */
        sem_wait(&empty);            /* P：等空槽（缓冲区满时阻塞） */
        buf[tail] = item;
        tail = (tail + 1) % BUF_SIZE;

        /* TODO 2: 放行消费者（V full） */
        sem_post(&full);             /* V：放行消费者 */
        printf("P: 生产 %d\n", item);
        task_sleep(300);
    }
}

static void consumer()
{
    for (;;) {
        /* TODO 3: 等一个满槽（P full） */
        sem_wait(&full);             /* P：等满槽（缓冲区空时阻塞） */
        int item = buf[head];
        head = (head + 1) % BUF_SIZE;

        /* TODO 4: 还给生产者一个空槽（V empty） */
        sem_post(&empty);            /* V：还给生产者一个空槽 */
        printf("C: 消费 %d\n", item);
        task_sleep(300);
    }
}
```

在 `kmain` 里挂载（这个不用填，照抄）：

```cpp
sem_init(&empty, BUF_SIZE);   /* 8 个空槽 */
sem_init(&full, 0);           /* 0 个满槽 */
task_create(producer);
task_create(consumer);
```

## 4. 自查清单（填完逐条过）

- [ ] 两个信号量初值对吗？（empty=8, full=0）
- [ ] 生产者先 `P(empty)`，**放完之后**才 `V(full)` —— 顺序反了会怎样？想想
- [ ] 消费者先 `P(full)`，**取完之后**才 `V(empty)`
- [ ] 用恒等式心算：`empty + full` 恒等于 8？（各减各加，应该守恒）
- [ ] 有没有可能出现"空取"（缓冲区空时 consumer 读了 buf）或"溢出"（缓冲区满时 producer 还写）？

## 5. 验证

```bash
make && timeout 10 qemu-system-aarch64 -M virt,gic-version=3 -cpu cortex-a72 \
    -m 128M -nographic -kernel build/myos.elf </dev/null
```

期望：
- 每个 `P:` 后紧跟对应的 `C:`（同号，先产后消）
- 永远不出现"C: 消费"在"P: 生产"之前
- 不卡死（如果卡死，大概率是 P/V 配对错或初值错）

## 6. 扩展思考（选做）

- 把 `task_sleep(300)` 从 consumer 删掉，让 consumer 疯狂消费——验证它**抢不到**还没生产的 item（`P(full)` 会拦住它）
- 把 `task_sleep` 全删掉，生产者高速生产——验证缓冲区满时 producer 被 `P(empty)` 拦住
- 如果再加一个生产者（两个生产者共享缓冲区），当前代码会出什么问题？需要加什么？（提示：tail 的读改写需要互斥）

---

填完把代码贴给我 review，或者自己编译跑，有问题随时问。答案不唯一，能通过恒等式和运行验证的就是好答案。
