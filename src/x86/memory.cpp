#include "free86.h"

int Free86::_ld8_readonly_cplX() {
    page_translation(0, 0);
    tlb_hash = tlb_readonly_cplX[lax >> 12];
    return memory8[lax ^ tlb_hash];
}
int Free86::ld8_readonly_cplX() {
    return ((tlb_hash = tlb_readonly_cplX[lax >> 12]) == -1)
                ? _ld8_readonly_cplX()
                : memory8[lax ^ tlb_hash];
}
int Free86::_ld16_readonly_cplX() {
    int word = ld8_readonly_cplX();
    lax++;
    word |= ld8_readonly_cplX() << 8;
    lax--;
    return word;
}
int Free86::ld16_readonly_cplX() {
    return ((tlb_hash = tlb_readonly_cplX[lax >> 12]) | lax) & 1
                ? _ld16_readonly_cplX()
                : memory16[(lax ^ tlb_hash) >> 1];
}
int Free86::_ld_readonly_cplX() {
    int dword = ld8_readonly_cplX();
    lax++;
    dword |= ld8_readonly_cplX() << 8;
    lax++;
    dword |= ld8_readonly_cplX() << 16;
    lax++;
    dword |= ld8_readonly_cplX() << 24;
    lax -= 3;
    return dword;
}
int Free86::ld_readonly_cplX() {
    return ((tlb_hash = tlb_readonly_cplX[lax >> 12]) | lax) & 3
                ? _ld_readonly_cplX()
                : memory[(lax ^ tlb_hash) >> 2];
}
int Free86::_ld8_readonly_cpl3() {
    int byte;
    if (is_protected()) {
        page_translation(0, cpl == 3);
        tlb_hash = tlb_readonly[lax >> 12];
        byte = memory8[lax ^ tlb_hash];
    } else {
        byte = memory8[lax];
    }
    return byte;
}
int Free86::ld8_readonly_cpl3() {
    return (is_real__v86() ||
                    ((tlb_hash = tlb_readonly[lax >> 12]) == -1)
                ? _ld8_readonly_cpl3()
                : memory8[lax ^ tlb_hash]);
}
int Free86::_ld16_readonly_cpl3() {
    int word = ld8_readonly_cpl3();
    lax++;
    word |= ld8_readonly_cpl3() << 8;
    lax--;
    return word;
}
int Free86::ld16_readonly_cpl3() {
    return (is_real__v86() ||
                    ((tlb_hash = tlb_readonly[lax >> 12]) | lax) & 1
                ? _ld16_readonly_cpl3()
                : memory16[(lax ^ tlb_hash) >> 1]);
}
int Free86::_ld_readonly_cpl3() {
    int dword = ld8_readonly_cpl3();
    lax++;
    dword |= ld8_readonly_cpl3() << 8;
    lax++;
    dword |= ld8_readonly_cpl3() << 16;
    lax++;
    dword |= ld8_readonly_cpl3() << 24;
    lax -= 3;
    return dword;
}
int Free86::ld_readonly_cpl3() {
    return (is_real__v86() ||
                    ((tlb_hash = tlb_readonly[lax >> 12]) | lax) & 3
                ? _ld_readonly_cpl3()
                : memory[(lax ^ tlb_hash) >> 2]);
}
int Free86::_ld8_writable_cpl3() {
    int byte;
    if (is_protected()) {
        page_translation(1, cpl == 3);
        tlb_hash = tlb_writable[lax >> 12];
        byte = memory8[lax ^ tlb_hash];
    } else {
        byte = memory8[lax];
    }
    return byte;
}
int Free86::ld8_writable_cpl3() {
    return (is_real__v86() ||
                    (tlb_hash = tlb_writable[lax >> 12]) == -1)
                ? _ld8_writable_cpl3()
                : memory8[lax ^ tlb_hash];
}
int Free86::_ld16_writable_cpl3() {
    int word = ld8_writable_cpl3();
    lax++;
    word |= ld8_writable_cpl3() << 8;
    lax--;
    return word;
}
int Free86::ld16_writable_cpl3() {
    return (is_real__v86() ||
                    (tlb_hash = tlb_writable[lax >> 12]) | lax) & 1
                ? _ld16_writable_cpl3()
                : memory16[(lax ^ tlb_hash) >> 1];
}
int Free86::_ld_writable_cpl3() {
    int dword = ld8_writable_cpl3();
    lax++;
    dword |= ld8_writable_cpl3() << 8;
    lax++;
    dword |= ld8_writable_cpl3() << 16;
    lax++;
    dword |= ld8_writable_cpl3() << 24;
    lax -= 3;
    return dword;
}
int Free86::ld_writable_cpl3() {
    return (is_real__v86() ||
                    (tlb_hash = tlb_writable[lax >> 12]) | lax) & 3
                ? _ld_writable_cpl3()
                : memory[(lax ^ tlb_hash) >> 2];
}
void Free86::_st8_writable_cplX(int byte) {
    page_translation(1, 0);
    tlb_hash = tlb_writable_cplX[lax >> 12];
    memory8[lax ^ tlb_hash] = byte;
}
void Free86::st8_writable_cplX(int byte) {
    tlb_hash = tlb_writable_cplX[lax >> 12];
    if (tlb_hash == -1) {
        _st8_writable_cplX(byte);
    } else {
        memory8[lax ^ tlb_hash] = byte;
    }
}
void Free86::_st16_writable_cplX(int word) {
    st8_writable_cplX(word);
    lax++;
    st8_writable_cplX(word >> 8);
    lax--;
}
void Free86::st16_writable_cplX(int word) {
    tlb_hash = tlb_writable_cplX[lax >> 12];
    if ((tlb_hash | lax) & 1) {
        _st16_writable_cplX(word);
    } else {
        memory16[(lax ^ tlb_hash) >> 1] = word;
    }
}
void Free86::_st_writable_cplX(int dword) {
    st8_writable_cplX(dword);
    lax++;
    st8_writable_cplX(dword >> 8);
    lax++;
    st8_writable_cplX(dword >> 16);
    lax++;
    st8_writable_cplX(dword >> 24);
    lax -= 3;
}
void Free86::st_writable_cplX(int dword) {
    tlb_hash = tlb_writable_cplX[lax >> 12];
    if ((tlb_hash | lax) & 3) {
        _st_writable_cplX(dword);
    } else {
        memory[(lax ^ tlb_hash) >> 2] = dword;
    }
}
void Free86::_st8_writable_cpl3(int byte) {
    if (is_protected()) {
        page_translation(1, cpl == 3);
        tlb_hash = tlb_writable[lax >> 12];
        memory8[lax ^ tlb_hash] = byte;
    } else {
        memory8[lax] = byte;
    }
}
void Free86::st8_writable_cpl3(int byte) {
    tlb_hash = tlb_writable[lax >> 12];
    if (is_real__v86() || tlb_hash == -1) {
        _st8_writable_cpl3(byte);
    } else {
        memory8[lax ^ tlb_hash] = byte;
    }
}
void Free86::_st16_writable_cpl3(int word) {
    st8_writable_cpl3(word);
    lax++;
    st8_writable_cpl3(word >> 8);
    lax--;
}
void Free86::st16_writable_cpl3(int word) {
    tlb_hash = tlb_writable[lax >> 12];
    if (is_real__v86() || (tlb_hash | lax) & 1) {
        _st16_writable_cpl3(word);
    } else {
        memory16[(lax ^ tlb_hash) >> 1] = word;
    }
}
void Free86::_st_writable_cpl3(int dword) {
    st8_writable_cpl3(dword);
    lax++;
    st8_writable_cpl3(dword >> 8);
    lax++;
    st8_writable_cpl3(dword >> 16);
    lax++;
    st8_writable_cpl3(dword >> 24);
    lax -= 3;
}
void Free86::st_writable_cpl3(int dword) {
    tlb_hash = tlb_writable[lax >> 12];
    if (is_real__v86() || (tlb_hash | lax) & 3) {
        _st_writable_cpl3(dword);
    } else {
        memory[(lax ^ tlb_hash) >> 2] = dword;
    }
}
int Free86::fetch8() {
    return memory8[far++];
}
int Free86::fetch16() {
    return fetch8() | (fetch8() << 8);
}
int Free86::fetch() {
    return fetch8() | (fetch8() << 8) | (fetch8() << 16) | (fetch8() << 24);
}
int Free86::ld8_direct(int address) {
    return memory8[address];
}
int Free86::ld_direct(int address) {
    return memory8[address] | (memory8[address + 1] << 8) | (memory8[address + 2] << 16) | (memory8[address + 3] << 24);
}
void Free86::st8_direct(int address, int byte) {
    memory8[address] = byte;
}
void Free86::st8_direct(int address, std::string data) {
    auto s = data.c_str();
    auto l = data.length();
    for (int i = 0; i < l; i++) {
        st8_direct(address++, s[i] & 0xff);
    }
    st8_direct(address, 0);
}
void Free86::st_direct(int address, int dword) {
    memory8[address] = dword;
    memory8[address + 1] = dword >> 8;
    memory8[address + 2] = dword >> 16;
    memory8[address + 3] = dword >> 24;
}
void Free86::push16(int word) {
    int esp = regs[4] - 2;
    lax = (esp & SS_mask) + SS_base;
    st16_writable_cpl3(word);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
}
void Free86::push(int dword) {
    int esp = regs[4] - 4;
    lax = (esp & SS_mask) + SS_base;
    st_writable_cpl3(dword);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
}
int Free86::pop16() {
    int x = ld16_stack();
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 2) & SS_mask);
    return x;
}
int Free86::pop() {
    int x = ld_stack();
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 4) & SS_mask);
    return x;
}
int Free86::ld16_stack() {
    lax = (regs[4] & SS_mask) + SS_base;
    return ld16_readonly_cpl3();
}
int Free86::ld_stack() {
    lax = (regs[4] & SS_mask) + SS_base;
    return ld_readonly_cpl3();
}
