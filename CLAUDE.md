# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

- **Cross-compiler**: `aarch64-linux-gnu-` (g++ for C++, gcc for `.S` assembly). Host gcc/g++ will not work.
- `make` / `make all` → `myos.elf`
- `make run` → boots in QEMU (`qemu-system-aarch64 -M virt -cpu cortex-a72 -m 128M -nographic -kernel myos.elf`)
- `make clean` → removes `src/*.o` and `myos.elf`
- No test suite or linter; verification is `make && make run` and check UART output.

## Architecture

This is a **bare-metal aarch64 OS kernel** — no libc, no standard headers, no C++ runtime. Compiled with `-ffreestanding -nostdlib -fno-exceptions -fno-rtti -fno-stack-protector -mgeneral-regs-only`.

- **Entry**: `_start` in `src/boot.S` (section `.text.boot`, placed first by `linker.ld`) → zeroes BSS → sets up stack (`__stack_top` at `0x40080000 + 16KB`) → sets VBAR_EL1 to `vector_table` → calls `kmain`.
- **Kernel load address**: `0x40080000` (defined in `linker.ld`). Stack size: 16KB.
- **UART**: PL011 at MMIO base `0x09000000`, driven by `src/uart.cpp` via volatile pointers. `uart_putc` prepends `\r` before `\n`, polls TXFF (bit 5 of UARTFR) before writing.
- **printf**: `src/printf.cpp` provides a custom `printf()` (declared in `include/printf.hpp`) supporting `%d %i %u %x %X %c %s %p %`. Uses `__builtin_va_list` for variadic args. Output goes through `uart_putc`.
- **Exception handling**: `src/exception.S` defines a `vector_table` (`.section .text.vectors`, 2KB-aligned, 16 entries of 128 bytes each branching to `exception_common`). The common handler saves all GP registers + ELR_EL1, ESR_EL1, SPSR_EL1 onto the stack, then calls `exception_handler` (C++ in `src/exception.cpp`) which prints the exception class, ISS, and ELR before halting.
- **Shutdown**: `kmain` issues PSCI `SYSTEM_OFF` (`w0=0x84000008`, `hvc #0`) with a `wfi` fallback loop. QEMU powering off and exiting is normal behavior.

## Types

Custom fixed-width types in `include/uart.hpp` (no `<cstdint>`): `u8`, `u32`, `u64`. `include/printf.hpp` adds `s64` (`long long`). Headers chain-include via `#include "uart.hpp"`; this is intentional — do not pull in standard headers.

## Gotchas

- The `.vscode` configs reference host gcc/g++ and a `build/Debug` path that don't match this cross-build — do not rely on them.
- Stale `.o` files and `myos.elf` are git-tracked; run `make clean` before building after edits.
- The vector table is placed at a 4KB-aligned boundary inside `.text` (the linker script aligns after `.text.boot`). VBAR_EL1 requires 2KB alignment minimum, so this satisfies the hardware constraint.
