#include "free86.h"

void Free86::aux_INSB() {
    uint32_t ecx, edx, edi;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    edi = regs[7];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = io_read(edx);
        lax = segs[0].shadow.base + (edi & ipr_as_mask);
        st8_writable_cpl3(u);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 0)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = io_read(edx);
        lax = segs[0].shadow.base + (edi & ipr_as_mask);
        st8_writable_cpl3(u);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 0)) & ipr_as_mask);
    }
}
void Free86::aux_OUTSB() {
    uint32_t ecx, edx, esi, sreg;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
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
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
        u = ld8_readonly_cpl3();
        io_write(edx, u);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 0)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
        u = ld8_readonly_cpl3();
        io_write(edx, u);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 0)) & ipr_as_mask);
    }
}
void Free86::aux_MOVSB() {
    uint32_t ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
    la = segs[0].shadow.base + (edi & ipr_as_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = ld8_readonly_cpl3();
        lax = la;
        st8_writable_cpl3(u);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 0)) & ipr_as_mask);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 0)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = ld8_readonly_cpl3();
        lax = la;
        st8_writable_cpl3(u);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 0)) & ipr_as_mask);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 0)) & ipr_as_mask);
    }
}
void Free86::aux_STOSB() {
    uint32_t ecx, edi;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    edi = regs[7];
    lax = segs[0].shadow.base + (edi & ipr_as_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        st8_writable_cpl3(regs[0]);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 0)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        st8_writable_cpl3(regs[0]);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 0)) & ipr_as_mask);
    }
}
void Free86::aux_CMPSB() {
    uint32_t ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
    la = segs[0].shadow.base + (edi & ipr_as_mask);
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = ld8_readonly_cpl3();
        lax = la;
        v = ld8_readonly_cpl3();
        calculate8(u, v);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 0)) & ipr_as_mask);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 0)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = ld8_readonly_cpl3();
        lax = la;
        v = ld8_readonly_cpl3();
        calculate8(u, v);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 0)) & ipr_as_mask);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 0)) & ipr_as_mask);
    }
}
void Free86::aux_LODSB() {
    uint32_t ecx, esi, sreg;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = ld8_readonly_cpl3();
        regs[0] = (regs[0] & 0xffffff00) | u;
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 0)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = ld8_readonly_cpl3();
        regs[0] = (regs[0] & 0xffffff00) | u;
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 0)) & ipr_as_mask);
    }
}
void Free86::aux_SCASB() {
    uint32_t ecx, edi;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    edi = regs[7];
    lax = segs[0].shadow.base + (edi & ipr_as_mask);
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = ld8_readonly_cpl3();
        calculate8(regs[0], u);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 0)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = ld8_readonly_cpl3();
        calculate8(regs[0], u);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 0)) & ipr_as_mask);
    }
}
void Free86::aux_INSW() {
    uint32_t ecx, edx, edi;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    edi = regs[7];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = io_read(edx);
        lax = segs[0].shadow.base + (edi & ipr_as_mask);
        st16_writable_cpl3(u);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = io_read(edx);
        lax = segs[0].shadow.base + (edi & ipr_as_mask);
        st16_writable_cpl3(u);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
    }
}
void Free86::aux_OUTSW() {
    uint32_t ecx, edx, esi, sreg;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
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
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
        u = ld16_readonly_cpl3();
        io_write(edx, u);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
        u = ld16_readonly_cpl3();
        io_write(edx, u);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
    }
}
void Free86::aux_MOVSW() {
    uint32_t ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
    la = segs[0].shadow.base + (edi & ipr_as_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = ld16_readonly_cpl3();
        lax = la;
        st16_writable_cpl3(u);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = ld16_readonly_cpl3();
        lax = la;
        st16_writable_cpl3(u);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
    }
}
void Free86::aux_STOSW() {
    uint32_t ecx, edi;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    edi = regs[7];
    lax = segs[0].shadow.base + (edi & ipr_as_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        st16_writable_cpl3(regs[0]);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        st16_writable_cpl3(regs[0]);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
    }
}
void Free86::aux_CMPSW() {
    uint32_t ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
    la = segs[0].shadow.base + (edi & ipr_as_mask);
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = ld16_readonly_cpl3();
        lax = la;
        v = ld16_readonly_cpl3();
        calculate16(u, v);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = ld16_readonly_cpl3();
        lax = la;
        v = ld16_readonly_cpl3();
        calculate16(u, v);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
    }
}
void Free86::aux_LODSW() {
    uint32_t ecx, esi, sreg;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = ld16_readonly_cpl3();
        regs[0] = u;
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = ld16_readonly_cpl3();
        regs[0] = u;
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
    }
}
void Free86::aux_SCASW() {
    uint32_t ecx, edi;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    edi = regs[7];
    lax = segs[0].shadow.base + (edi & ipr_as_mask);
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = ld16_readonly_cpl3();
        calculate16(regs[0], u);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = ld16_readonly_cpl3();
        calculate16(regs[0], u);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
    }
}
void Free86::aux16_INS() {
    uint32_t ecx, edx, edi;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    edi = regs[7];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = io_read(edx);
        lax = segs[0].shadow.base + (edi & ipr_as_mask);
        st16_writable_cpl3(u);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = io_read(edx);
        lax = segs[0].shadow.base + (edi & ipr_as_mask);
        st16_writable_cpl3(u);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
    }
}
void Free86::aux16_OUTS() {
    uint32_t ecx, edx, esi, sreg;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
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
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
        u = ld16_readonly_cpl3();
        io_write(edx, u);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
        u = ld16_readonly_cpl3();
        io_write(edx, u);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
    }
}
void Free86::aux16_MOVS() {
    uint32_t ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
    la = segs[0].shadow.base + (edi & ipr_as_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = ld16_readonly_cpl3();
        lax = la;
        st16_writable_cpl3(u);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = ld16_readonly_cpl3();
        lax = la;
        st16_writable_cpl3(u);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
    }
}
void Free86::aux16_STOS() {
    uint32_t ecx, edi;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    edi = regs[7];
    lax = segs[0].shadow.base + (edi & ipr_as_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        st16_writable_cpl3(regs[0]);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        st16_writable_cpl3(regs[0]);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
    }
}
void Free86::aux16_CMPS() {
    uint32_t ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
    la = segs[0].shadow.base + (edi & ipr_as_mask);
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = ld16_readonly_cpl3();
        lax = la;
        v = ld16_readonly_cpl3();
        calculate16(u, v);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = ld16_readonly_cpl3();
        lax = la;
        v = ld16_readonly_cpl3();
        calculate16(u, v);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
    }
}
void Free86::aux16_LODS() {
    uint32_t ecx, esi, sreg;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = ld16_readonly_cpl3();
        regs[0] = (regs[0] & 0xffff0000) | u;
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = ld16_readonly_cpl3();
        regs[0] = (regs[0] & 0xffff0000) | u;
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
    }
}
void Free86::aux16_SCAS() {
    uint32_t ecx, edi;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    edi = regs[7];
    lax = segs[0].shadow.base + (edi & ipr_as_mask);
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = ld16_readonly_cpl3();
        calculate16(regs[0], u);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = ld16_readonly_cpl3();
        calculate16(regs[0], u);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 1)) & ipr_as_mask);
    }
}
void Free86::aux_INSD() {
    uint32_t ecx, edx, edi;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    edi = regs[7];
    edx = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = io_read(edx);
        lax = segs[0].shadow.base + (edi & ipr_as_mask);
        st_writable_cpl3(u);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 2)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = io_read(edx);
        lax = segs[0].shadow.base + (edi & ipr_as_mask);
        st_writable_cpl3(u);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 2)) & ipr_as_mask);
    }
}
void Free86::aux_OUTSD() {
    uint32_t ecx, edx, esi, sreg;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
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
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
        u = ld_readonly_cpl3();
        io_write(edx, u);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 2)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
        u = ld_readonly_cpl3();
        io_write(edx, u);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 2)) & ipr_as_mask);
    }
}
void Free86::aux_MOVSD() {
    uint32_t ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
    la = segs[0].shadow.base + (edi & ipr_as_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = ld_readonly_cpl3();
        lax = la;
        st_writable_cpl3(u);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 2)) & ipr_as_mask);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 2)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = ld_readonly_cpl3();
        lax = la;
        st_writable_cpl3(u);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 2)) & ipr_as_mask);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 2)) & ipr_as_mask);
    }
}
void Free86::aux_STOSD() {
    uint32_t ecx, edi;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    edi = regs[7];
    lax = segs[0].shadow.base + (edi & ipr_as_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        st_writable_cpl3(regs[0]);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 2)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        st_writable_cpl3(regs[0]);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 2)) & ipr_as_mask);
    }
}
void Free86::aux_CMPSD() {
    uint32_t ecx, edi, esi, sreg, la;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    edi = regs[7];
    lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
    la = segs[0].shadow.base + (edi & ipr_as_mask);
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = ld_readonly_cpl3();
        lax = la;
        v = ld_readonly_cpl3();
        calculate(u, v);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 2)) & ipr_as_mask);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 2)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = ld_readonly_cpl3();
        lax = la;
        v = ld_readonly_cpl3();
        calculate(u, v);
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 2)) & ipr_as_mask);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 2)) & ipr_as_mask);
    }
}
void Free86::aux_LODSD() {
    uint32_t ecx, esi, sreg;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    esi = regs[6];
    lax = segs[sreg].shadow.base + (esi & ipr_as_mask);
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = ld_readonly_cpl3();
        regs[0] = u;
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 2)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = ld_readonly_cpl3();
        regs[0] = u;
        regs[6] = (esi & ~ipr_as_mask) | ((esi + (static_cast<uint32_t>(df) << 2)) & ipr_as_mask);
    }
}
void Free86::aux_SCASD() {
    uint32_t ecx, edi;
    if (ipr & 0x0080) {
        ipr_as_mask = 0xffff;
    } else {
        ipr_as_mask = 0xffffffff;
    }
    edi = regs[7];
    lax = segs[0].shadow.base + (edi & ipr_as_mask);
    operation = 7;
    if (ipr & (0x0010 | 0x0020)) {
        ecx = regs[1];
        if ((ecx & ipr_as_mask) == 0) {
            return;
        }
        u = ld_readonly_cpl3();
        calculate(regs[0], u);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 2)) & ipr_as_mask);
        regs[1] = ecx = (ecx & ~ipr_as_mask) | ((ecx - 1) & ipr_as_mask);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ecx & ipr_as_mask) {
            far = far_start;
        }
    } else {
        u = ld_readonly_cpl3();
        calculate(regs[0], u);
        regs[7] = (edi & ~ipr_as_mask) | ((edi + (static_cast<uint32_t>(df) << 2)) & ipr_as_mask);
    }
}
