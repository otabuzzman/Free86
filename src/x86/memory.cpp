#include "free86.h"

int Free86::_ld8_readonly_cplX() {
    page_translation(address_operand, 0, 0);
    tlb_hash = tlb_readonly_cplX[address_operand >> 12];
    return memory8[address_operand ^ tlb_hash];
}
int Free86::ld8_readonly_cplX() {
    return ((tlb_hash = tlb_readonly_cplX[address_operand >> 12]) == -1)
               ? _ld8_readonly_cplX()
               : memory8[address_operand ^ tlb_hash];
}
int Free86::_ld16_readonly_cplX() {
    int word = ld8_readonly_cplX();
    address_operand++;
    word |= ld8_readonly_cplX() << 8;
    address_operand--;
    return word;
}
int Free86::ld16_readonly_cplX() {
    return ((tlb_hash = tlb_readonly_cplX[address_operand >> 12]) | address_operand) & 1
               ? _ld16_readonly_cplX()
               : memory16[(address_operand ^ tlb_hash) >> 1];
}
int Free86::_ld_readonly_cplX() {
    int dword = ld8_readonly_cplX();
    address_operand++;
    dword |= ld8_readonly_cplX() << 8;
    address_operand++;
    dword |= ld8_readonly_cplX() << 16;
    address_operand++;
    dword |= ld8_readonly_cplX() << 24;
    address_operand -= 3;
    return dword;
}
int Free86::ld_readonly_cplX() {
    return ((tlb_hash = tlb_readonly_cplX[address_operand >> 12]) | address_operand) & 3
               ? _ld_readonly_cplX()
               : memory[(address_operand ^ tlb_hash) >> 2];
}
int Free86::_ld8_readonly_cpl3() {
    int byte;
    if (is_protected()) {
        page_translation(address_operand, 0, cpl == 3);
        tlb_hash = tlb_readonly[address_operand >> 12];
        byte = memory8[address_operand ^ tlb_hash];
    } else {
        byte = memory8[address_operand];
    }
    return byte;
}
int Free86::ld8_readonly_cpl3() {
    return (is_real__v86() ||
                    ((tlb_hash = tlb_readonly[address_operand >> 12]) == -1)
                ? _ld8_readonly_cpl3()
                : memory8[address_operand ^ tlb_hash]);
}
int Free86::_ld16_readonly_cpl3() {
    int word = ld8_readonly_cpl3();
    address_operand++;
    word |= ld8_readonly_cpl3() << 8;
    address_operand--;
    return word;
}
int Free86::ld16_readonly_cpl3() {
    return (is_real__v86() ||
                    ((tlb_hash = tlb_readonly[address_operand >> 12]) | address_operand) & 1
                ? _ld16_readonly_cpl3()
                : memory16[(address_operand ^ tlb_hash) >> 1]);
}
int Free86::_ld_readonly_cpl3() {
    int dword = ld8_readonly_cpl3();
    address_operand++;
    dword |= ld8_readonly_cpl3() << 8;
    address_operand++;
    dword |= ld8_readonly_cpl3() << 16;
    address_operand++;
    dword |= ld8_readonly_cpl3() << 24;
    address_operand -= 3;
    return dword;
}
int Free86::ld_readonly_cpl3() {
    return (is_real__v86() ||
                    ((tlb_hash = tlb_readonly[address_operand >> 12]) | address_operand) & 3
                ? _ld_readonly_cpl3()
                : memory[(address_operand ^ tlb_hash) >> 2]);
}
int Free86::_ld8_writable_cpl3() {
    int byte;
    if (is_protected()) {
        page_translation(address_operand, 1, cpl == 3);
        tlb_hash = tlb_writable[address_operand >> 12];
        byte = memory8[address_operand ^ tlb_hash];
    } else {
        byte = memory8[address_operand];
    }
    return byte;
}
int Free86::ld8_writable_cpl3() {
    return (is_real__v86() ||
                    (tlb_hash = tlb_writable[address_operand >> 12]) == -1)
                ? _ld8_writable_cpl3()
                : memory8[address_operand ^ tlb_hash];
}
int Free86::_ld16_writable_cpl3() {
    int word = ld8_writable_cpl3();
    address_operand++;
    word |= ld8_writable_cpl3() << 8;
    address_operand--;
    return word;
}
int Free86::ld16_writable_cpl3() {
    return (is_real__v86() ||
                    (tlb_hash = tlb_writable[address_operand >> 12]) | address_operand) & 1
                ? _ld16_writable_cpl3()
                : memory16[(address_operand ^ tlb_hash) >> 1];
}
int Free86::_ld_writable_cpl3() {
    int dword = ld8_writable_cpl3();
    address_operand++;
    dword |= ld8_writable_cpl3() << 8;
    address_operand++;
    dword |= ld8_writable_cpl3() << 16;
    address_operand++;
    dword |= ld8_writable_cpl3() << 24;
    address_operand -= 3;
    return dword;
}
int Free86::ld_writable_cpl3() {
    return (is_real__v86() ||
                    (tlb_hash = tlb_writable[address_operand >> 12]) | address_operand) & 3
               ? _ld_writable_cpl3()
               : memory[(address_operand ^ tlb_hash) >> 2];
}
void Free86::_st8_writable_cplX(int byte) {
    page_translation(address_operand, 1, 0);
    tlb_hash = tlb_writable_cplX[address_operand >> 12];
    memory8[address_operand ^ tlb_hash] = byte;
}
void Free86::st8_writable_cplX(int byte) {
    tlb_hash = tlb_writable_cplX[address_operand >> 12];
    if (tlb_hash == -1) {
        _st8_writable_cplX(byte);
    } else {
        memory8[address_operand ^ tlb_hash] = byte;
    }
}
void Free86::_st16_writable_cplX(int word) {
    st8_writable_cplX(word);
    address_operand++;
    st8_writable_cplX(word >> 8);
    address_operand--;
}
void Free86::st16_writable_cplX(int word) {
    tlb_hash = tlb_writable_cplX[address_operand >> 12];
    if ((tlb_hash | address_operand) & 1) {
        _st16_writable_cplX(word);
    } else {
        memory16[(address_operand ^ tlb_hash) >> 1] = word;
    }
}
void Free86::_st_writable_cplX(int dword) {
    st8_writable_cplX(dword);
    address_operand++;
    st8_writable_cplX(dword >> 8);
    address_operand++;
    st8_writable_cplX(dword >> 16);
    address_operand++;
    st8_writable_cplX(dword >> 24);
    address_operand -= 3;
}
void Free86::st_writable_cplX(int dword) {
    tlb_hash = tlb_writable_cplX[address_operand >> 12];
    if ((tlb_hash | address_operand) & 3) {
        _st_writable_cplX(dword);
    } else {
        memory[(address_operand ^ tlb_hash) >> 2] = dword;
    }
}
void Free86::_st8_writable_cpl3(int byte) {
    if (is_protected()) {
        page_translation(address_operand, 1, cpl == 3);
        tlb_hash = tlb_writable[address_operand >> 12];
        memory8[address_operand ^ tlb_hash] = byte;
    } else {
        memory8[address_operand] = byte;
    }
}
void Free86::st8_writable_cpl3(int byte) {
    tlb_hash = tlb_writable[address_operand >> 12];
    if (is_real__v86() || tlb_hash == -1) {
        _st8_writable_cpl3(byte);
    } else {
        memory8[address_operand ^ tlb_hash] = byte;
    }
}
void Free86::_st16_writable_cpl3(int word) {
    st8_writable_cpl3(word);
    address_operand++;
    st8_writable_cpl3(word >> 8);
    address_operand--;
}
void Free86::st16_writable_cpl3(int word) {
    tlb_hash = tlb_writable[address_operand >> 12];
    if (is_real__v86() || (tlb_hash | address_operand) & 1) {
        _st16_writable_cpl3(word);
    } else {
        memory16[(address_operand ^ tlb_hash) >> 1] = word;
    }
}
void Free86::_st_writable_cpl3(int dword) {
    st8_writable_cpl3(dword);
    address_operand++;
    st8_writable_cpl3(dword >> 8);
    address_operand++;
    st8_writable_cpl3(dword >> 16);
    address_operand++;
    st8_writable_cpl3(dword >> 24);
    address_operand -= 3;
}
void Free86::st_writable_cpl3(int dword) {
    tlb_hash = tlb_writable[address_operand >> 12];
    if (is_real__v86() || (tlb_hash | address_operand) & 3) {
        _st_writable_cpl3(dword);
    } else {
        memory[(address_operand ^ tlb_hash) >> 2] = dword;
    }
}
int Free86::ld8_direct() {
    return memory8[far++];
}
int Free86::ld16_direct() {
    return ld8_direct() | (ld8_direct() << 8);
}
int Free86::ld_direct() {
    return ld8_direct() | (ld8_direct() << 8) | (ld8_direct() << 16) | (ld8_direct() << 24);
}
int Free86::ld8_direct(int address) {
    return memory8[address];
}
int Free86::ld_direct(int address) {
    return memory[address >> 2];
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
    memory[address >> 2] = dword;
}
void Free86::push_word(int word) {
    int esp = regs[4] - 2;
    address_operand = (esp & SS_mask) + SS_base;
    st16_writable_cpl3(word);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
}
void Free86::push_dword(int dword) {
    int esp = regs[4] - 4;
    address_operand = (esp & SS_mask) + SS_base;
    st_writable_cpl3(dword);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
}
void Free86::pop_word() {
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 2) & SS_mask);
}
void Free86::pop_dword() {
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 4) & SS_mask);
}
int Free86::read_stack_word() {
    address_operand = (regs[4] & SS_mask) + SS_base;
    return ld16_readonly_cpl3();
}
int Free86::read_stack_dword() {
    address_operand = (regs[4] & SS_mask) + SS_base;
    return ld_readonly_cpl3();
}
