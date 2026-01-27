#include "free86.h"

int Free86::ld8_readonly_cplX() {
    tlb_hash = tlb_readonly_cplX[lax >> 12];
    if (tlb_hash == -1) {
        page_translation(0, 0);
        tlb_hash = tlb_readonly_cplX[lax >> 12];
    }
    return ld8_direct(lax ^ tlb_hash);
}
int Free86::ld16_readonly_cplX() {
    tlb_hash = tlb_readonly_cplX[lax >> 12];
    if ((tlb_hash | lax) & 1) {
        int word = ld8_readonly_cplX();
        lax++;
        word |= ld8_readonly_cplX() << 8;
        lax--;
        return word;
    }
    return ld16_direct(lax ^ tlb_hash);
}
int Free86::ld_readonly_cplX() {
    tlb_hash = tlb_readonly_cplX[lax >> 12];
    if ((tlb_hash | lax) & 3) {
        int dword = ld16_readonly_cplX();
        lax += 2;
        dword |= ld16_readonly_cplX() << 16;
        lax -= 2;
        return dword;
    }
    return ld_direct(lax ^ tlb_hash);
}
int Free86::ld8_readonly_cpl3() {
    if (is_real__v86()) {
        return ld8_direct(lax);
    }
    tlb_hash = tlb_readonly[lax >> 12];
    if (tlb_hash == -1) {
        page_translation(0, cpl == 3);
        tlb_hash = tlb_readonly[lax >> 12];
    }
    return ld8_direct(lax ^ tlb_hash);
}
int Free86::ld16_readonly_cpl3() {
    if (is_real__v86()) {
        return ld16_direct(lax);
    }
    tlb_hash = tlb_readonly[lax >> 12];
    if ((tlb_hash | lax) & 1) {
        int word = ld8_readonly_cpl3();
        lax++;
        word |= ld8_readonly_cpl3() << 8;
        lax--;
        return word;
    }
    return ld16_direct(lax ^ tlb_hash);
}
int Free86::ld_readonly_cpl3() {
    if (is_real__v86()) {
        return ld_direct(lax);
    }
    tlb_hash = tlb_readonly[lax >> 12];
        if ((tlb_hash | lax) & 3) {
        int dword = ld16_readonly_cpl3();
        lax += 2;
        dword |= ld16_readonly_cpl3() << 16;
        lax -= 2;
        return dword;
    }
    return ld_direct(lax ^ tlb_hash);
}
int Free86::ld8_writable_cpl3() {
    if (is_real__v86()) {
        return ld8_direct(lax);
    }
    tlb_hash = tlb_writable[lax >> 12];
    if (tlb_hash == -1) {
        page_translation(1, cpl == 3);
        tlb_hash = tlb_writable[lax >> 12];
    }
    return ld8_direct(lax ^ tlb_hash);
}
int Free86::ld16_writable_cpl3() {
    if (is_real__v86()) {
        return ld16_direct(lax);
    }
    tlb_hash = tlb_writable[lax >> 12];
    if ((tlb_hash | lax) & 1) {
        int word = ld8_writable_cpl3();
        lax++;
        word |= ld8_writable_cpl3() << 8;
        lax--;
        return word;
    }
    return ld16_direct(lax ^ tlb_hash);
}
int Free86::ld_writable_cpl3() {
    if (is_real__v86()) {
        return ld_direct(lax);
    }
    tlb_hash = tlb_writable[lax >> 12];
        if ((tlb_hash | lax) & 3) {
        int dword = ld16_writable_cpl3();
        lax += 2;
        dword |= ld16_writable_cpl3() << 16;
        lax -= 2;
        return dword;
    }
    return ld_direct(lax ^ tlb_hash);
}
void Free86::st8_writable_cplX(int byte) {
    tlb_hash = tlb_writable_cplX[lax >> 12];
    if (tlb_hash == -1) {
        page_translation(1, 0);
        tlb_hash = tlb_writable_cplX[lax >> 12];
    }
    st8_direct(lax ^ tlb_hash, byte);
}
void Free86::st16_writable_cplX(int word) {
    tlb_hash = tlb_writable_cplX[lax >> 12];
    if ((tlb_hash | lax) & 1) {
         st8_writable_cplX(word);
        lax++;
        st8_writable_cplX(word >> 8);
        lax--;
    } else {
        st16_direct(lax ^ tlb_hash, word);
    }
}
void Free86::st_writable_cplX(int dword) {
    tlb_hash = tlb_writable_cplX[lax >> 12];
    if ((tlb_hash | lax) & 3) {
        st16_writable_cplX(dword);
        lax += 2;
        st16_writable_cplX(dword >> 16);
        lax -= 2;
    } else {
        st_direct(lax ^ tlb_hash, dword);
    }
}
void Free86::st8_writable_cpl3(int byte) {
    if (is_real__v86()) {
        st8_direct(lax, byte);
    } 
    tlb_hash = tlb_writable[lax >> 12];
    if (tlb_hash == -1) {
        page_translation(1, cpl == 3);
        tlb_hash = tlb_writable[lax >> 12];
    }
    st8_direct(lax ^ tlb_hash, byte);
}
void Free86::st16_writable_cpl3(int word) {
    if (is_real__v86()) {
        st16_direct(lax, word);
    }
    tlb_hash = tlb_writable[lax >> 12];
    if ((tlb_hash | lax) & 1) {
        st8_writable_cpl3(word);
        lax++;
        st8_writable_cpl3(word >> 8);
        lax--;
    } else {
        st16_direct(lax ^ tlb_hash, word);
    }
}
void Free86::st_writable_cpl3(int dword) {
    if (is_real__v86()) {
        st_direct(lax, dword);
    }
    tlb_hash = tlb_writable[lax >> 12];
    if ((tlb_hash | lax) & 3) {
        st16_writable_cpl3(dword);
        lax += 2;
        st16_writable_cpl3(dword >> 16);
        lax -= 2;
    } else {
        st_direct(lax ^ tlb_hash, dword);
    }
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
void Free86::st16_direct(int address, int word) {
    memory[address] = word;
    memory[address + 1] = word >> 8;
}
void Free86::st_direct(int address, int dword) {
    memory[address] = dword;
    memory[address + 1] = dword >> 8;
    memory[address + 2] = dword >> 16;
    memory[address + 3] = dword >> 24;
}
int Free86::fetch_data8() {
    return ld8_direct(far++);
}
int Free86::fetch_data16() {
    int word = fetch_data8();
    word |= fetch_data8() << 8;
    return word;
}
int Free86::fetch_data() {
    int dword = fetch_data8();
    dword |= fetch_data8() << 8;
    dword |= fetch_data8() << 16;
    dword |= fetch_data8() << 24;
    return dword;
}
void Free86::push16(int word) {
    int esp = regs[4] - 2;
    lax = SS_base + (esp & SS_mask);
    st16_writable_cpl3(word);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
}
void Free86::push(int dword) {
    int esp = regs[4] - 4;
    lax = SS_base + (esp & SS_mask);
    st_writable_cpl3(dword);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
}
int Free86::pop16() {
    int res = ld16_stack();
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 2) & SS_mask);
    return res;
}
int Free86::pop() {
    int res = ld_stack();
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 4) & SS_mask);
    return res;
}
int Free86::ld16_stack() {
    lax = SS_base + (regs[4] & SS_mask);
    return ld16_readonly_cpl3();
}
int Free86::ld_stack() {
    lax = SS_base + (regs[4] & SS_mask);
    return ld_readonly_cpl3();
}
