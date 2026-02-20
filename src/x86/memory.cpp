#include "free86.h"

uint32_t Free86::ld8_readonly_cplX() {
    tlb_hash = tlb_readonly_cplX[lax >> 12];
    if (tlb_hash == 0xffffffff) {
        page_translation(0, 0);
        tlb_hash = tlb_readonly_cplX[lax >> 12];
    }
    return ld8_direct(lax ^ tlb_hash);
}
uint32_t Free86::ld16_readonly_cplX() {
    tlb_hash = tlb_readonly_cplX[lax >> 12];
    if ((tlb_hash | lax) & 1) {
        uint32_t word = ld8_readonly_cplX();
        lax++;
        word |= ld8_readonly_cplX() << 8;
        lax--;
        return word;
    }
    return ld16_direct(lax ^ tlb_hash);
}
uint32_t Free86::ld_readonly_cplX() {
    tlb_hash = tlb_readonly_cplX[lax >> 12];
    if ((tlb_hash | lax) & 3) {
        uint32_t dword = ld16_readonly_cplX();
        lax += 2;
        dword |= ld16_readonly_cplX() << 16;
        lax -= 2;
        return dword;
    }
    return ld_direct(lax ^ tlb_hash);
}
uint64_t Free86::ld64_readonly_cplX() {
    tlb_hash = tlb_readonly_cplX[lax >> 12];
    if ((tlb_hash | lax) & 3) {
        uint64_t qword = ld_readonly_cplX() & 0xffffffff;
        lax += 4;
        qword |= static_cast<uint64_t>(ld_readonly_cplX()) << 32;
        lax -= 4;
        return qword;
    }
    return ld64_direct(lax ^ tlb_hash);
}
uint32_t Free86::ld8_readonly_cpl3() {
    if (is_real__v86()) {
        return ld8_direct(lax);
    }
    tlb_hash = tlb_readonly[lax >> 12];
    if (tlb_hash == 0xffffffff) {
        page_translation(0, cpl == 3);
        tlb_hash = tlb_readonly[lax >> 12];
    }
    return ld8_direct(lax ^ tlb_hash);
}
uint32_t Free86::ld16_readonly_cpl3() {
    if (is_real__v86()) {
        return ld16_direct(lax);
    }
    tlb_hash = tlb_readonly[lax >> 12];
    if ((tlb_hash | lax) & 1) {
        uint32_t word = ld8_readonly_cpl3();
        lax++;
        word |= ld8_readonly_cpl3() << 8;
        lax--;
        return word;
    }
    return ld16_direct(lax ^ tlb_hash);
}
uint32_t Free86::ld_readonly_cpl3() {
    if (is_real__v86()) {
        return ld_direct(lax);
    }
    tlb_hash = tlb_readonly[lax >> 12];
        if ((tlb_hash | lax) & 3) {
        uint32_t dword = ld16_readonly_cpl3();
        lax += 2;
        dword |= ld16_readonly_cpl3() << 16;
        lax -= 2;
        return dword;
    }
    return ld_direct(lax ^ tlb_hash);
}
uint32_t Free86::ld8_writable_cpl3() {
    if (is_real__v86()) {
        return ld8_direct(lax);
    }
    tlb_hash = tlb_writable[lax >> 12];
    if (tlb_hash == 0xffffffff) {
        page_translation(1, cpl == 3);
        tlb_hash = tlb_writable[lax >> 12];
    }
    return ld8_direct(lax ^ tlb_hash);
}
uint32_t Free86::ld16_writable_cpl3() {
    if (is_real__v86()) {
        return ld16_direct(lax);
    }
    tlb_hash = tlb_writable[lax >> 12];
    if ((tlb_hash | lax) & 1) {
        uint32_t word = ld8_writable_cpl3();
        lax++;
        word |= ld8_writable_cpl3() << 8;
        lax--;
        return word;
    }
    return ld16_direct(lax ^ tlb_hash);
}
uint32_t Free86::ld_writable_cpl3() {
    if (is_real__v86()) {
        return ld_direct(lax);
    }
    tlb_hash = tlb_writable[lax >> 12];
        if ((tlb_hash | lax) & 3) {
        uint32_t dword = ld16_writable_cpl3();
        lax += 2;
        dword |= ld16_writable_cpl3() << 16;
        lax -= 2;
        return dword;
    }
    return ld_direct(lax ^ tlb_hash);
}
void Free86::st8_writable_cplX(uint32_t byte) {
    tlb_hash = tlb_writable_cplX[lax >> 12];
    if (tlb_hash == 0xffffffff) {
        page_translation(1, 0);
        tlb_hash = tlb_writable_cplX[lax >> 12];
    }
    st8_direct(lax ^ tlb_hash, byte);
}
void Free86::st16_writable_cplX(uint32_t word) {
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
void Free86::st_writable_cplX(uint32_t dword) {
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
void Free86::st64_writable_cplX(uint64_t qword) {
    tlb_hash = tlb_writable_cplX[lax >> 12];
    if ((tlb_hash | lax) & 3) {
        st_writable_cplX(qword & 0xffffffff);
        lax += 4;
        st_writable_cplX(qword >> 32);
        lax -= 4;
    } else {
        st64_direct(lax ^ tlb_hash, qword);
    }
}
void Free86::st8_writable_cpl3(uint32_t byte) {
    if (is_real__v86()) {
        st8_direct(lax, byte);
    } 
    tlb_hash = tlb_writable[lax >> 12];
    if (tlb_hash == 0xffffffff) {
        page_translation(1, cpl == 3);
        tlb_hash = tlb_writable[lax >> 12];
    }
    st8_direct(lax ^ tlb_hash, byte);
}
void Free86::st16_writable_cpl3(uint32_t word) {
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
void Free86::st_writable_cpl3(uint32_t dword) {
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
uint32_t Free86::ld8_direct(uint32_t address) {
    return memory[address];
}
uint32_t Free86::ld16_direct(uint32_t address) {
    return ld8_direct(address) | ld8_direct(address + 1) << 8;
}
uint32_t Free86::ld_direct(uint32_t address) {
    return ld16_direct(address) | ld16_direct(address + 2) << 16;
}
uint64_t Free86::ld64_direct(uint32_t address) {
    return (ld_direct(address) & 0xffffffff) | static_cast<uint64_t>(ld_direct(address + 4)) << 32;
}
void Free86::st8_direct(uint32_t address, uint32_t byte) {
    memory[address] = byte & 0xff;
}
void Free86::st8_direct(uint32_t address, std::string data) {
    auto s = data.c_str();
    int l = static_cast<int>(data.length());
    for (int i = 0; i < l; i++) {
        st8_direct(address++, s[i] & 0xff);
    }
    st8_direct(address, 0);
}
void Free86::st16_direct(uint32_t address, uint32_t word) {
    memory[address] = word & 0xff;
    memory[address + 1] = (word >> 8) & 0xff;
}
void Free86::st_direct(uint32_t address, uint32_t dword) {
    memory[address] = dword & 0xff;
    memory[address + 1] = (dword >> 8) & 0xff;
    memory[address + 2] = (dword >> 16) & 0xff;
    memory[address + 3] = (dword >> 24) & 0xff;
}
void Free86::st64_direct(uint32_t address, uint64_t qword) {
    st_direct(address, qword & 0xffffffff);
    st_direct(address + 4, (qword >> 32) & 0xffffffff);
}
uint32_t Free86::fetch8() {
    return ld8_direct(far++);
}
uint32_t Free86::fetch16() {
    uint32_t word = fetch8();
    word |= fetch8() << 8;
    return word;
}
uint32_t Free86::fetch() {
    uint32_t dword = fetch8();
    dword |= fetch8() << 8;
    dword |= fetch8() << 16;
    dword |= fetch8() << 24;
    return dword;
}
void Free86::push16(uint32_t word) {
    uint32_t esp = regs[4] - 2;
    lax = SS_base + (esp & SS_mask);
    st16_writable_cpl3(word);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
}
void Free86::push(uint32_t dword) {
    uint32_t esp = regs[4] - 4;
    lax = SS_base + (esp & SS_mask);
    st_writable_cpl3(dword);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
}
uint32_t Free86::pop16() {
    uint32_t res = ld16_stack();
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 2) & SS_mask);
    return res;
}
uint32_t Free86::pop() {
    uint32_t res = ld_stack();
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 4) & SS_mask);
    return res;
}
uint32_t Free86::ld16_stack() {
    lax = SS_base + (regs[4] & SS_mask);
    return ld16_readonly_cpl3();
}
uint32_t Free86::ld_stack() {
    lax = SS_base + (regs[4] & SS_mask);
    return ld_readonly_cpl3();
}
