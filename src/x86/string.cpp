#include "free86.h"

void Free86::aux_INSB() {
    int ecx, edx, edi;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    edi = regs[7];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = io_read(edx);
        lax = segs[0].shadow.base + (edi & ipr_os_mask);
        st8_writable_cpl3(x);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 0)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = io_read(edx);
        lax = segs[0].shadow.base + (edi & ipr_os_mask);
        st8_writable_cpl3(x);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 0)) & ipr_os_mask);
    }
}
void Free86::aux_OUTSB() {
    int ecx, edx, esi, sreg;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
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
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
        x = ld8_readonly_cpl3();
        io_write(edx, x);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 0)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
        x = ld8_readonly_cpl3();
        io_write(edx, x);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 0)) & ipr_os_mask);
    }
}
void Free86::aux_MOVSB() {
    int ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
    la = segs[0].shadow.base + (edi & ipr_os_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = ld8_readonly_cpl3();
        lax = la;
        st8_writable_cpl3(x);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 0)) & ipr_os_mask);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 0)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = ld8_readonly_cpl3();
        lax = la;
        st8_writable_cpl3(x);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 0)) & ipr_os_mask);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 0)) & ipr_os_mask);
    }
}
void Free86::aux_STOSB() {
    int ecx, edi;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    edi = regs[7];
    lax = segs[0].shadow.base + (edi & ipr_os_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        st8_writable_cpl3(regs[0]);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 0)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        st8_writable_cpl3(regs[0]);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 0)) & ipr_os_mask);
    }
}
void Free86::aux_CMPSB() {
    int ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
    la = segs[0].shadow.base + (edi & ipr_os_mask);
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = ld8_readonly_cpl3();
        lax = la;
        y = ld8_readonly_cpl3();
        calculate8(x, y);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 0)) & ipr_os_mask);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 0)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = ld8_readonly_cpl3();
        lax = la;
        y = ld8_readonly_cpl3();
        calculate8(x, y);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 0)) & ipr_os_mask);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 0)) & ipr_os_mask);
    }
}
void Free86::aux_LODSB() {
    int ecx, esi, sreg;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = ld8_readonly_cpl3();
        regs[0] = (regs[0] & -256) | x;
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 0)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = ld8_readonly_cpl3();
        regs[0] = (regs[0] & -256) | x;
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 0)) & ipr_os_mask);
    }
}
void Free86::aux_SCASB() {
    int ecx, edi;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    edi = regs[7];
    lax = segs[0].shadow.base + (edi & ipr_os_mask);
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = ld8_readonly_cpl3();
        calculate8(regs[0], x);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 0)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = ld8_readonly_cpl3();
        calculate8(regs[0], x);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 0)) & ipr_os_mask);
    }
}
void Free86::aux_INSW() {
    int ecx, edx, edi;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    edi = regs[7];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = io_read(edx);
        lax = segs[0].shadow.base + (edi & ipr_os_mask);
        st16_writable_cpl3(x);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = io_read(edx);
        lax = segs[0].shadow.base + (edi & ipr_os_mask);
        st16_writable_cpl3(x);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
    }
}
void Free86::aux_OUTSW() {
    int ecx, edx, esi, sreg;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
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
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
        x = ld16_readonly_cpl3();
        io_write(edx, x);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 1)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
        x = ld16_readonly_cpl3();
        io_write(edx, x);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 1)) & ipr_os_mask);
    }
}
void Free86::aux_MOVSW() {
    int ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
    la = segs[0].shadow.base + (edi & ipr_os_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = ld16_readonly_cpl3();
        lax = la;
        st16_writable_cpl3(x);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 1)) & ipr_os_mask);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = ld16_readonly_cpl3();
        lax = la;
        st16_writable_cpl3(x);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 1)) & ipr_os_mask);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
    }
}
void Free86::aux_STOSW() {
    int ecx, edi;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    edi = regs[7];
    lax = segs[0].shadow.base + (edi & ipr_os_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        st16_writable_cpl3(regs[0]);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        st16_writable_cpl3(regs[0]);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
    }
}
void Free86::aux_CMPSW() {
    int ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
    la = segs[0].shadow.base + (edi & ipr_os_mask);
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = ld16_readonly_cpl3();
        lax = la;
        y = ld16_readonly_cpl3();
        calculate16(x, y);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 1)) & ipr_os_mask);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = ld16_readonly_cpl3();
        lax = la;
        y = ld16_readonly_cpl3();
        calculate16(x, y);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 1)) & ipr_os_mask);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
    }
}
void Free86::aux_LODSW() {
    int ecx, esi, sreg;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = ld16_readonly_cpl3();
        regs[0] = x;
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 1)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = ld16_readonly_cpl3();
        regs[0] = x;
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 1)) & ipr_os_mask);
    }
}
void Free86::aux_SCASW() {
    int ecx, edi;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    edi = regs[7];
    lax = segs[0].shadow.base + (edi & ipr_os_mask);
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = ld16_readonly_cpl3();
        calculate16(regs[0], x);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = ld16_readonly_cpl3();
        calculate16(regs[0], x);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
    }
}
void Free86::aux_INS16() {
    int ecx, edx, edi;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    edi = regs[7];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = io_read(edx);
        lax = segs[0].shadow.base + (edi & ipr_os_mask);
        st16_writable_cpl3(x);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = io_read(edx);
        lax = segs[0].shadow.base + (edi & ipr_os_mask);
        st16_writable_cpl3(x);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
    }
}
void Free86::aux_OUTS16() {
    int ecx, edx, esi, sreg;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
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
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
        x = ld16_readonly_cpl3();
        io_write(edx, x);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 1)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
        x = ld16_readonly_cpl3();
        io_write(edx, x);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 1)) & ipr_os_mask);
    }
}
void Free86::aux_MOVS16() {
    int ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
    la = segs[0].shadow.base + (edi & ipr_os_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = ld16_readonly_cpl3();
        lax = la;
        st16_writable_cpl3(x);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 1)) & ipr_os_mask);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = ld16_readonly_cpl3();
        lax = la;
        st16_writable_cpl3(x);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 1)) & ipr_os_mask);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
    }
}
void Free86::aux_STOS16() {
    int ecx, edi;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    edi = regs[7];
    lax = segs[0].shadow.base + (edi & ipr_os_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        st16_writable_cpl3(regs[0]);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        st16_writable_cpl3(regs[0]);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
    }
}
void Free86::aux_CMPS16() {
    int ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
    la = segs[0].shadow.base + (edi & ipr_os_mask);
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = ld16_readonly_cpl3();
        lax = la;
        y = ld16_readonly_cpl3();
        calculate16(x, y);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 1)) & ipr_os_mask);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = ld16_readonly_cpl3();
        lax = la;
        y = ld16_readonly_cpl3();
        calculate16(x, y);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 1)) & ipr_os_mask);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
    }
}
void Free86::aux_LODS16() {
    int ecx, esi, sreg;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = ld16_readonly_cpl3();
        regs[0] = (regs[0] & -65536) | x;
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 1)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = ld16_readonly_cpl3();
        regs[0] = (regs[0] & -65536) | x;
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 1)) & ipr_os_mask);
    }
}
void Free86::aux_SCAS16() {
    int ecx, edi;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    edi = regs[7];
    lax = segs[0].shadow.base + (edi & ipr_os_mask);
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = ld16_readonly_cpl3();
        calculate16(regs[0], x);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = ld16_readonly_cpl3();
        calculate16(regs[0], x);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 1)) & ipr_os_mask);
    }
}
void Free86::aux_INSD() {
    int ecx, edx, edi;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    edi = regs[7];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = io_read(edx);
        lax = segs[0].shadow.base + (edi & ipr_os_mask);
        st_writable_cpl3(x);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 2)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = io_read(edx);
        lax = segs[0].shadow.base + (edi & ipr_os_mask);
        st_writable_cpl3(x);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 2)) & ipr_os_mask);
    }
}
void Free86::aux_OUTSD() {
    int ecx, edx, esi, sreg;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
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
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
        x = ld_readonly_cpl3();
        io_write(edx, x);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 2)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
        x = ld_readonly_cpl3();
        io_write(edx, x);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 2)) & ipr_os_mask);
    }
}
void Free86::aux_MOVSD() {
    int ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
    la = segs[0].shadow.base + (edi & ipr_os_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = ld_readonly_cpl3();
        lax = la;
        st_writable_cpl3(x);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 2)) & ipr_os_mask);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 2)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = ld_readonly_cpl3();
        lax = la;
        st_writable_cpl3(x);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 2)) & ipr_os_mask);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 2)) & ipr_os_mask);
    }
}
void Free86::aux_STOSD() {
    int ecx, edi;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    edi = regs[7];
    lax = segs[0].shadow.base + (edi & ipr_os_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        st_writable_cpl3(regs[0]);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 2)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        st_writable_cpl3(regs[0]);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 2)) & ipr_os_mask);
    }
}
void Free86::aux_CMPSD() {
    int ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
    la = segs[0].shadow.base + (edi & ipr_os_mask);
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = ld_readonly_cpl3();
        lax = la;
        y = ld_readonly_cpl3();
        calculate(x, y);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 2)) & ipr_os_mask);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 2)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = ld_readonly_cpl3();
        lax = la;
        y = ld_readonly_cpl3();
        calculate(x, y);
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 2)) & ipr_os_mask);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 2)) & ipr_os_mask);
    }
}
void Free86::aux_LODSD() {
    int ecx, esi, sreg;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    lax = segs[sreg].shadow.base + (esi & ipr_os_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = ld_readonly_cpl3();
        regs[0] = x;
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 2)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = ld_readonly_cpl3();
        regs[0] = x;
        regs[6] = (esi & ~ipr_os_mask) | ((esi + (df << 2)) & ipr_os_mask);
    }
}
void Free86::aux_SCASD() {
    int ecx, edi;
    if (ipr & 0x0080) {
        ipr_os_mask = 0xffff;
    } else {
        ipr_os_mask = -1;
    }
    edi = regs[7];
    lax = segs[0].shadow.base + (edi & ipr_os_mask);
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_os_mask) == 0) {
            return;
        }
        x = ld_readonly_cpl3();
        calculate(regs[0], x);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 2)) & ipr_os_mask);
        regs[1] = ecx = (ecx & ~ipr_os_mask) | ((ecx - 1) & ipr_os_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & ipr_os_mask) {
            far = far_start;
        }
    } else {
        x = ld_readonly_cpl3();
        calculate(regs[0], x);
        regs[7] = (edi & ~ipr_os_mask) | ((edi + (df << 2)) & ipr_os_mask);
    }
}
