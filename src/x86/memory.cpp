#include "free86.h"

int Free86::_ld8_readonly_cplX() {
    page_translation(mem8_loc, 0, 0);
    tlb_hash = tlb_readonly_cplX[mem8_loc >> 12];
    return phys_mem8[mem8_loc ^ tlb_hash];
}
int Free86::ld8_readonly_cplX() {
    return ((tlb_hash = tlb_readonly_cplX[mem8_loc >> 12]) == -1)
               ? _ld8_readonly_cplX()
               : phys_mem8[mem8_loc ^ tlb_hash];
}
int Free86::_ld16_readonly_cplX() {
    int word = ld8_readonly_cplX();
    mem8_loc++;
    word |= ld8_readonly_cplX() << 8;
    mem8_loc--;
    return word;
}
int Free86::ld16_readonly_cplX() {
    return ((tlb_hash = tlb_readonly_cplX[mem8_loc >> 12]) | mem8_loc) & 1
               ? _ld16_readonly_cplX()
               : phys_mem16[(mem8_loc ^ tlb_hash) >> 1];
}
int Free86::_ld32_readonly_cplX() {
    int dword = ld8_readonly_cplX();
    mem8_loc++;
    dword |= ld8_readonly_cplX() << 8;
    mem8_loc++;
    dword |= ld8_readonly_cplX() << 16;
    mem8_loc++;
    dword |= ld8_readonly_cplX() << 24;
    mem8_loc -= 3;
    return dword;
}
int Free86::ld32_readonly_cplX() {
    return ((tlb_hash = tlb_readonly_cplX[mem8_loc >> 12]) | mem8_loc) & 3
               ? _ld32_readonly_cplX()
               : phys_mem32[(mem8_loc ^ tlb_hash) >> 2];
}
int Free86::_ld8_readonly_cpl3() {
    int byte;
    if (is_protected()) {
        page_translation(mem8_loc, 0, cpl == 3);
        tlb_hash = tlb_readonly[mem8_loc >> 12];
        byte = phys_mem8[mem8_loc ^ tlb_hash];
    } else {
        byte = phys_mem8[mem8_loc];
    }
    return byte;
}
int Free86::ld8_readonly_cpl3() {
    return (is_real__v86() ||
                    ((tlb_hash = tlb_readonly[mem8_loc >> 12]) == -1)
                ? _ld8_readonly_cpl3()
                : phys_mem8[mem8_loc ^ tlb_hash]);
}
int Free86::_ld16_readonly_cpl3() {
    int word = ld8_readonly_cpl3();
    mem8_loc++;
    word |= ld8_readonly_cpl3() << 8;
    mem8_loc--;
    return word;
}
int Free86::ld16_readonly_cpl3() {
    return (is_real__v86() ||
                    ((tlb_hash = tlb_readonly[mem8_loc >> 12]) | mem8_loc) & 1
                ? _ld16_readonly_cpl3()
                : phys_mem16[(mem8_loc ^ tlb_hash) >> 1]);
}
int Free86::_ld32_readonly_cpl3() {
    int dword = ld8_readonly_cpl3();
    mem8_loc++;
    dword |= ld8_readonly_cpl3() << 8;
    mem8_loc++;
    dword |= ld8_readonly_cpl3() << 16;
    mem8_loc++;
    dword |= ld8_readonly_cpl3() << 24;
    mem8_loc -= 3;
    return dword;
}
int Free86::ld32_readonly_cpl3() {
    return (is_real__v86() ||
                    ((tlb_hash = tlb_readonly[mem8_loc >> 12]) | mem8_loc) & 3
                ? _ld32_readonly_cpl3()
                : phys_mem32[(mem8_loc ^ tlb_hash) >> 2]);
}
int Free86::_ld8_writable_cpl3() {
    int byte;
    if (is_protected()) {
        page_translation(mem8_loc, 1, cpl == 3);
        tlb_hash = tlb_writable[mem8_loc >> 12];
        byte = phys_mem8[mem8_loc ^ tlb_hash];
    } else {
        byte = phys_mem8[mem8_loc];
    }
    return byte;
}
int Free86::ld8_writable_cpl3() {
    return (is_real__v86() ||
                    (tlb_hash = tlb_writable[mem8_loc >> 12]) == -1)
                ? _ld8_writable_cpl3()
                : phys_mem8[mem8_loc ^ tlb_hash];
}
int Free86::_ld16_writable_cpl3() {
    int word = ld8_writable_cpl3();
    mem8_loc++;
    word |= ld8_writable_cpl3() << 8;
    mem8_loc--;
    return word;
}
int Free86::ld16_writable_cpl3() {
    return (is_real__v86() ||
                    (tlb_hash = tlb_writable[mem8_loc >> 12]) | mem8_loc) & 1
                ? _ld16_writable_cpl3()
                : phys_mem16[(mem8_loc ^ tlb_hash) >> 1];
}
int Free86::_ld32_writable_cpl3() {
    int dword = ld8_writable_cpl3();
    mem8_loc++;
    dword |= ld8_writable_cpl3() << 8;
    mem8_loc++;
    dword |= ld8_writable_cpl3() << 16;
    mem8_loc++;
    dword |= ld8_writable_cpl3() << 24;
    mem8_loc -= 3;
    return dword;
}
int Free86::ld32_writable_cpl3() {
    return (is_real__v86() ||
                    (tlb_hash = tlb_writable[mem8_loc >> 12]) | mem8_loc) & 3
               ? _ld32_writable_cpl3()
               : phys_mem32[(mem8_loc ^ tlb_hash) >> 2];
}
void Free86::_st8_writable_cplX(int byte) {
    page_translation(mem8_loc, 1, 0);
    tlb_hash = tlb_writable_cplX[mem8_loc >> 12];
    phys_mem8[mem8_loc ^ tlb_hash] = byte;
}
void Free86::st8_writable_cplX(int byte) {
    tlb_hash = tlb_writable_cplX[mem8_loc >> 12];
    if (tlb_hash == -1) {
        _st8_writable_cplX(byte);
    } else {
        phys_mem8[mem8_loc ^ tlb_hash] = byte;
    }
}
void Free86::_st16_writable_cplX(int word) {
    st8_writable_cplX(word);
    mem8_loc++;
    st8_writable_cplX(word >> 8);
    mem8_loc--;
}
void Free86::st16_writable_cplX(int word) {
    tlb_hash = tlb_writable_cplX[mem8_loc >> 12];
    if ((tlb_hash | mem8_loc) & 1) {
        _st16_writable_cplX(word);
    } else {
        phys_mem16[(mem8_loc ^ tlb_hash) >> 1] = word;
    }
}
void Free86::_st32_writable_cplX(int dword) {
    st8_writable_cplX(dword);
    mem8_loc++;
    st8_writable_cplX(dword >> 8);
    mem8_loc++;
    st8_writable_cplX(dword >> 16);
    mem8_loc++;
    st8_writable_cplX(dword >> 24);
    mem8_loc -= 3;
}
void Free86::st32_writable_cplX(int dword) {
    tlb_hash = tlb_writable_cplX[mem8_loc >> 12];
    if ((tlb_hash | mem8_loc) & 3) {
        _st32_writable_cplX(dword);
    } else {
        phys_mem32[(mem8_loc ^ tlb_hash) >> 2] = dword;
    }
}
void Free86::_st8_writable_cpl3(int byte) {
    if (is_protected()) {
        page_translation(mem8_loc, 1, cpl == 3);
        tlb_hash = tlb_writable[mem8_loc >> 12];
        phys_mem8[mem8_loc ^ tlb_hash] = byte;
    } else {
        phys_mem8[mem8_loc] = byte;
    }
}
void Free86::st8_writable_cpl3(int byte) {
    tlb_hash = tlb_writable[mem8_loc >> 12];
    if (is_real__v86() || tlb_hash == -1) {
        _st8_writable_cpl3(byte);
    } else {
        phys_mem8[mem8_loc ^ tlb_hash] = byte;
    }
}
void Free86::_st16_writable_cpl3(int word) {
    st8_writable_cpl3(word);
    mem8_loc++;
    st8_writable_cpl3(word >> 8);
    mem8_loc--;
}
void Free86::st16_writable_cpl3(int word) {
    tlb_hash = tlb_writable[mem8_loc >> 12];
    if (is_real__v86() || (tlb_hash | mem8_loc) & 1) {
        _st16_writable_cpl3(word);
    } else {
        phys_mem16[(mem8_loc ^ tlb_hash) >> 1] = word;
    }
}
void Free86::_st32_writable_cpl3(int dword) {
    st8_writable_cpl3(dword);
    mem8_loc++;
    st8_writable_cpl3(dword >> 8);
    mem8_loc++;
    st8_writable_cpl3(dword >> 16);
    mem8_loc++;
    st8_writable_cpl3(dword >> 24);
    mem8_loc -= 3;
}
void Free86::st32_writable_cpl3(int dword) {
    tlb_hash = tlb_writable[mem8_loc >> 12];
    if (is_real__v86() || (tlb_hash | mem8_loc) & 3) {
        _st32_writable_cpl3(dword);
    } else {
        phys_mem32[(mem8_loc ^ tlb_hash) >> 2] = dword;
    }
}
int Free86::ld8_direct() {
    return phys_mem8[far++];
}
int Free86::ld16_direct() {
    return ld8_direct() | (ld8_direct() << 8);
}
int Free86::ld32_direct() {
    return ld8_direct() | (ld8_direct() << 8) | (ld8_direct() << 16) | (ld8_direct() << 24);
}
void Free86::push_word(int word) {
    int esp = regs[4] - 2;
    mem8_loc = (esp & SS_mask) + SS_base;
    st16_writable_cpl3(word);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
}
void Free86::push_dword(int dword) {
    int esp = regs[4] - 4;
    mem8_loc = (esp & SS_mask) + SS_base;
    st32_writable_cpl3(dword);
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
    return ld16_readonly_cpl3();
}
int Free86::read_stack_dword() {
    mem8_loc = (regs[4] & SS_mask) + SS_base;
    return ld32_readonly_cpl3();
}
