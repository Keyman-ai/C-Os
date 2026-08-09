# 工具链前缀
CROSS   = aarch64-linux-gnu-
CXX     = $(CROSS)g++

# 编译 flags
CXXFLAGS = -ffreestanding -nostdlib \
           -fno-exceptions -fno-rtti -fno-stack-protector \
           -mgeneral-regs-only -O2 -Wall -Wextra \
           -fno-pie -Iinclude

# 链接 flags
LDFLAGS = -T linker.ld -nostdlib -ffreestanding -static -no-pie

# 源文件（链接顺序：arch 第一，其余任意）
CXX_SOURCES = arch/exception_handler.cpp \
              kernel/main.cpp kernel/sched.cpp kernel/mmu.cpp \
              kernel/page_alloc.cpp kernel/gic.cpp kernel/timer.cpp \
              drivers/uart.cpp drivers/shell.cpp \
              lib/printf.cpp lib/string.cpp
ASM_SOURCES = arch/boot.S arch/exception.S arch/switch.S

# 编译产物目录：对象与 elf 全部输出到 build/，镜像源码子目录
OBJ_DIR = build
OBJECTS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(CXX_SOURCES)) \
          $(patsubst %.S,$(OBJ_DIR)/%.o,$(ASM_SOURCES))

# 默认目标
all: $(OBJ_DIR)/myos.elf

# 编译 C++
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 编译汇编
$(OBJ_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CROSS)gcc $(CXXFLAGS) -c $< -o $@

# 链接
$(OBJ_DIR)/myos.elf: $(OBJECTS) linker.ld
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

# 启动 QEMU
run: $(OBJ_DIR)/myos.elf
	qemu-system-aarch64 -M virt,gic-version=3 -cpu cortex-a72 -m 128M -nographic -kernel $(OBJ_DIR)/myos.elf

# 清理
clean:
	rm -rf $(OBJ_DIR)

.PHONY: all run clean
