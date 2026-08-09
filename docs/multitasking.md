# 多任务上下文切换实现与排障

> myos 从"单线程裸机"到"多任务内核线程 + 时间片轮转 + 定时器抢占"的完整实现记录。
> 代码全部手写：无 libc、无 C++ 运行时，跑在 aarch64 QEMU（`qemu-system-aarch64 -M virt`）上。
> 所有代码注释为中文，与仓库代码一致。

---

## 1. 概述与术语澄清

**我们实现的是内核线程（kernel thread）**：同一个地址空间（同一套页表）里的多个执行流。切换时只需保存/恢复 CPU 寄存器上下文。

注意区分**进程**：真正的进程切换还要额外切换地址空间（TTBR/页表），每个进程有独立的虚拟内存视图。本文不涉及。

### 核心概念：三句话

1. **线程 = 一个函数 + 一块独立的内核栈**。函数是线程入口；栈从页分配器 `alloc_page()` 拿 4KB，各线程互不干扰。
2. **切换 = 换栈 + 换 PC**。sp 指向谁的栈，CPU 就活在谁的"世界"里——栈上存着调用链、局部变量、现场。换 PC 靠 `ret` 跳到目标线程的 x30。
3. **借尸还魂**：`switch_to(prev, next)` 保存 prev 的现场、加载 next 的现场，最后 `ret` 跳进 next 的世界。

### 为什么只保存 callee-saved 寄存器？

按 AAPCS64 调用约定，x19~x30（含 x29、x30）是函数自己的保命责任（callee-saved）；x0~x18 是调用方责任（caller-saved）。`switch_to` 是被当普通函数调用的，所以只保存 x19~x30 就尽到了义务——调用链上每层 C 函数已经替自己管好了 caller-saved。

---

## 2. 数据结构：TCB

```c
// include/kernel/sched.hpp
struct TCB {
    u64 sp;          /* 现场栈指针：不运行时的现场存在哪（核心字段） */
    u64 stack;       /* 该线程的独立内核栈（页分配器给的 4KB） */
    u32 id;          /* 线程编号（同时是任务数组下标） */
    void (*entry)(); /* 线程入口函数：被切进来后从哪开始跑 */
};
```

`sp` 是灵魂字段：线程被切走时，`switch_to` 把当时的栈指针存这里；切回时从这里恢复。

调度器全局状态（`kernel/sched.cpp`）：

```c
static TCB tasks[MAX_TASKS];  /* 任务表：最多 8 个线程 */
static int  task_count;       /* 已创建的线程数 */
static TCB *current;          /* 当前正在运行的线程（调度器维护） */
static TCB boot;              /* 停车场：存 kmain 的现场，不参与轮转 */
```

`boot` 的用途见 4.4。

---

## 3. 各部分实现

### 3.1 task_create：布置假现场

```c
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
    return task_count++;
}
```

**假现场（fake frame）**：新线程从未运行过，没有"上次被切走"的现场。`switch_to` 的恢复端无条件弹 6 组（96 字节），所以 task_create 预置一份 96 字节的假现场，其中只有 x29=0、x30=trampoline 是真实有效的，其余 5 组（x19~x28）是垃圾值——按 AAPCS64，被调用方永远不会读 callee-saved 的"进来时"的值，所以无害。

**为什么 `frame[0]`/`frame[1]` 是 x29/x30 而不是 x19/x20？** 因为栈向下生长，`switch_to` 压栈时 x29/x30 最后压、反而在最低地址；恢复时从最低地址往上弹，第一组 `ldp` 读到的就是 x29/x30。假现场必须和恢复端的布局完全一致（见排障 5.1）。

### 3.2 switch_to：上下文切换（汇编）

```asm
// arch/switch.S
.section .text
.global switch_to

/* void switch_to(struct TCB *prev, struct TCB *next)
 * 进入时: x0 = prev, x1 = next
 */
switch_to:
    /* 1. 存 prev 的现场：callee-saved 寄存器 x19-x30 压栈，再存 sp */
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x28, [sp, #-16]!
    stp x29, x30, [sp, #-16]!
    mov x9, sp                /* 先把 sp 挪到临时寄存器 */
    str x9, [x0, #0]          /* prev->sp = sp */

    /* 2. 换栈：把 next 的现场倒序弹回 */
    ldr x9, [x1, #0]          /* x9 = next->sp */
    mov sp, x9
    ldp x29, x30, [sp], #16
    ldp x27, x28, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16

    ret                        /* 借尸还魂：跳到 x30 */
```

要点：

- **存现场**：6 组 `stp` 用 pre-index `[sp, #-16]!`（先减 16 再存，写回 sp），把 x19~x30 全部压入 prev 的栈。
- **记现场**：`str` 把 sp 存进 `prev->sp`。注意不能 `str sp, [...]` 直接存——AArch64 里 STR 的数据寄存器字段取 31 代表 XZR 不是 SP（见排障 5.2），必须先 `mov x9, sp`。
- **换栈**：`ldr x9, [x1, #0]` 取 `next->sp`，`mov sp, x9` 完成换栈。
- **弹现场**：6 组 `ldp` 用 post-index `[sp], #16`（先读后加），顺序和压栈相反——x29/x30 最先弹（它们在最低地址）。
- **`ret` 跳 x30**：`ret` ≡ `PC = X30`，是间接跳转。老线程的 x30 = 上次 `bl switch_to` 后的返回地址，`ret` 让它回到 schedule 继续跑；新线程的 x30 = trampoline，`ret` 让它第一次踏进 trampoline。

### 3.3 thread_trampoline：新线程的出生地

```c
extern "C" void thread_trampoline()
{
    __asm__ __volatile__("msr DAIFClr, #2");  /* 新线程第一次进来，解除 IRQ 屏蔽 */
    current->entry();                         /* 调用这个线程真正的入口函数 */
    for (;;) __asm__ __volatile__("wfi");     /* 入口返回后死循环兜底，绝不掉进垃圾地址 */
}
```

`msr DAIFClr, #2` 是**排障 5.4** 的产物：新线程第一次进入不经过 `eret`，会继承异常入口自动置上的中断屏蔽（PSTATE.I=1），必须手动解除。

### 3.4 schedule()：轮转调度

```c
void schedule()
{
    if (task_count == 0)
        return;                            /* 还没有线程，没得切 */

    TCB *prev = current;
    TCB *next;

    if (prev == nullptr)
        next = &tasks[0];                  /* 第一次调度：从 0 号线程开始 */
    else
        next = &tasks[(prev->id + 1) % task_count];  /* 轮转：id+1 取模 */

    if (prev == next)
        return;                            /* 只有自己，切了也白切 */

    current = next;                        /* 先改 current（trampoline 要读它） */
    switch_to(prev ? prev : &boot, next);  /* 有 prev 存 prev，否则存 boot */
}
```

三个设计点：

- **`% task_count` 就是轮转的"圈"**：id 从 0 转到最后一个后取模回 0。
- **`current = next` 必须在 `switch_to` 之前**：新线程第一次跑时，trampoline 执行 `current->entry()`，读到的必须是自己。
- **第一次切换没有 prev**：`current` 还是 nullptr，但 `switch_to` 的 x0 必须是个合法 TCB。用一个静态 `boot` 当"停车场"，第一次切时把 kmain 的现场存进 boot。**boot 不进任务数组、永远不被选为 next**——kmain 交权后永不回来。

### 3.5 定时器抢占：在 irq_handler 里换人

```c
// kernel/timer.cpp
extern "C" void irq_handler()
{
    u64 iar = READ_SYSREG(ICC_IAR1_EL1);
    u64 id  = iar & 0x3FF;

    WRITE_SYSREG(ICC_EOIR1_EL1, iar);   /* 先 EOI：立刻释放 GIC 优先级（见排障 5.3） */

    if (id == 30) {
        u64 freq = READ_SYSREG(CNTFRQ_EL0);
        WRITE_SYSREG(CNTP_CVAL_EL0, READ_SYSREG(CNTPCT_EL0) + freq);
        printf("tick\n");
        schedule();                     /* 时间片到：抢占，切换下一个线程 */
    }
}
```

**抢占**：每个定时器 tick（1 秒）就是"当前线程的时间片用完了"，`irq_handler` 里调 `schedule()` 强制换人。线程自己完全无感——它被切回时从 `schedule()` 返回、`irq_handler` 结束、`irq_common` 恢复 272 字节异常帧、`eret`，继续跑被中断前的指令。

### 3.6 main.cpp：接线

```c
/* 线程 A：打印 + 忙等模拟干活 */
void thread_a()
{
    for (;;) {
        printf("A: 线程A在跑\n");
        for (volatile int i = 0; i < 1000000; i++);
    }
}

/* 线程 B：打印 + 忙等模拟干活 */
void thread_b()
{
    for (;;) {
        printf("B: 线程B在跑\n");
        for (volatile int i = 0; i < 1000000; i++);
    }
}

extern "C" void kmain(void) {
    /* ...原有初始化：uart / mmu / page_alloc / gic / timer... */

    sched_init();            /* 清空任务表 */
    task_create(thread_a);   /* 创建线程 A */
    task_create(thread_b);   /* 创建线程 B */
    for (;;) schedule();     /* kmain 交权：第一次调度把自己存进 boot，永不返回 */
}
```

注意 `volatile` 忙等：`-O2` 下普通空循环会被编译器整个删掉（它"证明"循环没用），`volatile` 强制真实读写，循环才存在——模拟"线程干活"。

---

## 4. 完整运行流程（一次 A→B 抢占的旅程）

```
kmain: for(;;) schedule()
  └─ 第一次：current==nullptr → next=tasks[0]，switch_to(&boot, tasks[0])
       ├─ 把 kmain 现场存进 boot（kmain 从此"退隐"）
       └─ 加载 tasks[0] 的假现场 → ret → trampoline
            └─ DAIFClr, #2（解开 IRQ 屏蔽）
                 └─ thread_a() 开始刷 "A: 线程A在跑"……

1 秒后：定时器中断
  ├─ 硬件：PSTATE.I=1（自动屏蔽），PC→VBAR+0x280
  ├─ irq_common：在 A 的栈上压 272 字节异常帧
  ├─ irq_handler：读 IAR(id=30) → EOI（释放优先级）→ 重装定时器 → "tick" → schedule()
  │    └─ schedule：prev=A, next=B, current=B, switch_to(A, B)
  │         ├─ A 的 callee-saved 压进 A 的栈，A->sp 记下
  │         └─ 加载 B 的假现场 → ret → trampoline → DAIFClr → thread_b() 开始刷 B……

1 秒后：又一个定时器中断（此时 B 在跑）
  ├─ 同样流程，schedule：prev=B, next=A, switch_to(B, A)
  │    ├─ B 的现场压进 B 的栈
  │    └─ 加载 A 的现场（96 字节）→ ret → 回到 A 上次的 schedule() 调用里
  │         └─ irq_handler 结束 → irq_common 恢复 272 字节帧 → eret
  │              └─ A 继续刷 "A: 线程A在跑"……（A 完全无感被切走过）
```

**两层现场**：A 被打断时，完整寄存器（272 字节异常帧）在 irq_common 里；切走时，调用链状态（96 字节 callee-saved）在 switch_to 里。两层都在 A 自己的栈上，合起来让抢占完全透明。

---

## 5. 遇到的问题与解决方案

这一路踩了 4 个真实 bug，每个都值得单独记录。

### 5.1 假现场只有 16 字节，恢复端弹 96 字节 → sp 越界崩溃

**现象**：`task_create` 初始写法把假现场布置在 `stack + 4096 - 16`（只有 `[x29, x30]` 16 字节），但 `switch_to` 恢复端无条件弹 6 组 = 96 字节。新线程第一次被切进来时，sp 一路弹出页面顶端、越界 80 字节。

**为什么必然崩**：第一个线程拿的是页分配器最高地址的页（`0x47fff000`，LIFO 分配），越界直接踩到 `0x48000000+`——超出 128MB 物理 RAM 末尾（QEMU virt 上是个空洞），一读就是 data abort。

**解决**：把假现场起点改为 `stack + 4096 - 96`，占满 96 字节。弹完 6 组后 sp 正好停在页面顶端（`stack + 4096`），一个字节不越界。`frame[2..11]`（x19~x28 位置）是垃圾值也没关系——callee-saved 的值永远不会被"进来时"读取。

**教训**：假现场必须和 `switch_to` 恢复端的**尺寸和布局完全一致**——恢复端弹多少，假现场就要铺多少。

### 5.2 `str sp, [x0, #0]` 汇编错误：SP 不能当数据寄存器

**现象**：编译报 `Error: expected ZA array at operand 1 -- str sp,[x0,#0]`（汇编器把 `str sp` 误解析成 SME 的 ZA 指令，报错信息很误导）。

**根因**（AArch64 架构约束）：64 位 `str` 指令的数据寄存器字段 Rt 取 31 时，编码的是 **XZR（零寄存器），不是 SP**。SP 只允许出现在**地址基址**位置（方括号里），永远不能当"被存/被取的数据"。所以 `str sp, [...]` 这条指令在架构上不存在。

**解决**：先挪到普通寄存器再存：

```asm
    mov x9, sp
    str x9, [x0, #0]
```

**教训**：回看所有指令，sp 只出现在 `mov sp, x9`、`[sp, #-16]!` 这类"地址"位置——这就是编码约束的外在表现。

### 5.3 EOI 晚于 schedule() → GIC 优先级被按住，定时器从此全哑

**现象**：第一次运行，A 正常跑了一秒 → 第一个 tick 切到 B → **之后定时器再也不响**，B 独占 CPU。12 秒只有 1 个 tick。

**根因**：最初 `irq_handler` 把 `WRITE_SYSREG(ICC_EOIR1_EL1, iar)` 写在 `schedule()` **之后**。第一次抢占（A→B）切到的是**新线程** B——B 走 trampoline 路线，**永远不会回到 irq_handler 的 EOI 那行**。于是中断 30 的 EOI 永远没写，它在 GIC 里保持 **ACTIVE**，其优先级被按住（running priority）。

下一个 tick 到来时，新中断的优先级和正在被按住的一样——而 **GIC 只会上报"优先级更高"的中断，相同的优先级一律吞掉**。于是第 2、3、4……秒的 tick 全在 GIC 里被静默丢弃。

**解决**：把 EOI 挪到 `irq_handler` 最前面，紧跟读 IAR 之后，**任何可能切走的调用之前**释放优先级：

```c
    u64 iar = READ_SYSREG(ICC_IAR1_EL1);
    u64 id  = iar & 0x3FF;
    WRITE_SYSREG(ICC_EOIR1_EL1, iar);   /* 先 EOI：立刻释放 GIC 优先级 */
```

提前 EOI 安全的原因：重装定时器后中断线已拉低，且异常处理期间 PSTATE.I=1（不会嵌套），没有伪中断风险。

### 5.4 新线程继承 PSTATE.I=1 → 中断屏蔽进不来

**现象**：修好 5.3 后重跑，**依然只有 1 个 tick**。EOI 修复是必要的，但不是全部。

**根因**：**任何异常（包括 IRQ）进入时，硬件自动把 PSTATE.I 置 1**（防嵌套）。这个屏蔽靠谁解开？靠 `eret`——它从 SPSR_EL1 恢复 PSTATE，把 I 还原成异常发生前的 0。

两条恢复路线对比：

- **老线程被切回**：`switch_to 的 ret` → schedule → irq_handler → irq_common → **`eret`** → I 还原为 0。✓
- **新线程第一次进入**：假现场 → `ret` → **trampoline**。这条路上**没有 eret**！所以 trampoline 开始执行时，PSTATE.I 还是异常入口置的 **1**——IRQ 被屏蔽着，定时器中断再也进不来。

这也解释了为什么 kmain→A（第一次切换）没问题：kmain 在正常上下文里调 schedule，当时 I=0，A 继承 I=0；而 A→B 发生在异常上下文里，B 就继承了 I=1。

**解决**：trampoline 一进来先手动解屏蔽（和 boot.S 里 `msr DAIFClr, #2` 同一句）：

```c
    __asm__ __volatile__("msr DAIFClr, #2");  /* 新线程第一次进来，解除 IRQ 屏蔽 */
```

### 5.3 与 5.4：两道独立的大门

5.3 和 5.4 各堵一路死锁，**缺一不可**：

| 修哪个 | 不修的后果 |
|--------|-----------|
| 只修 EOI（5.3） | 新线程带着 I=1 跑，中断送达 CPU 也收不下 |
| 只修 I 屏蔽（5.4） | 中断未 EOI、优先级被按住，GIC 根本不送达 |

**通用规则**：任何可能切走的可抢占中断处理路径，必须满足两条——① 在切换前 EOI，释放 GIC 优先级；② 新线程的入口路径重新使能 IRQ。

---

## 6. 验证结果

`make clean && make` 编译零错误（仅一条无害的 `.note.gnu.build-id` 链接警告）；QEMU 跑 12 秒：

| 指标 | 结果 |
|------|------|
| tick 数 | **11 个**（约每秒 1 个，第一个在启动后约 1 秒） |
| 线程 A | 429 行 `A: 线程A在跑` |
| 线程 B | 414 行 `B: 线程B在跑` |
| 交替节奏 | 每个 tick 精确换人：tick1 A→B、tick2 B→A、tick6 B→A…… |

A/B 时间片近乎对半，抢占稳定运行。

---

## 7. 涉及文件

| 文件 | 内容 |
|------|------|
| `include/kernel/sched.hpp` | TCB 结构、sched_init/task_create/schedule 声明、`extern "C" switch_to` |
| `kernel/sched.cpp` | 任务表、trampoline、sched_init、task_create（假现场）、schedule（轮转） |
| `arch/switch.S` | `switch_to`：保存/恢复 callee-saved + sp，`ret` 借尸还魂 |
| `kernel/timer.cpp` | `irq_handler`：先 EOI、重装定时器、`schedule()` 抢占 |
| `kernel/main.cpp` | 线程 A/B、sched_init + task_create + 交权 |
| `Makefile` | 加入 `sched.cpp` / `switch.S` |
