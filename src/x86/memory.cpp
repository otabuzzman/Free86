#include "x86.h"

int Free86::_ld8_mem8_kernel_read() {
    page_translation(mem8_loc, 0, 0);
    tlb_hash = tlb_read_kernel[mem8_loc >> 12];
    return phys_mem8[mem8_loc ^ tlb_hash];
}
int Free86::ld8_mem8_kernel_read() {
    return ((tlb_hash = tlb_read_kernel[mem8_loc >> 12]) == -1)
               ? _ld8_mem8_kernel_read()
               : phys_mem8[mem8_loc ^ tlb_hash];
}
int Free86::_ld16_mem8_kernel_read() {
    int word = ld8_mem8_kernel_read();
    mem8_loc++;
    word |= ld8_mem8_kernel_read() << 8;
    mem8_loc--;
    return word;
}
int Free86::ld16_mem8_kernel_read() {
    return ((tlb_hash = tlb_read_kernel[mem8_loc >> 12]) | mem8_loc) & 1
               ? _ld16_mem8_kernel_read()
               : phys_mem16[(mem8_loc ^ tlb_hash) >> 1];
}
int Free86::_ld32_mem8_kernel_read() {
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
int Free86::ld32_mem8_kernel_read() {
    return ((tlb_hash = tlb_read_kernel[mem8_loc >> 12]) | mem8_loc) & 3
               ? _ld32_mem8_kernel_read()
               : phys_mem32[(mem8_loc ^ tlb_hash) >> 2];
}
int Free86::ld16_mem8_direct() {
    int lower_byte = phys_mem8[far++];
    int upper_byte = phys_mem8[far++];
    return lower_byte | (upper_byte << 8);
}
int Free86::_ld8_mem8_read() {
    int byte;
    if (is_protected()) {
        page_translation(mem8_loc, 0, cpl == 3);
        tlb_hash = tlb_read[mem8_loc >> 12];
        byte = phys_mem8[mem8_loc ^ tlb_hash];
    } else {
        byte = phys_mem8[mem8_loc];
    }
    return byte;
}
int Free86::ld8_mem8_read() {
    return (is_real__v86() ||
                    ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                ? _ld8_mem8_read()
                : phys_mem8[mem8_loc ^ tlb_hash]);
}
int Free86::_ld16_mem8_read() {
    int word = ld8_mem8_read();
    mem8_loc++;
    word |= ld8_mem8_read() << 8;
    mem8_loc--;
    return word;
}
int Free86::ld16_mem8_read() {
    return (is_real__v86() ||
                    ((tlb_hash = tlb_read[mem8_loc >> 12]) | mem8_loc) & 1
                ? _ld16_mem8_read()
                : phys_mem16[(mem8_loc ^ tlb_hash) >> 1]);
}
int Free86::_ld32_mem8_read() {
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
int Free86::ld32_mem8_read() {
    return (is_real__v86() ||
                    ((tlb_hash = tlb_read[mem8_loc >> 12]) | mem8_loc) & 3
                ? _ld32_mem8_read()
                : phys_mem32[(mem8_loc ^ tlb_hash) >> 2]);
}
int Free86::_ld8_mem8_write() {
    int byte;
    if (is_protected()) {
        page_translation(mem8_loc, 1, cpl == 3);
        tlb_hash = tlb_write[mem8_loc >> 12];
        byte = phys_mem8[mem8_loc ^ tlb_hash];
    } else {
        byte = phys_mem8[mem8_loc];
    }
    return byte;
}
int Free86::ld8_mem8_write() {
    return (is_real__v86() ||
                    (tlb_hash = tlb_write[mem8_loc >> 12]) == -1)
                ? _ld8_mem8_write()
                : phys_mem8[mem8_loc ^ tlb_hash];
}
int Free86::_ld16_mem8_write() {
    int word = ld8_mem8_write();
    mem8_loc++;
    word |= ld8_mem8_write() << 8;
    mem8_loc--;
    return word;
}
int Free86::ld16_mem8_write() {
    return (is_real__v86() ||
                    (tlb_hash = tlb_write[mem8_loc >> 12]) | mem8_loc) & 1
                ? _ld16_mem8_write()
                : phys_mem16[(mem8_loc ^ tlb_hash) >> 1];
}
int Free86::_ld32_mem8_write() {
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
int Free86::ld32_mem8_write() {
    return (is_real__v86() ||
                    (tlb_hash = tlb_write[mem8_loc >> 12]) | mem8_loc) & 3
               ? _ld32_mem8_write()
               : phys_mem32[(mem8_loc ^ tlb_hash) >> 2];
}
void Free86::_st8_mem8_kernel_write(int byte) {
    page_translation(mem8_loc, 1, 0);
    tlb_hash = tlb_write_kernel[mem8_loc >> 12];
    phys_mem8[mem8_loc ^ tlb_hash] = byte;
}
void Free86::st8_mem8_kernel_write(int byte) {
    tlb_hash = tlb_write_kernel[mem8_loc >> 12];
    if (tlb_hash == -1) {
        _st8_mem8_kernel_write(byte);
    } else {
        phys_mem8[mem8_loc ^ tlb_hash] = byte;
    }
}
void Free86::_st16_mem8_kernel_write(int word) {
    st8_mem8_kernel_write(word);
    mem8_loc++;
    st8_mem8_kernel_write(word >> 8);
    mem8_loc--;
}
void Free86::st16_mem8_kernel_write(int word) {
    tlb_hash = tlb_write_kernel[mem8_loc >> 12];
    if ((tlb_hash | mem8_loc) & 1) {
        _st16_mem8_kernel_write(word);
    } else {
        phys_mem16[(mem8_loc ^ tlb_hash) >> 1] = word;
    }
}
void Free86::_st32_mem8_kernel_write(int dword) {
    st8_mem8_kernel_write(dword);
    mem8_loc++;
    st8_mem8_kernel_write(dword >> 8);
    mem8_loc++;
    st8_mem8_kernel_write(dword >> 16);
    mem8_loc++;
    st8_mem8_kernel_write(dword >> 24);
    mem8_loc -= 3;
}
void Free86::st32_mem8_kernel_write(int dword) {
    tlb_hash = tlb_write_kernel[mem8_loc >> 12];
    if ((tlb_hash | mem8_loc) & 3) {
        _st32_mem8_kernel_write(dword);
    } else {
        phys_mem32[(mem8_loc ^ tlb_hash) >> 2] = dword;
    }
}
void Free86::_st8_mem8_write(int byte) {
    if (is_protected()) {
        page_translation(mem8_loc, 1, cpl == 3);
        tlb_hash = tlb_write[mem8_loc >> 12];
        phys_mem8[mem8_loc ^ tlb_hash] = byte;
    } else {
        phys_mem8[mem8_loc] = byte;
    }
}
void Free86::st8_mem8_write(int byte) {
    tlb_hash = tlb_write[mem8_loc >> 12];
    if (is_real__v86() || tlb_hash == -1) {
        _st8_mem8_write(byte);
    } else {
        phys_mem8[mem8_loc ^ tlb_hash] = byte;
    }
}
void Free86::_st16_mem8_write(int word) {
    st8_mem8_write(word);
    mem8_loc++;
    st8_mem8_write(word >> 8);
    mem8_loc--;
}
void Free86::st16_mem8_write(int word) {
    tlb_hash = tlb_write[mem8_loc >> 12];
    if (is_real__v86() || (tlb_hash | mem8_loc) & 1) {
        _st16_mem8_write(word);
    } else {
        phys_mem16[(mem8_loc ^ tlb_hash) >> 1] = word;
    }
}
void Free86::_st32_mem8_write(int dword) {
    st8_mem8_write(dword);
    mem8_loc++;
    st8_mem8_write(dword >> 8);
    mem8_loc++;
    st8_mem8_write(dword >> 16);
    mem8_loc++;
    st8_mem8_write(dword >> 24);
    mem8_loc -= 3;
}
void Free86::st32_mem8_write(int dword) {
    tlb_hash = tlb_write[mem8_loc >> 12];
    if (is_real__v86() || (tlb_hash | mem8_loc) & 3) {
        _st32_mem8_write(dword);
    } else {
        phys_mem32[(mem8_loc ^ tlb_hash) >> 2] = dword;
    }
}
void Free86::push_word(int word) {
    int esp = regs[4] - 2;
    mem8_loc = (esp & SS_mask) + SS_base;
    st16_mem8_write(word);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
}
void Free86::push_dword(int dword) {
    int esp = regs[4] - 4;
    mem8_loc = (esp & SS_mask) + SS_base;
    st32_mem8_write(dword);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
}
void Free86::pop_word() {
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 2) & SS_mask);
}
void Free86::pop_dword() {
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 4) & SS_mask);
}
int Free86::read_stack_word() {
    mem8_loc = (regs[4] & SS_mask) + SS_base;
    return ld16_mem8_read();
}
int Free86::read_stack_dword() {
    mem8_loc = (regs[4] & SS_mask) + SS_base;
    return ld32_mem8_read();
}
