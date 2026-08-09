#include "kernel/page_alloc.hpp"
#include "types.hpp"

/* linker.ld 定义的符号，标记内核镜像末尾 */
extern "C" {
    extern u8 __stack_top;
}
static void* free_list = nullptr;

void page_alloc_init()
{
    /* 从 __stack_top 开始，向上对齐到 4KB */
    u64 start = (u64)&__stack_top;
    start = (start + 0xFFF) & ~0xFFF;
    /* QEMU 128MB RAM: 0x40000000 ~ 0x48000000 */
    u64 end = 0x40000000 + 128 * 1024 * 1024;

    free_list = nullptr;
    for (u64 addr = start; addr + 0x1000 <= end; addr += 0x1000) {
        *(void**)addr = free_list;    /* 当前页指向上一个空闲页 */
        free_list = (void*)addr;      /* 当前页成为新的链表头 */
    }
}

void* alloc_page()
{
    if (!free_list)
        return nullptr;
    void* page = free_list;
    free_list = *(void**)page;        /* 链表头后移 */
    return page;
}

void free_page(void* ptr)
{
    *(void**)ptr = free_list;         /* 归还的页指向当前链表头 */
    free_list = ptr;                  /* 归还的页成为新的链表头 */
}