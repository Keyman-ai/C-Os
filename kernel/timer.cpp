#include "kernel/timer.hpp"
#include "arch/sysreg.hpp"
#include "lib/printf.hpp"
#include "kernel/sched.hpp"
void timer_init()
{
    u64 freq = READ_SYSREG(CNTFRQ_EL0);
    u64 cnt  = READ_SYSREG(CNTPCT_EL0);
    WRITE_SYSREG(CNTP_CVAL_EL0, cnt + freq / 5);

    u64 ctl = READ_SYSREG(CNTP_CTL_EL0);
    ctl |= 1;
    ctl &= ~(1 << 1);
    WRITE_SYSREG(CNTP_CTL_EL0, ctl);
}

extern "C" void irq_handler()
{
    u64 iar = READ_SYSREG(ICC_IAR1_EL1);
    u64 id  = iar & 0x3FF;

    WRITE_SYSREG(ICC_EOIR1_EL1, iar);   /* 先 EOI：立刻释放 GIC 优先级 */

    if (id == 30) {
        u64 freq = READ_SYSREG(CNTFRQ_EL0);
        WRITE_SYSREG(CNTP_CVAL_EL0, READ_SYSREG(CNTPCT_EL0) + freq / 5);
        printf("tick\n");
        schedule();                     /* 时间片到：抢占，切换下一个线程 */
    }
}
