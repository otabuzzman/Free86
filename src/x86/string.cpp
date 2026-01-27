#include "free86.h"

void Free86::aux_INSB() {
    int ecx, edx, edi;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    edi = regs[7];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind = io_read(edx);
        lax = (edi & XS_mask) + segs[0].base;
        st8_writable_cpl3(ind);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 0)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind = io_read(edx);
        lax = (edi & XS_mask) + segs[0].base;
        st8_writable_cpl3(ind);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 0)) & XS_mask);
    }
}
void Free86::aux_OUTSB() {
    int ecx, edx, esi, sreg;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
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
        if ((ecx & XS_mask) == 0) {
            return;
        }
        lax = (esi & XS_mask) + segs[sreg].base;
        ind = ld8_readonly_cpl3();
        io_write(edx, ind);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 0)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        lax = (esi & XS_mask) + segs[sreg].base;
        ind = ld8_readonly_cpl3();
        io_write(edx, ind);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 0)) & XS_mask);
    }
}
void Free86::aux_MOVSB() {
    int ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = (esi & XS_mask) + segs[sreg].base;
    la = (edi & XS_mask) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind = ld8_readonly_cpl3();
        lax = la;
        st8_writable_cpl3(ind);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 0)) & XS_mask);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 0)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind = ld8_readonly_cpl3();
        lax = la;
        st8_writable_cpl3(ind);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 0)) & XS_mask);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 0)) & XS_mask);
    }
}
void Free86::aux_STOSB() {
    int ecx, edi;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    edi = regs[7];
    lax = (edi & XS_mask) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        st8_writable_cpl3(regs[0]);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 0)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        st8_writable_cpl3(regs[0]);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 0)) & XS_mask);
    }
}
void Free86::aux_CMPSB() {
    int ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = (esi & XS_mask) + segs[sreg].base;
    la = (edi & XS_mask) + segs[0].base;
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind1st = ld8_readonly_cpl3();
        lax = la;
        ind2nd = ld8_readonly_cpl3();
        calculate8(ind1st, ind2nd);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 0)) & XS_mask);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 0)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind1st = ld8_readonly_cpl3();
        lax = la;
        ind2nd = ld8_readonly_cpl3();
        calculate8(ind1st, ind2nd);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 0)) & XS_mask);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 0)) & XS_mask);
    }
}
void Free86::aux_LODSB() {
    int ecx, esi, sreg;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    lax = (esi & XS_mask) + segs[sreg].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind = ld8_readonly_cpl3();
        regs[0] = (regs[0] & -256) | ind;
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 0)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind = ld8_readonly_cpl3();
        regs[0] = (regs[0] & -256) | ind;
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 0)) & XS_mask);
    }
}
void Free86::aux_SCASB() {
    int ecx, edi;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    edi = regs[7];
    lax = (edi & XS_mask) + segs[0].base;
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind = ld8_readonly_cpl3();
        calculate8(regs[0], ind);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 0)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind = ld8_readonly_cpl3();
        calculate8(regs[0], ind);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 0)) & XS_mask);
    }
}
void Free86::aux_INSW() {
    int ecx, edx, edi;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    edi = regs[7];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind = io_read(edx);
        lax = (edi & XS_mask) + segs[0].base;
        st16_writable_cpl3(ind);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind = io_read(edx);
        lax = (edi & XS_mask) + segs[0].base;
        st16_writable_cpl3(ind);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
    }
}
void Free86::aux_OUTSW() {
    int ecx, edx, esi, sreg;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
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
        if ((ecx & XS_mask) == 0) {
            return;
        }
        lax = (esi & XS_mask) + segs[sreg].base;
        ind = ld16_readonly_cpl3();
        io_write(edx, ind);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 1)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        lax = (esi & XS_mask) + segs[sreg].base;
        ind = ld16_readonly_cpl3();
        io_write(edx, ind);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 1)) & XS_mask);
    }
}
void Free86::aux_MOVSW() {
    int ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = (esi & XS_mask) + segs[sreg].base;
    la = (edi & XS_mask) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind = ld16_readonly_cpl3();
        lax = la;
        st16_writable_cpl3(ind);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 1)) & XS_mask);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind = ld16_readonly_cpl3();
        lax = la;
        st16_writable_cpl3(ind);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 1)) & XS_mask);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
    }
}
void Free86::aux_STOSW() {
    int ecx, edi;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    edi = regs[7];
    lax = (edi & XS_mask) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        st16_writable_cpl3(regs[0]);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        st16_writable_cpl3(regs[0]);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
    }
}
void Free86::aux_CMPSW() {
    int ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = (esi & XS_mask) + segs[sreg].base;
    la = (edi & XS_mask) + segs[0].base;
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind1st = ld16_readonly_cpl3();
        lax = la;
        ind2nd = ld16_readonly_cpl3();
        calculate16(ind1st, ind2nd);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 1)) & XS_mask);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind1st = ld16_readonly_cpl3();
        lax = la;
        ind2nd = ld16_readonly_cpl3();
        calculate16(ind1st, ind2nd);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 1)) & XS_mask);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
    }
}
void Free86::aux_LODSW() {
    int ecx, esi, sreg;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    lax = (esi & XS_mask) + segs[sreg].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind = ld16_readonly_cpl3();
        regs[0] = ind;
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 1)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind = ld16_readonly_cpl3();
        regs[0] = ind;
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 1)) & XS_mask);
    }
}
void Free86::aux_SCASW() {
    int ecx, edi;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    edi = regs[7];
    lax = (edi & XS_mask) + segs[0].base;
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind = ld16_readonly_cpl3();
        calculate16(regs[0], ind);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind = ld16_readonly_cpl3();
        calculate16(regs[0], ind);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
    }
}
void Free86::aux_INS16() {
    int ecx, edx, edi;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    edi = regs[7];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind = io_read(edx);
        lax = (edi & XS_mask) + segs[0].base;
        st16_writable_cpl3(ind);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind = io_read(edx);
        lax = (edi & XS_mask) + segs[0].base;
        st16_writable_cpl3(ind);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
    }
}
void Free86::aux_OUTS16() {
    int ecx, edx, esi, sreg;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
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
        if ((ecx & XS_mask) == 0) {
            return;
        }
        lax = (esi & XS_mask) + segs[sreg].base;
        ind = ld16_readonly_cpl3();
        io_write(edx, ind);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 1)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        lax = (esi & XS_mask) + segs[sreg].base;
        ind = ld16_readonly_cpl3();
        io_write(edx, ind);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 1)) & XS_mask);
    }
}
void Free86::aux_MOVS16() {
    int ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = (esi & XS_mask) + segs[sreg].base;
    la = (edi & XS_mask) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind = ld16_readonly_cpl3();
        lax = la;
        st16_writable_cpl3(ind);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 1)) & XS_mask);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind = ld16_readonly_cpl3();
        lax = la;
        st16_writable_cpl3(ind);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 1)) & XS_mask);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
    }
}
void Free86::aux_STOS16() {
    int ecx, edi;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    edi = regs[7];
    lax = (edi & XS_mask) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        st16_writable_cpl3(regs[0]);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        st16_writable_cpl3(regs[0]);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
    }
}
void Free86::aux_CMPS16() {
    int ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = (esi & XS_mask) + segs[sreg].base;
    la = (edi & XS_mask) + segs[0].base;
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind1st = ld16_readonly_cpl3();
        lax = la;
        ind2nd = ld16_readonly_cpl3();
        calculate16(ind1st, ind2nd);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 1)) & XS_mask);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind1st = ld16_readonly_cpl3();
        lax = la;
        ind2nd = ld16_readonly_cpl3();
        calculate16(ind1st, ind2nd);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 1)) & XS_mask);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
    }
}
void Free86::aux_LODS16() {
    int ecx, esi, sreg;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    lax = (esi & XS_mask) + segs[sreg].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind = ld16_readonly_cpl3();
        regs[0] = (regs[0] & -65536) | ind;
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 1)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind = ld16_readonly_cpl3();
        regs[0] = (regs[0] & -65536) | ind;
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 1)) & XS_mask);
    }
}
void Free86::aux_SCAS16() {
    int ecx, edi;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    edi = regs[7];
    lax = (edi & XS_mask) + segs[0].base;
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind = ld16_readonly_cpl3();
        calculate16(regs[0], ind);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind = ld16_readonly_cpl3();
        calculate16(regs[0], ind);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 1)) & XS_mask);
    }
}
void Free86::aux_INSD() {
    int ecx, edx, edi;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    edi = regs[7];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind = io_read(edx);
        lax = (edi & XS_mask) + segs[0].base;
        st_writable_cpl3(ind);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 2)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind = io_read(edx);
        lax = (edi & XS_mask) + segs[0].base;
        st_writable_cpl3(ind);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 2)) & XS_mask);
    }
}
void Free86::aux_OUTSD() {
    int ecx, edx, esi, sreg;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
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
        if ((ecx & XS_mask) == 0) {
            return;
        }
        lax = (esi & XS_mask) + segs[sreg].base;
        ind = ld_readonly_cpl3();
        io_write(edx, ind);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 2)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        lax = (esi & XS_mask) + segs[sreg].base;
        ind = ld_readonly_cpl3();
        io_write(edx, ind);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 2)) & XS_mask);
    }
}
void Free86::aux_MOVSD() {
    int ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = (esi & XS_mask) + segs[sreg].base;
    la = (edi & XS_mask) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind = ld_readonly_cpl3();
        lax = la;
        st_writable_cpl3(ind);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 2)) & XS_mask);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 2)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind = ld_readonly_cpl3();
        lax = la;
        st_writable_cpl3(ind);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 2)) & XS_mask);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 2)) & XS_mask);
    }
}
void Free86::aux_STOSD() {
    int ecx, edi;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    edi = regs[7];
    lax = (edi & XS_mask) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        st_writable_cpl3(regs[0]);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 2)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        st_writable_cpl3(regs[0]);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 2)) & XS_mask);
    }
}
void Free86::aux_CMPSD() {
    int ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = (esi & XS_mask) + segs[sreg].base;
    la = (edi & XS_mask) + segs[0].base;
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind1st = ld_readonly_cpl3();
        lax = la;
        ind2nd = ld_readonly_cpl3();
        calculate(ind1st, ind2nd);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 2)) & XS_mask);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 2)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind1st = ld_readonly_cpl3();
        lax = la;
        ind2nd = ld_readonly_cpl3();
        calculate(ind1st, ind2nd);
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 2)) & XS_mask);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 2)) & XS_mask);
    }
}
void Free86::aux_LODSD() {
    int ecx, esi, sreg;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    lax = (esi & XS_mask) + segs[sreg].base;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind = ld_readonly_cpl3();
        regs[0] = ind;
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 2)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind = ld_readonly_cpl3();
        regs[0] = ind;
        regs[6] = (esi & ~XS_mask) | ((esi + (df << 2)) & XS_mask);
    }
}
void Free86::aux_SCASD() {
    int ecx, edi;
    if (ipr & 0x0080) {
        XS_mask = 0xffff;
    } else {
        XS_mask = -1;
    }
    edi = regs[7];
    lax = (edi & XS_mask) + segs[0].base;
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & XS_mask) == 0) {
            return;
        }
        ind = ld_readonly_cpl3();
        calculate(regs[0], ind);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 2)) & XS_mask);
        regs[1] = ecx = (ecx & ~XS_mask) | ((ecx - 1) & XS_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & XS_mask) {
            far = far_start;
        }
    } else {
        ind = ld_readonly_cpl3();
        calculate(regs[0], ind);
        regs[7] = (edi & ~XS_mask) | ((edi + (df << 2)) & XS_mask);
    }
}
