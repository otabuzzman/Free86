#include "x86.h"

int x86Internal::__ld8_mem8_kernel_read() {
    do_tlb_set_page(mem8_loc, 0, 0);
    int tlb_lookup = tlb_read_kernel[mem8_loc >> 12];
    return phys_mem8[mem8_loc ^ tlb_lookup];
}
int x86Internal::ld8_mem8_kernel_read() {
    int tlb_lookup;
    return ((tlb_lookup = tlb_read_kernel[mem8_loc >> 12]) == -1)
               ? __ld8_mem8_kernel_read()
               : phys_mem8[mem8_loc ^ tlb_lookup];
}
int x86Internal::__ld16_mem8_kernel_read() {
    int x = ld8_mem8_kernel_read();
    mem8_loc++;
    x |= ld8_mem8_kernel_read() << 8;
    mem8_loc--;
    return x;
}
int x86Internal::ld16_mem8_kernel_read() {
    int tlb_lookup;
    return ((tlb_lookup = tlb_read_kernel[mem8_loc >> 12]) | mem8_loc) & 1
               ? __ld16_mem8_kernel_read()
               : phys_mem16[(mem8_loc ^ tlb_lookup) >> 1];
}
int x86Internal::__ld32_mem8_kernel_read() {
    int x = ld8_mem8_kernel_read();
    mem8_loc++;
    x |= ld8_mem8_kernel_read() << 8;
    mem8_loc++;
    x |= ld8_mem8_kernel_read() << 16;
    mem8_loc++;
    x |= ld8_mem8_kernel_read() << 24;
    mem8_loc -= 3;
    return x;
}
int x86Internal::ld32_mem8_kernel_read() {
    int tlb_lookup;
    return ((tlb_lookup = tlb_read_kernel[mem8_loc >> 12]) | mem8_loc) & 3
               ? __ld32_mem8_kernel_read()
               : phys_mem32[(mem8_loc ^ tlb_lookup) >> 2];
}
int x86Internal::ld16_mem8_direct() {
    int x = phys_mem8[far++];
    int y = phys_mem8[far++];
    return x | (y << 8);
}
int x86Internal::__ld8_mem8_read() {
    int mem8_val;
    if (check_protected()) {
        do_tlb_set_page(mem8_loc, 0, cpl == 3);
        int tlb_lookup = tlb_read[mem8_loc >> 12];
        mem8_val = phys_mem8[mem8_loc ^ tlb_lookup];
    } else {
        mem8_val = phys_mem8[mem8_loc];
    }
    return mem8_val;
}
int x86Internal::ld8_mem8_read() {
    return (check_real__v86() ||
                    ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                ? __ld8_mem8_read()
                : phys_mem8[mem8_loc ^ tlb_hash]);
}
int x86Internal::__ld16_mem8_read() {
    int x = ld8_mem8_read();
    mem8_loc++;
    x |= ld8_mem8_read() << 8;
    mem8_loc--;
    return x;
}
int x86Internal::ld16_mem8_read() {
    return (check_real__v86() ||
                    ((tlb_hash = tlb_read[mem8_loc >> 12]) | mem8_loc) & 1
                ? __ld16_mem8_read()
                : phys_mem16[(mem8_loc ^ tlb_hash) >> 1]);
}
int x86Internal::__ld32_mem8_read() {
    int x = ld8_mem8_read();
    mem8_loc++;
    x |= ld8_mem8_read() << 8;
    mem8_loc++;
    x |= ld8_mem8_read() << 16;
    mem8_loc++;
    x |= ld8_mem8_read() << 24;
    mem8_loc -= 3;
    return x;
}
int x86Internal::ld32_mem8_read() {
    return (check_real__v86() ||
                    ((tlb_hash = tlb_read[mem8_loc >> 12]) | mem8_loc) & 3
                ? __ld32_mem8_read()
                : phys_mem32[(mem8_loc ^ tlb_hash) >> 2]);
}
int x86Internal::__ld8_mem8_write() {
    int mem8_val;
    if (check_protected()) {
        do_tlb_set_page(mem8_loc, 1, cpl == 3);
        int tlb_lookup = tlb_write[mem8_loc >> 12];
        mem8_val = phys_mem8[mem8_loc ^ tlb_lookup];
    } else {
        mem8_val = phys_mem8[mem8_loc];
    }
    return mem8_val;
}
int x86Internal::ld8_mem8_write() {
    int tlb_lookup;
    return (check_real__v86() ||
                    (tlb_lookup = tlb_write[mem8_loc >> 12]) == -1)
                ? __ld8_mem8_write()
                : phys_mem8[mem8_loc ^ tlb_lookup];
}
int x86Internal::__ld16_mem8_write() {
    int x = ld8_mem8_write();
    mem8_loc++;
    x |= ld8_mem8_write() << 8;
    mem8_loc--;
    return x;
}
int x86Internal::ld16_mem8_write() {
    int tlb_lookup;
    return (check_real__v86() ||
                    (tlb_lookup = tlb_write[mem8_loc >> 12]) | mem8_loc) & 1
                ? __ld16_mem8_write()
                : phys_mem16[(mem8_loc ^ tlb_lookup) >> 1];
}
int x86Internal::__ld32_mem8_write() {
    int x = ld8_mem8_write();
    mem8_loc++;
    x |= ld8_mem8_write() << 8;
    mem8_loc++;
    x |= ld8_mem8_write() << 16;
    mem8_loc++;
    x |= ld8_mem8_write() << 24;
    mem8_loc -= 3;
    return x;
}
int x86Internal::ld32_mem8_write() {
    int tlb_lookup;
    return (check_real__v86() ||
                    (tlb_lookup = tlb_write[mem8_loc >> 12]) | mem8_loc) & 3
               ? __ld32_mem8_write()
               : phys_mem32[(mem8_loc ^ tlb_lookup) >> 2];
}
void x86Internal::__st8_mem8_kernel_write(int x) {
    do_tlb_set_page(mem8_loc, 1, 0);
    int tlb_lookup = tlb_write_kernel[mem8_loc >> 12];
    phys_mem8[mem8_loc ^ tlb_lookup] = x;
}
void x86Internal::st8_mem8_kernel_write(int x) {
    int tlb_lookup = tlb_write_kernel[mem8_loc >> 12];
    if (tlb_lookup == -1) {
        __st8_mem8_kernel_write(x);
    } else {
        phys_mem8[mem8_loc ^ tlb_lookup] = x;
    }
}
void x86Internal::__st16_mem8_kernel_write(int x) {
    st8_mem8_kernel_write(x);
    mem8_loc++;
    st8_mem8_kernel_write(x >> 8);
    mem8_loc--;
}
void x86Internal::st16_mem8_kernel_write(int x) {
    int tlb_lookup = tlb_write_kernel[mem8_loc >> 12];
    if ((tlb_lookup | mem8_loc) & 1) {
        __st16_mem8_kernel_write(x);
    } else {
        phys_mem16[(mem8_loc ^ tlb_lookup) >> 1] = x;
    }
}
void x86Internal::__st32_mem8_kernel_write(int x) {
    st8_mem8_kernel_write(x);
    mem8_loc++;
    st8_mem8_kernel_write(x >> 8);
    mem8_loc++;
    st8_mem8_kernel_write(x >> 16);
    mem8_loc++;
    st8_mem8_kernel_write(x >> 24);
    mem8_loc -= 3;
}
void x86Internal::st32_mem8_kernel_write(int x) {
    int tlb_lookup = tlb_write_kernel[mem8_loc >> 12];
    if ((tlb_lookup | mem8_loc) & 3) {
        __st32_mem8_kernel_write(x);
    } else {
        phys_mem32[(mem8_loc ^ tlb_lookup) >> 2] = x;
    }
}
void x86Internal::__st8_mem8_write(int x) {
    if (check_protected()) {
        do_tlb_set_page(mem8_loc, 1, cpl == 3);
        int tlb_lookup = tlb_write[mem8_loc >> 12];
        phys_mem8[mem8_loc ^ tlb_lookup] = x;
    } else {
        phys_mem8[mem8_loc] = x;
    }
}
void x86Internal::st8_mem8_write(int x) {
    tlb_hash = tlb_write[mem8_loc >> 12];
    if (check_real__v86() || tlb_hash == -1) {
        __st8_mem8_write(x);
    } else {
        phys_mem8[mem8_loc ^ tlb_hash] = x;
    }
}
void x86Internal::__st16_mem8_write(int x) {
    st8_mem8_write(x);
    mem8_loc++;
    st8_mem8_write(x >> 8);
    mem8_loc--;
}
void x86Internal::st16_mem8_write(int x) {
    tlb_hash = tlb_write[mem8_loc >> 12];
    if (check_real__v86() || (tlb_hash | mem8_loc) & 1) {
        __st16_mem8_write(x);
    } else {
        phys_mem16[(mem8_loc ^ tlb_hash) >> 1] = x;
    }
}
void x86Internal::__st32_mem8_write(int x) {
    st8_mem8_write(x);
    mem8_loc++;
    st8_mem8_write(x >> 8);
    mem8_loc++;
    st8_mem8_write(x >> 16);
    mem8_loc++;
    st8_mem8_write(x >> 24);
    mem8_loc -= 3;
}
void x86Internal::st32_mem8_write(int x) {
    tlb_hash = tlb_write[mem8_loc >> 12];
    if (check_real__v86() || (tlb_hash | mem8_loc) & 3) {
        __st32_mem8_write(x);
    } else {
        int idx = (mem8_loc ^ tlb_hash) >> 2;
        phys_mem32[idx] = x;
    }
}
void x86Internal::push_word(int x) {
    int esp = regs[4] - 2;
    mem8_loc = (esp & SS_mask) + SS_base;
    st16_mem8_write(x);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
}
void x86Internal::push_dword(int x) {
    int esp = regs[4] - 4;
    mem8_loc = (esp & SS_mask) + SS_base;
    st32_mem8_write(x);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
}
void x86Internal::pop_word() {
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 2) & SS_mask);
}
void x86Internal::pop_dword() {
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 4) & SS_mask);
}
int x86Internal::read_stack_word() {
    mem8_loc = (regs[4] & SS_mask) + SS_base;
    return ld16_mem8_read();
}
int x86Internal::read_stack_dword() {
    mem8_loc = (regs[4] & SS_mask) + SS_base;
    return ld32_mem8_read();
}
