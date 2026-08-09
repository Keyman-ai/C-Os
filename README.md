# myos — 从零手写 aarch64 裸机操作系统

一个用于学习操作系统原理的裸机内核：**无 libc、无标准头文件、无 C++ 运行时**，所有代码手写，跑在 QEMU 的 `virt` 机器上（`qemu-system-aarch64 -M virt`）。

## 特性

- **UART 串口驱动**：PL011 驱动 + 自定义 `printf`（`%d %i %u %x %X %c %s %p`）
- **异常处理**：完整的 16 项异常向量表，保存全部寄存器现场，打印异常类型后停机
- **GICv3 + 定时器**：中断控制器初始化，ARM 通用定时器驱动
- **多任务**：内核线程 + 时间片轮转 + 定时器抢占（200ms/片，三线程：A、B、Shell）
- **交互式 Shell**：作为第三个内核线程参与调度，非阻塞读取 + 主动交权
- **内存管理**：MMU 页表初始化 + 简单页分配器（LIFO，4KB/页）
- **关机**：Ctrl+C（字节 `0x03`）触发 PSCI `SYSTEM_OFF`，QEMU 干净退出

## 构建与运行

需要 `aarch64-linux-gnu-` 交叉编译工具链（主机 gcc/g++ 不行）。

```bash
make            # 编译 → build/myos.elf
make run        # 启动 QEMU（-M virt,gic-version=3 -cpu cortex-a72 -m 128M -nographic）
make clean      # 删除 build/ 下所有产物
```

或者直接跑脚本：

```bash
bash run.sh
```

启动后你会看到 A/B 两个线程交替打印、定时器 `tick` 和 shell 提示符 `> `。输入 `help` 查看可用命令。

## Shell 命令

| 命令 | 作用 |
|------|------|
| `help` | 列出可用命令 |
| `alloc` | 从页分配器申请一页，打印地址 |
| `echo <文本>` | 回显文本 |
| `clear` | 清屏（输出几个空行） |
| `Ctrl+C` | 关机（PSCI SYSTEM_OFF） |

> Shell 是第三个内核线程，每 600ms（3 线程 × 200ms 时间片）轮转一次，输入会在它的时间片内被处理。

## 目录结构

```
arch/      CPU 架构层：启动(boot.S)、异常向量表(exception.S)、上下文切换(switch.S)、异常处理
kernel/    内核核心：入口(main.cpp)、调度(sched)、MMU、页分配器、GIC、定时器
drivers/   设备驱动：UART(PL011)、Shell
lib/       通用库：printf、string（手写，无 libc）
include/   头文件，镜像源码分层（include/kernel/、include/drivers/ ...），types.hpp 为全局基类型
build/     编译产物（.o 与 myos.elf），git 忽略
docs/      设计文档
```

## 内存布局

- 内核加载地址：`0x40080000`（`linker.ld` 定义），128MB 物理内存
- 内核栈：16KB，位于 BSS 之后（`__stack_top`）
- 每个内核线程拥有独立的 4KB 栈（来自页分配器）

## 文档

- `docs/multitasking.md` — 多任务实现完整记录：TCB 结构、`switch_to` 上下文切换、假现场、时间片抢占，以及踩过的 4 个真实 bug（假现场尺寸、`str sp` 编码约束、EOI 顺序、PSTATE.I 继承）

## 相关说明

- 这是教学项目，所有注释为中文
- 验证方式：`make && make run`，检查 UART 输出（无测试套件）
- `disasm-guide.md` / `disasm.txt` 是调试反汇编的排查笔记，不属于工程代码
