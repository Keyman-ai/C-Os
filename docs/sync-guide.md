# 同步原语与睡眠/唤醒调度 — 开发指导

> 本文档是「信号量 + 睡眠/唤醒调度」功能的开发脚手架，供动手实现时对照。
> 代码由你（学习者）自己写，本文档只提供设计思路、接口规格与易错点。

## 1. 目标

把当前"纯时间片轮转"的调度器升级为**事件驱动**：

- 任务有状态：`RUNNING / READY / BLOCKED`
- 任务可以主动睡眠 `task_sleep(ms)`，到点由定时器中断唤醒
- 提供计数信号量 `sem_init / sem_wait / sem_post`，等待者阻塞、释放者唤醒
- **printf 内部内置互斥锁**（count=1 信号量，见 lib/printf.cpp），所有打印天然
  互斥、不交错，调用者无需手动加锁；异常/中断路径用裸 UART（arch/exception_handler.cpp）
- idle 任务兜底，调度器永远有任务可切

## 2. 设计总览

```
TCB 增加：state、sleep_until(CNTPCT 绝对时间)、next(等待队列节点)
就绪队列：不新建结构——扫描 tasks[] 中 state==READY 的任务即可
idle 任务：永远 READY，for(;;) wfi，调度器兜底
task_sleep(ms)：记 sleep_until → state=BLOCKED → schedule() 主动交权
timer 中断：tick_update() = 累计毫秒 + 唤醒睡到点的任务，然后照旧 schedule() 抢占
信号量：FIFO 等待队列，关中断保原子
```

## 3. 接口规格

### include/kernel/sched.hpp（扩展）

```cpp
enum TaskState { TASK_RUNNING, TASK_READY, TASK_BLOCKED };

struct TCB {
    u64 sp;              /* 已有 */
    u64 stack;           /* 已有 */
    u32 id;              /* 已有 */
    u32 state;           /* 新增 */
    u64 sleep_until;     /* 新增：0 = 未睡眠 */
    void (*entry)();     /* 已有 */
    TCB *next;           /* 新增：等待队列节点 */
};

void task_sleep(u64 ms);        /* 新增 */
u64  uptime_ms();               /* 新增 */
extern "C" void tick_update();  /* 新增：timer 中断回调 */
TCB *sched_current();           /* 新增：返回当前任务，sem.cpp 要用 */
```

### include/kernel/sem.hpp（新建）

```cpp
struct TCB;                     /* 前置声明 */

struct Semaphore {
    int  count;
    TCB *wait_head;             /* FIFO 队首 */
    TCB *wait_tail;             /* FIFO 队尾 */
};
void sem_init(Semaphore *s, int count);
void sem_wait(Semaphore *s);    /* P：拿不到就阻塞 */
void sem_post(Semaphore *s);    /* V：释放/唤醒 */
```

## 4. 关键易错点（实测/推理结论）

### 4.1 DAIF 位图（本项目实测）
`msr DAIFSet/DAIFClr, #2` 操作的是 **IRQ 屏蔽位（I）**，与 `boot.S`/trampoline
现有写法一致。实测值：初始 `DAIF=0x340`（I=0 开着的）；`DAIFSet #2` 后 `0x3c0`
（I=1 屏蔽）。
- 关中断：`__asm__ __volatile__("msr DAIFSet, #2");`
- 开中断：`__asm__ __volatile__("msr DAIFClr, #2");`

### 4.2 丢失唤醒（lost wakeup）
`sem_wait` 的「检查 count → 挂队列 → 置 BLOCKED」三步必须**关中断原子完成**，
否则 post 可能先执行（唤醒了一个还没挂进队列的任务），然后你才阻塞 → 永久睡死。
`sem_post` 的「取队首 / ++count」同样需要关中断。

### 4.3 PSTATE.I 继承
`switch_to`（arch/switch.S）**不保存/恢复 DAIF**。所以：
**关中断只保护临界区，`schedule()` 必须在开中断之后调用**。
否则切过去的任务带着"中断屏蔽"跑，整机失去抢占，症状像死机。

### 4.4 唤醒判断抗回绕
`now >= sleep_until` 用无符号比较在回绕时会错，用带符号差：
`(long long)(now - sleep_until) >= 0`

### 4.5 task_sleep 的写入顺序
**先写 `sleep_until`，再写 `state = BLOCKED`**。反了的话，中断窗口里
tick_update 会用旧的 sleep_until 误判（可能提前唤醒或漏唤醒）。
（先写 sleep_until 的窗口是无害的：state 还没变 BLOCKED，唤醒逻辑不会碰它。）

### 4.6 信号量语义：资源移交
- `sem_post`：**有人等待 → 直接把资源移交给队首任务**（count 不变，唤醒它）；
  无人等待 → count++。
- `sem_wait` 返回即视为"已拿到资源"（count 已在 wait 时减过，或被 post 移交）。

### 4.7 schedule() 状态机要点
- 当前任务若还是 RUNNING（被抢占/主动交权）→ 先置 READY
- 从 `prev->id + 1` 开始绕一圈扫描，找第一个 READY 的任务
- `prev == next`（就绪的只有自己）→ 只更新 state 为 RUNNING，原地继续，不切换
- BLOCKED 的任务不会被选中，也不会被误置回 READY
- idle 永远 READY，所以扫描必有结果

## 5. 实现顺序（每步可独立编译验证）

1. **头文件**：sched.hpp 扩展 + sem.hpp 新建
2. **调度器**：sched.cpp — 状态机轮转 + idle 任务 + task_sleep + tick_update
   - 注意：idle 任务在 `sched_init()` 里创建（id=0），`for(;;) wfi`
   - tick 间隔固定 200ms（与 timer.cpp 的 freq/5 一致），`ticks_ms += 200`
3. **信号量**：kernel/sem.cpp 新建
4. **timer**：timer.cpp — `irq_handler` 删掉 `printf("tick")`，改调 `tick_update()`
   （中断上下文不再碰 UART，从根源消除打印竞态）
5. **演示**：main.cpp — 全局互斥锁（count=1）保护 printf；A/B 拿锁打印 + `task_sleep(400)`；
   pinger/ponger 用两个信号量（初值 1/0）严格交替打印 PING/pong
6. **Makefile**：`CXX_SOURCES` 加 `kernel/sem.cpp`

## 6. Review 检查清单

- [ ] task_sleep 先写 sleep_until 再写 state=BLOCKED
- [ ] sem_wait 临界区整段关中断，schedule() 在开中断后
- [ ] 唤醒比较 `(long long)(now - sleep_until) >= 0`
- [ ] sem_post 有人等待时不加 count（移交语义）
- [ ] schedule()：RUNNING→READY 再找 next；prev==next 原地继续
- [ ] 任务数 ≤ MAX_TASKS(8)：idle+A+B+pinger+ponger+shell = 6
- [ ] sched_current() 对 null 防御（kmain 早期可能无 current）

## 7. 验证方法

```bash
make && timeout 8 qemu-system-aarch64 -M virt,gic-version=3 -cpu cortex-a72 \
    -m 128M -nographic -kernel build/myos.elf </dev/null
```

期望输出特征：
- 无 `tick` 打印（已移除），改为 uptime 计数可见
- A/B 交替输出，`uptime` 递增，节奏受 task_sleep(400) 控制
- PING / pong 严格交替（信号量同步，不靠时间片）
- 打印行之间不交错、不乱码（互斥锁生效）
- shell 提示符 `> ` 可用，help/echo 等命令正常
- 按 Ctrl+C（字节 0x03）干净关机
