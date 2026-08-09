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

