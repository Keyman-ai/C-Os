# AGENTS.md

Compact guidance for working in this bare-metal aarch64 OS project.

## Toolchain & build

- Cross-compile with `aarch64-linux-gnu-` (g++ / ld / gcc for asm). Host gcc/g++ will NOT work.
- Freestanding C++: `-ffreestanding -nostdlib -fno-exceptions -fno-rtti -fno-stack-protector -mgeneral-regs-only -fno-pie -O2 -Iinclude`. No libc, no `<cstdint>`.
- Commands:
  - `make` / `make all` → `myos.elf`
  - `make run` → boots `myos.elf` in QEMU
  - `make clean` → removes `src/*.o` and `myos.elf`
- QEMU args (also in `run.sh`): `qemu-system-aarch64 -M virt -cpu cortex-a72 -m 128M -nographic -kernel myos.elf`
- There is no test suite or linter configured; verification = `make && make run` and check UART output.

## Architecture

- Entry point `_start` in `src/boot.S` (section `.text.boot` placed first by `linker.ld`), then calls C-linkage `kmain` in `src/main.cpp`.
- Kernel load address `0x40080000`, 16KB stack allocated in linker script (`__stack_top`), BSS zeroed by boot code using `__bss_start`/`__bss_end`.
- UART driver (`src/uart.cpp`) talks to PL011 at MMIO base `0x09000000` via volatile pointers; `uart_putc` polls TXFF and prepends `\r` before `\n`.
- `kmain` issues PSCI `SYSTEM_OFF` (`w0=0x84000008`, `hvc #0`) and falls back to a `wfi` loop. A successful run powers off QEMU on its own — exit back to shell is normal, not a crash.
- Custom fixed-width types `u8/u32/u64` are defined in `include/uart.hpp`; headers reuse them via `#include "uart.hpp"`. Do not pull in standard headers.

## Gotchas

- **`src/printf.cpp` is NOT in the Makefile** (`CXX_SOURCES = src/main.cpp src/uart.cpp`). `include/printf.hpp` declares `printf()`, but nothing provides it — using `printf` from `main` will fail at link time. To enable it: add `src/printf.cpp` to `CXX_SOURCES` + `OBJECTS`, and note the file currently contains a stray `#endif` (around the `s64` typedef block) that is a compile error and must be fixed first. The `%d %i %u %x %X %c %s %p` handlers are also still TODO stubs (empty cases).
- Stale `src/*.o` and `myos.elf` are committed/checked in; `make clean` before building after edits.
- `.vscode` configs (C_Cpp_Runner, launch.json) reference host `gcc/g++` and a `build/Debug` path that do not match this Makefile-driven cross build — do not rely on them.

## Conventions

- Code comments are written in Chinese; match this when editing nearby code.