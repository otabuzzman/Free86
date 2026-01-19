#include "free86.h"

int Free86::_ld8_readonly_cplX() {
    page_translation(0, 0);
    tlb_hash = tlb_readonly_cplX[lax >> 12];
    return ld8_direct(lax ^ tlb_hash);
}
int Free86::ld8_readonly_cplX() {
    return ((tlb_hash = tlb_readonly_cplX[lax >> 12]) == -1)
                ? _ld8_readonly_cplX()
                : ld8_direct(lax ^ tlb_hash);
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
                : ld16_direct(lax ^ tlb_hash);
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
    page_translation(0, cpl == 3);
    tlb_hash = tlb_readonly[lax >> 12];
    return ld8_direct(lax ^ tlb_hash);
}
int Free86::ld8_readonly_cpl3() {
    return (is_real__v86() ? ld8_direct(lax) :
                    ((tlb_hash = tlb_readonly[lax >> 12]) == -1)
                ? _ld8_readonly_cpl3()
                : ld8_direct(lax ^ tlb_hash);
}
int Free86::_ld16_readonly_cpl3() {
    int word = ld8_readonly_cpl3();
    lax++;
    word |= ld8_readonly_cpl3() << 8;
    lax--;
    return word;
}
int Free86::ld16_readonly_cpl3() {
    return (is_real__v86() ? ld16_direct(lax) :
                    ((tlb_hash = tlb_readonly[lax >> 12]) | lax) & 1
                ? _ld16_readonly_cpl3()
                : ld16_direct(lax ^ tlb_hash);
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
    return (is_real__v86() ? ld_direct(lax) :
                    ((tlb_hash = tlb_readonly[lax >> 12]) | lax) & 3
                ? _ld_readonly_cpl3()
                : memory[(lax ^ tlb_hash) >> 2]);
}
int Free86::_ld8_writable_cpl3() {
    page_translation(1, cpl == 3);
    tlb_hash = tlb_writable[lax >> 12];
    return ld8_direct(lax ^ tlb_hash);
}
int Free86::ld8_writable_cpl3() {
    return (is_real__v86() ? ld8_direct(lax) :
                    (tlb_hash = tlb_writable[lax >> 12]) == -1)
                ? _ld8_writable_cpl3()
                : ld8_direct(lax ^ tlb_hash);
}
int Free86::_ld16_writable_cpl3() {
    int word = ld8_writable_cpl3();
    lax++;
    word |= ld8_writable_cpl3() << 8;
    lax--;
    return word;
}
int Free86::ld16_writable_cpl3() {
    return (is_real__v86() ? ld16_direct(lax) :
                    (tlb_hash = tlb_writable[lax >> 12]) | lax) & 1
                ? _ld16_writable_cpl3()
                : ld16_direct(lax ^ tlb_hash);
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
    return (is_real__v86() ? ld_direct(lax) :
                    (tlb_hash = tlb_writable[lax >> 12]) | lax) & 3
                ? _ld_writable_cpl3()
                : memory[(lax ^ tlb_hash) >> 2];
}
void Free86::_st8_writable_cplX(int byte) {
    page_translation(1, 0);
    tlb_hash = tlb_writable_cplX[lax >> 12];
    st8_direct(lax ^ tlb_hash, byte);
}
void Free86::st8_writable_cplX(int byte) {
    if ((tlb_hash = tlb_writable_cplX[lax >> 12]) == -1) {
        _st8_writable_cplX(byte);
    } else {
        st8_direct(lax ^ tlb_hash, byte);
    }
}
void Free86::_st16_writable_cplX(int word) {
    st8_writable_cplX(word);
    lax++;
    st8_writable_cplX(word >> 8);
    lax--;
}
void Free86::st16_writable_cplX(int word) {
    if (((tlb_hash = tlb_writable_cplX[lax >> 12]) | lax) & 1) {
        _st16_writable_cplX(word);
    } else {
        st16_direct(lax ^ tlb_hash, word);
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
    if (((tlb_hash = tlb_writable_cplX[lax >> 12]) | lax) & 3) {
        _st_writable_cplX(dword);
    } else {
        memory[(lax ^ tlb_hash) >> 2] = dword;
    }
}
void Free86::_st8_writable_cpl3(int byte) {
    page_translation(1, cpl == 3);
    tlb_hash = tlb_writable[lax >> 12];
    st8_direct(lax ^ tlb_hash, byte);
}
void Free86::st8_writable_cpl3(int byte) {
    if (is_real__v86()) {
        st8_direct(lax, byte);
    } else if ((tlb_hash = tlb_writable[lax >> 12]) == -1) {
        _st8_writable_cpl3(byte);
    } else {
        st8_direct(lax ^ tlb_hash, byte);
    }
}
void Free86::_st16_writable_cpl3(int word) {
    st8_writable_cpl3(word);
    lax++;
    st8_writable_cpl3(word >> 8);
    lax--;
}
void Free86::st16_writable_cpl3(int word) {
    if (is_real__v86()) {
        st16_direct(lax, word);
    } else if (((tlb_hash = tlb_writable[lax >> 12]) | lax) & 1) {
        _st16_writable_cpl3(word);
    } else {
        st16_direct(lax ^ tlb_hash, word);
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
    if (is_real__v86()) {
        st_direct(lax, dword);
    } else if (((tlb_hash = tlb_writable[lax >> 12]) | lax) & 3) {
        _st_writable_cpl3(dword);
    } else {
        memory[(lax ^ tlb_hash) >> 2] = dword;
    }
}
int Free86::fetch8() {
    return ld8_direct(far++);
}
int Free86::fetch16() {
    int word = fetch8();
    word |= fetch8() << 8;
    return word;
}
int Free86::fetch() {
    int dword = fetch8();
    dword |= fetch8() << 8;
    dword |= fetch8() << 16;
    dword |= fetch8() << 24;
    return dword;
}
int Free86::ld8_direct(int address) {
    return memory[address];
}
int Free86::ld16_direct(int address) {
    return memory[address] | (memory[address + 1] << 8);
}
int Free86::ld_direct(int address) {
    return memory[address] | (memory[address + 1] << 8) | (memory[address + 2] << 16) | (memory[address + 3] << 24);
}
void Free86::st8_direct(int address, int byte) {
    memory[address] = byte;
}
void Free86::st8_direct(int address, std::string data) {
    auto s = data.c_str();
    auto l = data.length();
    for (int i = 0; i < l; i++) {
        st8_direct(address++, s[i] & 0xff);
    }
    st8_direct(address, 0);
}
void Free86::st16_direct(int address, int dword) {
    memory[address] = dword;
    memory[address + 1] = dword >> 8;
}
void Free86::st_direct(int address, int dword) {
    memory[address] = dword;
    memory[address + 1] = dword >> 8;
    memory[address + 2] = dword >> 16;
    memory[address + 3] = dword >> 24;
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
