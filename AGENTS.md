# AGENTS.md

Compact guidance for working in this bare-metal aarch64 OS project.

## Toolchain & build

- Cross-compile with `aarch64-linux-gnu-` (g++ / ld / gcc for asm). Host gcc/g++ will NOT work.
- Freestanding C++: `-ffreestanding -nostdlib -fno-exceptions -fno-rtti -fno-stack-protector -mgeneral-regs-only -fno-pie -O2 -Iinclude`. No libc, no `<cstdint>`.
- Commands:
  - `make` / `make all` → `build/myos.elf`
  - `make run` → boots `build/myos.elf` in QEMU
  - `make clean` → removes `build/` (all `.o` and the ELF)
- QEMU args (also in `run.sh`): `qemu-system-aarch64 -M virt -cpu cortex-a72 -m 128M -nographic -kernel build/myos.elf`
- There is no test suite or linter configured; verification = `make && make run` and check UART output.

## Directory layout

Sources are split by layer: `arch/` (CPU: boot/exception/switch), `kernel/` (sched/mmu/page_alloc/gic/timer/main), `drivers/` (uart/shell), `lib/` (printf/string). Headers mirror this in `include/` (`include/kernel/`, `include/drivers/`, ...); include paths carry the layer prefix (e.g. `#include "kernel/sched.hpp"`). Build artifacts go to `build/`, git-ignored.

## Architecture

- Entry point `_start` in `arch/boot.S` (section `.text.boot` placed first by `linker.ld`), then calls C-linkage `kmain` in `kernel/main.cpp`.
- Kernel load address `0x40080000`, 16KB stack allocated in linker script (`__stack_top`), BSS zeroed by boot code using `__bss_start`/`__bss_end`.
- UART driver (`drivers/uart.cpp`) talks to PL011 at MMIO base `0x09000000` via volatile pointers; `uart_putc` polls TXFF and prepends `\r` before `\n`.
- `kmain` issues PSCI `SYSTEM_OFF` (`w0=0x84000008`, `hvc #0`) and falls back to a `wfi` loop. A successful run powers off QEMU on its own — exit back to shell is normal, not a crash.
- Custom fixed-width types `u8/u32/u64` are defined in `include/types.hpp`; headers reuse them via `#include "types.hpp"`. Do not pull in standard headers.

## Gotchas

- Build artifacts live in `build/` and are git-ignored; run `make clean` before building after edits.
- `.vscode` configs (C_Cpp_Runner, launch.json) reference host `gcc/g++` and a `build/Debug` path that do not match this Makefile-driven cross build — do not rely on them.

## Conventions

- Code comments are written in Chinese; match this when editing nearby code.
