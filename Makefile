# 工具链前缀
CROSS   = aarch64-linux-gnu-
CXX     = $(CROSS)g++
LD      = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy

# 编译 flags
CXXFLAGS = -ffreestanding -nostdlib \
           -fno-exceptions -fno-rtti -fno-stack-protector \
           -mgeneral-regs-only -O2 -Wall -Wextra \
           -fno-pie -Iinclude

# 链接 flags
LDFLAGS = -T linker.ld -nostdlib -ffreestanding -static -no-pie

# 源文件
CXX_SOURCES = src/main.cpp src/uart.cpp src/printf.cpp src/exception_handler.cpp
ASM_SOURCES = src/boot.S src/exception.S
OBJECTS = src/boot.o src/main.o src/uart.o src/printf.o src/exception.o src/exception_handler.o

# 默认目标
all: myos.elf

# 编译 C++
src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 编译汇编
src/%.o: src/%.S
	$(CROSS)gcc $(CXXFLAGS) -c $< -o $@

# 链接
myos.elf: $(OBJECTS) linker.ld
	$(CXX) $(LDFLAGS) $(OBJECTS) -o myos.elf

# 启动 QEMU
run: myos.elf
	qemu-system-aarch64 -M virt -cpu cortex-a72 -m 128M -nographic -kernel myos.elf

# 清理
clean:
	rm -f src/*.o myos.elf

.PHONY: all run clean