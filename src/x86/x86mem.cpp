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
    int word = ld8_mem8_kernel_read();
    mem8_loc++;
    word |= ld8_mem8_kernel_read() << 8;
    mem8_loc--;
    return word;
}
int x86Internal::ld16_mem8_kernel_read() {
    int tlb_lookup;
    return ((tlb_lookup = tlb_read_kernel[mem8_loc >> 12]) | mem8_loc) & 1
               ? __ld16_mem8_kernel_read()
               : phys_mem16[(mem8_loc ^ tlb_lookup) >> 1];
}
int x86Internal::__ld32_mem8_kernel_read() {
    int dword = ld8_mem8_kernel_read();
    mem8_loc++;
    dword |= ld8_mem8_kernel_read() << 8;
    mem8_loc++;
    dword |= ld8_mem8_kernel_read() << 16;
    mem8_loc++;
    dword |= ld8_mem8_kernel_read() << 24;
    mem8_loc -= 3;
    return dword;
}
int x86Internal::ld32_mem8_kernel_read() {
    int tlb_lookup;
    return ((tlb_lookup = tlb_read_kernel[mem8_loc >> 12]) | mem8_loc) & 3
               ? __ld32_mem8_kernel_read()
               : phys_mem32[(mem8_loc ^ tlb_lookup) >> 2];
}
int x86Internal::ld16_mem8_direct() {
    int lower_byte = phys_mem8[far++];
    int upper_byte = phys_mem8[far++];
    return lower_byte | (upper_byte << 8);
}
int x86Internal::__ld8_mem8_read() {
    int byte;
    if (check_protected()) {
        do_tlb_set_page(mem8_loc, 0, cpl == 3);
        int tlb_lookup = tlb_read[mem8_loc >> 12];
        byte = phys_mem8[mem8_loc ^ tlb_lookup];
    } else {
        byte = phys_mem8[mem8_loc];
    }
    return byte;
}
int x86Internal::ld8_mem8_read() {
    return (check_real__v86() ||
                    ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                ? __ld8_mem8_read()
                : phys_mem8[mem8_loc ^ tlb_hash]);
}
int x86Internal::__ld16_mem8_read() {
    int word = ld8_mem8_read();
    mem8_loc++;
    word |= ld8_mem8_read() << 8;
    mem8_loc--;
    return word;
}
int x86Internal::ld16_mem8_read() {
    return (check_real__v86() ||
                    ((tlb_hash = tlb_read[mem8_loc >> 12]) | mem8_loc) & 1
                ? __ld16_mem8_read()
                : phys_mem16[(mem8_loc ^ tlb_hash) >> 1]);
}
int x86Internal::__ld32_mem8_read() {
    int dword = ld8_mem8_read();
    mem8_loc++;
    dword |= ld8_mem8_read() << 8;
    mem8_loc++;
    dword |= ld8_mem8_read() << 16;
    mem8_loc++;
    dword |= ld8_mem8_read() << 24;
    mem8_loc -= 3;
    return dword;
}
int x86Internal::ld32_mem8_read() {
    return (check_real__v86() ||
                    ((tlb_hash = tlb_read[mem8_loc >> 12]) | mem8_loc) & 3
                ? __ld32_mem8_read()
                : phys_mem32[(mem8_loc ^ tlb_hash) >> 2]);
}
int x86Internal::__ld8_mem8_write() {
    int byte;
    if (check_protected()) {
        do_tlb_set_page(mem8_loc, 1, cpl == 3);
        int tlb_lookup = tlb_write[mem8_loc >> 12];
        byte = phys_mem8[mem8_loc ^ tlb_lookup];
    } else {
        byte = phys_mem8[mem8_loc];
    }
    return byte;
}
int x86Internal::ld8_mem8_write() {
    int tlb_lookup;
    return (check_real__v86() ||
                    (tlb_lookup = tlb_write[mem8_loc >> 12]) == -1)
                ? __ld8_mem8_write()
                : phys_mem8[mem8_loc ^ tlb_lookup];
}
int x86Internal::__ld16_mem8_write() {
    int word = ld8_mem8_write();
    mem8_loc++;
    word |= ld8_mem8_write() << 8;
    mem8_loc--;
    return word;
}
int x86Internal::ld16_mem8_write() {
    int tlb_lookup;
    return (check_real__v86() ||
                    (tlb_lookup = tlb_write[mem8_loc >> 12]) | mem8_loc) & 1
                ? __ld16_mem8_write()
                : phys_mem16[(mem8_loc ^ tlb_lookup) >> 1];
}
int x86Internal::__ld32_mem8_write() {
    int dword = ld8_mem8_write();
    mem8_loc++;
    dword |= ld8_mem8_write() << 8;
    mem8_loc++;
    dword |= ld8_mem8_write() << 16;
    mem8_loc++;
    dword |= ld8_mem8_write() << 24;
    mem8_loc -= 3;
    return dword;
}
int x86Internal::ld32_mem8_write() {
    int tlb_lookup;
    return (check_real__v86() ||
                    (tlb_lookup = tlb_write[mem8_loc >> 12]) | mem8_loc) & 3
               ? __ld32_mem8_write()
               : phys_mem32[(mem8_loc ^ tlb_lookup) >> 2];
}
void x86Internal::__st8_mem8_kernel_write(int byte) {
    do_tlb_set_page(mem8_loc, 1, 0);
    int tlb_lookup = tlb_write_kernel[mem8_loc >> 12];
    phys_mem8[mem8_loc ^ tlb_lookup] = byte;
}
void x86Internal::st8_mem8_kernel_write(int byte) {
    int tlb_lookup = tlb_write_kernel[mem8_loc >> 12];
    if (tlb_lookup == -1) {
        __st8_mem8_kernel_write(byte);
    } else {
        phys_mem8[mem8_loc ^ tlb_lookup] = byte;
    }
}
void x86Internal::__st16_mem8_kernel_write(int word) {
    st8_mem8_kernel_write(word);
    mem8_loc++;
    st8_mem8_kernel_write(word >> 8);
    mem8_loc--;
}
void x86Internal::st16_mem8_kernel_write(int word) {
    int tlb_lookup = tlb_write_kernel[mem8_loc >> 12];
    if ((tlb_lookup | mem8_loc) & 1) {
        __st16_mem8_kernel_write(word);
    } else {
        phys_mem16[(mem8_loc ^ tlb_lookup) >> 1] = word;
    }
}
void x86Internal::__st32_mem8_kernel_write(int dword) {
    st8_mem8_kernel_write(dword);
    mem8_loc++;
    st8_mem8_kernel_write(dword >> 8);
    mem8_loc++;
    st8_mem8_kernel_write(dword >> 16);
    mem8_loc++;
    st8_mem8_kernel_write(dword >> 24);
    mem8_loc -= 3;
}
void x86Internal::st32_mem8_kernel_write(int dword) {
    int tlb_lookup = tlb_write_kernel[mem8_loc >> 12];
    if ((tlb_lookup | mem8_loc) & 3) {
        __st32_mem8_kernel_write(dword);
    } else {
        phys_mem32[(mem8_loc ^ tlb_lookup) >> 2] = dword;
    }
}
void x86Internal::__st8_mem8_write(int byte) {
    if (check_protected()) {
        do_tlb_set_page(mem8_loc, 1, cpl == 3);
        int tlb_lookup = tlb_write[mem8_loc >> 12];
        phys_mem8[mem8_loc ^ tlb_lookup] = byte;
    } else {
        phys_mem8[mem8_loc] = byte;
    }
}
void x86Internal::st8_mem8_write(int byte) {
    tlb_hash = tlb_write[mem8_loc >> 12];
    if (check_real__v86() || tlb_hash == -1) {
        __st8_mem8_write(byte);
    } else {
        phys_mem8[mem8_loc ^ tlb_hash] = byte;
    }
}
void x86Internal::__st16_mem8_write(int word) {
    st8_mem8_write(word);
    mem8_loc++;
    st8_mem8_write(word >> 8);
    mem8_loc--;
}
void x86Internal::st16_mem8_write(int word) {
    tlb_hash = tlb_write[mem8_loc >> 12];
    if (check_real__v86() || (tlb_hash | mem8_loc) & 1) {
        __st16_mem8_write(word);
    } else {
        phys_mem16[(mem8_loc ^ tlb_hash) >> 1] = word;
    }
}
void x86Internal::__st32_mem8_write(int dword) {
    st8_mem8_write(dword);
    mem8_loc++;
    st8_mem8_write(dword >> 8);
    mem8_loc++;
    st8_mem8_write(dword >> 16);
    mem8_loc++;
    st8_mem8_write(dword >> 24);
    mem8_loc -= 3;
}
void x86Internal::st32_mem8_write(int dword) {
    tlb_hash = tlb_write[mem8_loc >> 12];
    if (check_real__v86() || (tlb_hash | mem8_loc) & 3) {
        __st32_mem8_write(dword);
    } else {
        int idx = (mem8_loc ^ tlb_hash) >> 2;
        phys_mem32[idx] = dword;
    }
}
void x86Internal::push_word(int word) {
    int esp = regs[4] - 2;
    mem8_loc = (esp & SS_mask) + SS_base;
    st16_mem8_write(word);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
}
void x86Internal::push_dword(int dword) {
    int esp = regs[4] - 4;
    mem8_loc = (esp & SS_mask) + SS_base;
    st32_mem8_write(dword);
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
