#include "printf.hpp"

extern "C" void exception_handler(unsigned long esr, unsigned long elr) {
    unsigned int ec = (esr >> 26) & 0x3F;
    unsigned int iss = esr & 0x1FFFFFF;
    printf("EXCEPTION: EC=%x ISS=%x ELR=%x\n", ec, iss, elr);
    while (1) { __asm__ volatile("wfi"); }
}