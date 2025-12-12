#include "free86.h"

void Free86::aux_INSB() {
    int address_size_mask, ecx, edx, edi;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    edi = regs[7];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld8_io(edx);
        mem8_loc = (edi & address_size_mask) + segs[0].base;
        st8_writable_cpl3(x);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 0)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld8_io(edx);
        mem8_loc = (edi & address_size_mask) + segs[0].base;
        st8_writable_cpl3(x);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 0)) & address_size_mask);
    }
}
void Free86::aux_OUTSB() {
    int address_size_mask, ecx, edx, esi, sreg;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        mem8_loc = (esi & address_size_mask) + segs[sreg].base;
        x = ld8_readonly_cpl3();
        st8_io(edx, x);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 0)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        mem8_loc = (esi & address_size_mask) + segs[sreg].base;
        x = ld8_readonly_cpl3();
        st8_io(edx, x);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 0)) & address_size_mask);
    }
}
void Free86::aux_MOVSB() {
    int address_size_mask, ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    mem8_loc = (esi & address_size_mask) + segs[sreg].base;
    la = (edi & address_size_mask) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld8_readonly_cpl3();
        mem8_loc = la;
        st8_writable_cpl3(x);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 0)) & address_size_mask);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 0)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld8_readonly_cpl3();
        mem8_loc = la;
        st8_writable_cpl3(x);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 0)) & address_size_mask);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 0)) & address_size_mask);
    }
}
void Free86::aux_STOSB() {
    int address_size_mask, ecx, edi;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    edi = regs[7];
    mem8_loc = (edi & address_size_mask) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        st8_writable_cpl3(regs[0]);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 0)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        st8_writable_cpl3(regs[0]);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 0)) & address_size_mask);
    }
}
void Free86::aux_CMPSB() {
    int address_size_mask, ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    mem8_loc = (esi & address_size_mask) + segs[sreg].base;
    la = (edi & address_size_mask) + segs[0].base;
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld8_readonly_cpl3();
        mem8_loc = la;
        y = ld8_readonly_cpl3();
        calculate8(x, y);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 0)) & address_size_mask);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 0)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld8_readonly_cpl3();
        mem8_loc = la;
        y = ld8_readonly_cpl3();
        calculate8(x, y);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 0)) & address_size_mask);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 0)) & address_size_mask);
    }
}
void Free86::aux_LODSB() {
    int address_size_mask, ecx, esi, sreg;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    mem8_loc = (esi & address_size_mask) + segs[sreg].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld8_readonly_cpl3();
        regs[0] = (regs[0] & -256) | x;
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 0)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld8_readonly_cpl3();
        regs[0] = (regs[0] & -256) | x;
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 0)) & address_size_mask);
    }
}
void Free86::aux_SCASB() {
    int address_size_mask, ecx, edi;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    edi = regs[7];
    mem8_loc = (edi & address_size_mask) + segs[0].base;
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld8_readonly_cpl3();
        calculate8(regs[0], x);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 0)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld8_readonly_cpl3();
        calculate8(regs[0], x);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 0)) & address_size_mask);
    }
}
void Free86::aux_INSW() {
    int address_size_mask, ecx, edx, edi;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    edi = regs[7];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld16_io(edx);
        mem8_loc = (edi & address_size_mask) + segs[0].base;
        st16_writable_cpl3(x);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld16_io(edx);
        mem8_loc = (edi & address_size_mask) + segs[0].base;
        st16_writable_cpl3(x);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
    }
}
void Free86::aux_OUTSW() {
    int address_size_mask, ecx, edx, esi, sreg;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        mem8_loc = (esi & address_size_mask) + segs[sreg].base;
        x = ld16_readonly_cpl3();
        st32_io(edx, x);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 1)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        mem8_loc = (esi & address_size_mask) + segs[sreg].base;
        x = ld16_readonly_cpl3();
        st32_io(edx, x);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 1)) & address_size_mask);
    }
}
void Free86::aux_MOVSW() {
    int address_size_mask, ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    mem8_loc = (esi & address_size_mask) + segs[sreg].base;
    la = (edi & address_size_mask) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld16_readonly_cpl3();
        mem8_loc = la;
        st16_writable_cpl3(x);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 1)) & address_size_mask);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld16_readonly_cpl3();
        mem8_loc = la;
        st16_writable_cpl3(x);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 1)) & address_size_mask);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
    }
}
void Free86::aux_STOSW() {
    int address_size_mask, ecx, edi;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    edi = regs[7];
    mem8_loc = (edi & address_size_mask) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        st16_writable_cpl3(regs[0]);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        st16_writable_cpl3(regs[0]);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
    }
}
void Free86::aux_CMPSW() {
    int address_size_mask, ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    mem8_loc = (esi & address_size_mask) + segs[sreg].base;
    la = (edi & address_size_mask) + segs[0].base;
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld16_readonly_cpl3();
        mem8_loc = la;
        y = ld16_readonly_cpl3();
        calculate16(x, y);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 1)) & address_size_mask);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld16_readonly_cpl3();
        mem8_loc = la;
        y = ld16_readonly_cpl3();
        calculate16(x, y);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 1)) & address_size_mask);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
    }
}
void Free86::aux_LODSW() {
    int address_size_mask, ecx, esi, sreg;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    mem8_loc = (esi & address_size_mask) + segs[sreg].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld16_readonly_cpl3();
        regs[0] = x;
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 1)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld16_readonly_cpl3();
        regs[0] = x;
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 1)) & address_size_mask);
    }
}
void Free86::aux_SCASW() {
    int address_size_mask, ecx, edi;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    edi = regs[7];
    mem8_loc = (edi & address_size_mask) + segs[0].base;
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld16_readonly_cpl3();
        calculate16(regs[0], x);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld16_readonly_cpl3();
        calculate16(regs[0], x);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
    }
}
void Free86::aux_INS16() {
    int address_size_mask, ecx, edx, edi;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    edi = regs[7];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld16_io(edx);
        mem8_loc = (edi & address_size_mask) + segs[0].base;
        st16_writable_cpl3(x);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld16_io(edx);
        mem8_loc = (edi & address_size_mask) + segs[0].base;
        st16_writable_cpl3(x);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
    }
}
void Free86::aux_OUTS16() {
    int address_size_mask, ecx, edx, esi, sreg;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        mem8_loc = (esi & address_size_mask) + segs[sreg].base;
        x = ld16_readonly_cpl3();
        st16_io(edx, x);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 1)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        mem8_loc = (esi & address_size_mask) + segs[sreg].base;
        x = ld16_readonly_cpl3();
        st16_io(edx, x);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 1)) & address_size_mask);
    }
}
void Free86::aux_MOVS16() {
    int address_size_mask, ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    mem8_loc = (esi & address_size_mask) + segs[sreg].base;
    la = (edi & address_size_mask) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld16_readonly_cpl3();
        mem8_loc = la;
        st16_writable_cpl3(x);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 1)) & address_size_mask);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld16_readonly_cpl3();
        mem8_loc = la;
        st16_writable_cpl3(x);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 1)) & address_size_mask);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
    }
}
void Free86::aux_STOS16() {
    int address_size_mask, ecx, edi;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    edi = regs[7];
    mem8_loc = (edi & address_size_mask) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        st16_writable_cpl3(regs[0]);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        st16_writable_cpl3(regs[0]);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
    }
}
void Free86::aux_CMPS16() {
    int address_size_mask, ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    mem8_loc = (esi & address_size_mask) + segs[sreg].base;
    la = (edi & address_size_mask) + segs[0].base;
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld16_readonly_cpl3();
        mem8_loc = la;
        y = ld16_readonly_cpl3();
        calculate16(x, y);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 1)) & address_size_mask);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld16_readonly_cpl3();
        mem8_loc = la;
        y = ld16_readonly_cpl3();
        calculate16(x, y);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 1)) & address_size_mask);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
    }
}
void Free86::aux_LODS16() {
    int address_size_mask, ecx, esi, sreg;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    mem8_loc = (esi & address_size_mask) + segs[sreg].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld16_readonly_cpl3();
        regs[0] = (regs[0] & -65536) | x;
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 1)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld16_readonly_cpl3();
        regs[0] = (regs[0] & -65536) | x;
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 1)) & address_size_mask);
    }
}
void Free86::aux_SCAS16() {
    int address_size_mask, ecx, edi;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    edi = regs[7];
    mem8_loc = (edi & address_size_mask) + segs[0].base;
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld16_readonly_cpl3();
        calculate16(regs[0], x);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld16_readonly_cpl3();
        calculate16(regs[0], x);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 1)) & address_size_mask);
    }
}
void Free86::aux_INSD() {
    int address_size_mask, ecx, edx, edi;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    edi = regs[7];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld32_io(edx);
        mem8_loc = (edi & address_size_mask) + segs[0].base;
        st32_writable_cpl3(x);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 2)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld32_io(edx);
        mem8_loc = (edi & address_size_mask) + segs[0].base;
        st32_writable_cpl3(x);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 2)) & address_size_mask);
    }
}
void Free86::aux_OUTSD() {
    int address_size_mask, ecx, edx, esi, sreg;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        mem8_loc = (esi & address_size_mask) + segs[sreg].base;
        x = ld32_readonly_cpl3();
        st32_io(edx, x);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 2)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        mem8_loc = (esi & address_size_mask) + segs[sreg].base;
        x = ld32_readonly_cpl3();
        st32_io(edx, x);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 2)) & address_size_mask);
    }
}
void Free86::aux_MOVSD() {
    int address_size_mask, ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    mem8_loc = (esi & address_size_mask) + segs[sreg].base;
    la = (edi & address_size_mask) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld32_readonly_cpl3();
        mem8_loc = la;
        st32_writable_cpl3(x);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 2)) & address_size_mask);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 2)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld32_readonly_cpl3();
        mem8_loc = la;
        st32_writable_cpl3(x);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 2)) & address_size_mask);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 2)) & address_size_mask);
    }
}
void Free86::aux_STOSD() {
    int address_size_mask, ecx, edi;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    edi = regs[7];
    mem8_loc = (edi & address_size_mask) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        st32_writable_cpl3(regs[0]);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 2)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        st32_writable_cpl3(regs[0]);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 2)) & address_size_mask);
    }
}
void Free86::aux_CMPSD() {
    int address_size_mask, ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    mem8_loc = (esi & address_size_mask) + segs[sreg].base;
    la = (edi & address_size_mask) + segs[0].base;
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld32_readonly_cpl3();
        mem8_loc = la;
        y = ld32_readonly_cpl3();
        calculate32(x, y);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 2)) & address_size_mask);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 2)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld32_readonly_cpl3();
        mem8_loc = la;
        y = ld32_readonly_cpl3();
        calculate32(x, y);
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 2)) & address_size_mask);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 2)) & address_size_mask);
    }
}
void Free86::aux_LODSD() {
    int address_size_mask, ecx, esi, sreg;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    mem8_loc = (esi & address_size_mask) + segs[sreg].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld32_readonly_cpl3();
        regs[0] = x;
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 2)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld32_readonly_cpl3();
        regs[0] = x;
        regs[6] = (esi & ~address_size_mask) | ((esi + (df << 2)) & address_size_mask);
    }
}
void Free86::aux_SCASD() {
    int address_size_mask, ecx, edi;
    if (ipr & 0x0080) {
        address_size_mask = 0xffff;
    } else {
        address_size_mask = -1;
    }
    edi = regs[7];
    mem8_loc = (edi & address_size_mask) + segs[0].base;
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & address_size_mask) == 0) {
            return;
        }
        x = ld32_readonly_cpl3();
        calculate32(regs[0], x);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 2)) & address_size_mask);
        regs[1] = ecx = (ecx & ~address_size_mask) | ((ecx - 1) & address_size_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & address_size_mask) {
            far = far_start;
        }
    } else {
        x = ld32_readonly_cpl3();
        calculate32(regs[0], x);
        regs[7] = (edi & ~address_size_mask) | ((edi + (df << 2)) & address_size_mask);
    }
}
