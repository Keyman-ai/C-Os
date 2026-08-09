#include "lib/printf.hpp"

extern "C" void exception_handler(u64 esr, u64 elr) {
    unsigned int ec = (esr >> 26) & 0x3F;
    unsigned int iss = esr & 0x1FFFFFF;
    printf("EXCEPTION: EC=%x ISS=%x ELR=%x\n", ec, iss, elr);
    while (1) { __asm__ volatile("wfi"); }
}