# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

- **Cross-compiler**: `aarch64-linux-gnu-` (g++ for C++, gcc for `.S` assembly). Host gcc/g++ will not work.
- `make` / `make all` → `build/myos.elf`
- `make run` → boots in QEMU (`qemu-system-aarch64 -M virt -cpu cortex-a72 -m 128M -nographic -kernel build/myos.elf`)
- `make clean` → removes `build/` (all `.o` and the ELF)
- No test suite or linter; verification is `make && make run` and check UART output.

## Directory layout

- `arch/` — CPU layer: `boot.S` (entry), `exception.S` (vector table), `switch.S` (context switch), `exception_handler.cpp`
- `kernel/` — core: `main.cpp` (`kmain`), `sched.cpp`, `mmu.cpp`, `page_alloc.cpp`, `gic.cpp`, `timer.cpp`
- `drivers/` — devices: `uart.cpp` (PL011), `shell.cpp`
- `lib/` — utilities: `printf.cpp`, `string.cpp`
- `include/` — headers mirror the source layers (`include/kernel/`, `include/drivers/`, ...); `types.hpp` and `arch/sysreg.hpp` are shared at their layer root
- `build/` — compile artifacts (`.o` and ELF), git-ignored, mirrors source subdirectories

## Architecture

This is a **bare-metal aarch64 OS kernel** — no libc, no standard headers, no C++ runtime. Compiled with `-ffreestanding -nostdlib -fno-exceptions -fno-rtti -fno-stack-protector -mgeneral-regs-only`.

- **Entry**: `_start` in `arch/boot.S` (section `.text.boot`, placed first by `linker.ld`) → zeroes BSS → sets up stack (`__stack_top` at `0x40080000 + 16KB`) → sets VBAR_EL1 to `vector_table` → calls `kmain`.
- **Kernel load address**: `0x40080000` (defined in `linker.ld`). Stack size: 16KB.
- **UART**: PL011 at MMIO base `0x09000000`, driven by `drivers/uart.cpp` via volatile pointers. `uart_putc` prepends `\r` before `\n`, polls TXFF (bit 5 of UARTFR) before writing.
- **printf**: `lib/printf.cpp` provides a custom `printf()` (declared in `include/lib/printf.hpp`) supporting `%d %i %u %x %X %c %s %p %`. Uses `__builtin_va_list` for variadic args. Output goes through `uart_putc`.
- **Exception handling**: `arch/exception.S` defines a `vector_table` (`.section .text.vectors`, 2KB-aligned, 16 entries of 128 bytes each branching to `exception_common`). The common handler saves all GP registers + ELR_EL1, ESR_EL1, SPSR_EL1 onto the stack, then calls `exception_handler` (C++ in `arch/exception_handler.cpp`) which prints the exception class, ISS, and ELR before halting.
- **Shutdown**: `kmain` issues PSCI `SYSTEM_OFF` (`w0=0x84000008`, `hvc #0`) with a `wfi` fallback loop. QEMU powering off and exiting is normal behavior.

## Types

Custom fixed-width types in `include/types.hpp` (no `<cstdint>`): `u8`, `u32`, `u64`. `include/lib/printf.hpp` adds `s64` (`long long`). Headers chain-include via `#include "types.hpp"`; this is intentional — do not pull in standard headers.

## Gotchas

- The `.vscode` configs reference host gcc/g++ and a `build/Debug` path that don't match this cross-build — do not rely on them.
- Build artifacts live in `build/` (git-ignored via `.gitignore`); run `make clean` before building after edits.
- The vector table is placed at a 4KB-aligned boundary inside `.text` (the linker script aligns after `.text.boot`). VBAR_EL1 requires 2KB alignment minimum, so this satisfies the hardware constraint.
