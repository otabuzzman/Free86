#include "x86.h"

bool x86Internal::is_CF() {
    bool rval;
    int currentcc_op;
    uint32_t reldst;
    if (osm >= 25) {
        currentcc_op = ocm_preserved;
        reldst = ocm_dst_preserved;
    } else {
        currentcc_op = osm;
        reldst = osm_dst;
    }
    switch (currentcc_op % 25) {
    case 0:
        rval = (reldst & 0xff) < (osm_src & 0xff);
        break;
    case 1:
        rval = (reldst & 0xffff) < (osm_src & 0xffff);
        break;
    case 2:
        rval = reldst < osm_src;
        break;
    case 3:
        rval = (reldst & 0xff) <= (osm_src & 0xff);
        break;
    case 4:
        rval = (reldst & 0xffff) <= (osm_src & 0xffff);
        break;
    case 5:
        rval = reldst <= osm_src;
        break;
    case 6:
        rval = ((reldst + osm_src) & 0xff) < (osm_src & 0xff);
        break;
    case 7:
        rval = ((reldst + osm_src) & 0xffff) < (osm_src & 0xffff);
        break;
    case 8:
        rval = ((reldst + osm_src) < osm_src);
        break;
    case 9:
        rval = ((reldst + osm_src + 1) & 0xff) <= (osm_src & 0xff);
        break;
    case 10:
        rval = ((reldst + osm_src + 1) & 0xffff) <= (osm_src & 0xffff);
        break;
    case 11:
        rval = ((reldst + osm_src + 1) <= osm_src);
        break;
    case 12:
    case 13:
    case 14:
        rval = 0;
        break;
    case 15:
        rval = (osm_src >> 7) & 1;
        break;
    case 16:
        rval = (osm_src >> 15) & 1;
        break;
    case 17:
        rval = (osm_src >> 31) & 1;
        break;
    case 18:
    case 19:
    case 20:
        rval = osm_src & 1;
        break;
    case 21:
    case 22:
    case 23:
        rval = osm_src != 0;
        break;
    case 24:
        rval = osm_src & 1;
        break;
    }
    return rval;
}
bool x86Internal::is_OF() {
    bool rval;
    int Yb;
    switch (osm % 0x1f) {
    case 0:
        Yb = osm_dst - osm_src;
        rval = (((Yb ^ osm_src ^ -1) & (Yb ^ osm_dst)) >> 7) & 1;
        break;
    case 1:
        Yb = osm_dst - osm_src;
        rval = (((Yb ^ osm_src ^ -1) & (Yb ^ osm_dst)) >> 15) & 1;
        break;
    case 2:
        Yb = osm_dst - osm_src;
        rval = (((Yb ^ osm_src ^ -1) & (Yb ^ osm_dst)) >> 31) & 1;
        break;
    case 3:
        Yb = osm_dst - osm_src - 1;
        rval = (((Yb ^ osm_src ^ -1) & (Yb ^ osm_dst)) >> 7) & 1;
        break;
    case 4:
        Yb = osm_dst - osm_src - 1;
        rval = (((Yb ^ osm_src ^ -1) & (Yb ^ osm_dst)) >> 15) & 1;
        break;
    case 5:
        Yb = osm_dst - osm_src - 1;
        rval = (((Yb ^ osm_src ^ -1) & (Yb ^ osm_dst)) >> 31) & 1;
        break;
    case 6:
        Yb = osm_dst + osm_src;
        rval = (((Yb ^ osm_src) & (Yb ^ osm_dst)) >> 7) & 1;
        break;
    case 7:
        Yb = osm_dst + osm_src;
        rval = (((Yb ^ osm_src) & (Yb ^ osm_dst)) >> 15) & 1;
        break;
    case 8:
        Yb = osm_dst + osm_src;
        rval = (((Yb ^ osm_src) & (Yb ^ osm_dst)) >> 31) & 1;
        break;
    case 9:
        Yb = osm_dst + osm_src + 1;
        rval = (((Yb ^ osm_src) & (Yb ^ osm_dst)) >> 7) & 1;
        break;
    case 10:
        Yb = osm_dst + osm_src + 1;
        rval = (((Yb ^ osm_src) & (Yb ^ osm_dst)) >> 15) & 1;
        break;
    case 11:
        Yb = osm_dst + osm_src + 1;
        rval = (((Yb ^ osm_src) & (Yb ^ osm_dst)) >> 31) & 1;
        break;
    case 12:
    case 13:
    case 14:
        rval = 0;
        break;
    case 15:
    case 18:
        rval = ((osm_src ^ osm_dst) >> 7) & 1;
        break;
    case 16:
    case 19:
        rval = ((osm_src ^ osm_dst) >> 15) & 1;
        break;
    case 17:
    case 20:
        rval = ((osm_src ^ osm_dst) >> 31) & 1;
        break;
    case 21:
    case 22:
    case 23:
        rval = osm_src != 0;
        break;
    case 24:
        rval = (osm_src >> 11) & 1;
        break;
    case 25:
        rval = (osm_dst & 0xff) == 0x80;
        break;
    case 26:
        rval = (osm_dst & 0xffff) == 0x8000;
        break;
    case 27:
        rval = (osm_dst == -2147483648);
        break;
    case 28:
        rval = (osm_dst & 0xff) == 0x7f;
        break;
    case 29:
        rval = (osm_dst & 0xffff) == 0x7fff;
        break;
    case 30:
        rval = osm_dst == 0x7fffffff;
        break;
    }
    return rval;
}
bool x86Internal::is_BE() { // `below' for signed comparison, PM p. 317
    bool flg = false;
    switch (osm) {
    case 6:
        flg = ((osm_dst + osm_src) & 0xff) <= (osm_src & 0xff);
        break;
    case 7:
        flg = ((osm_dst + osm_src) & 0xffff) <= (osm_src & 0xffff);
        break;
    case 8: {
        uint32_t val = osm_dst + osm_src;
        flg = val <= osm_src;
    } break;
    case 24:
        flg = (osm_src & (0x0040 | 0x0001)) != 0;
        break;
    default:
        flg = is_CF() | (osm_dst == 0);
        break;
    }
    return flg;
}
int x86Internal::is_PF() {
    if (osm == 24) {
        return (osm_src >> 2) & 1;
    } else {
        return parity_LUT[osm_dst & 0xff];
    }
}
int x86Internal::is_LT() {
    bool flg;
    switch (osm) {
    case 6:
        flg = ((osm_dst + osm_src) << 24) < (osm_src << 24);
        break;
    case 7:
        flg = ((osm_dst + osm_src) << 16) < (osm_src << 16);
        break;
    case 8:
        flg = (osm_dst + osm_src) < osm_src;
        break;
    case 12:
    case 13:
    case 14:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
        flg = osm_dst < 0;
        break;
    case 24:
        flg = ((osm_src >> 7) ^ (osm_src >> 11)) & 1;
        break;
    default:
        flg = (osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst < 0)) ^ is_OF();
        break;
    }
    return flg;
}
int x86Internal::is_LE() { // `less' for unsigned comparison, PM p. 317
    bool flg;
    switch (osm) {
    case 6:
        flg = ((osm_dst + osm_src) << 24) <= (osm_src << 24);
        break;
    case 7:
        flg = ((osm_dst + osm_src) << 16) <= (osm_src << 16);
        break;
    case 8:
        flg = (osm_dst + osm_src) <= osm_src;
        break;
    case 12:
    case 13:
    case 14:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
        flg = osm_dst <= 0;
        break;
    case 24:
        flg = (((osm_src >> 7) ^ (osm_src >> 11)) | (osm_src >> 6)) & 1;
        break;
    default:
        flg = ((osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst < 0)) ^ is_OF()) | (osm_dst == 0);
        break;
    }
    return flg;
}
int x86Internal::is_AF() {
    int Yb;
    int rval;
    switch (osm % 0x1f) {
    case 0:
    case 1:
    case 2:
        Yb = osm_dst - osm_src;
        rval = (osm_dst ^ Yb ^ osm_src) & 0x10;
        break;
    case 3:
    case 4:
    case 5:
        Yb = osm_dst - osm_src - 1;
        rval = (osm_dst ^ Yb ^ osm_src) & 0x10;
        break;
    case 6:
    case 7:
    case 8:
        Yb = osm_dst + osm_src;
        rval = (osm_dst ^ Yb ^ osm_src) & 0x10;
        break;
    case 9:
    case 10:
    case 11:
        Yb = osm_dst + osm_src + 1;
        rval = (osm_dst ^ Yb ^ osm_src) & 0x10;
        break;
    case 12:
    case 13:
    case 14:
        rval = 0;
        break;
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
        rval = 0;
        break;
    case 24:
        rval = osm_src & 0x10;
        break;
    case 25:
    case 26:
    case 27:
        rval = (osm_dst ^ (osm_dst - 1)) & 0x10;
        break;
    case 28:
    case 29:
    case 30:
        rval = (osm_dst ^ (osm_dst + 1)) & 0x10;
        break;
    }
    return rval;
}
int x86Internal::can_jump(int condition) {
    bool flg;
    switch ((condition >> 1) & 7) {
    case 0:
        flg = is_OF();
        break;
    case 1:
        flg = is_CF();
        break;
    case 2:
        flg = (osm_dst == 0);
        break;
    case 3:
        flg = is_BE();
        break;
    case 4:
        flg = (osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst < 0));
        break;
    case 5:
        flg = is_PF();
        break;
    case 6:
        flg = is_LT();
        break;
    case 7:
        flg = is_LE();
        break;
    }
    return flg ^ (condition & 1);
}
int x86Internal::compile_flags(bool shift) {
    int c0 = 0, c11 = 0;
    if (!shift) {
        c0 = (is_CF() << 0);
        c11 = (is_OF() << 11);
    }
    int c2 = (is_PF() << 2);
    int c4 = is_AF();
    int c6 = ((osm_dst == 0) << 6);
    int c7 = ((osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst < 0)) << 7);
    int val = c0 | c2 | c4 | c6 | c7 | c11;
    return val;
}
int x86Internal::get_EFLAGS() {
    int bits = compile_flags();
    bits |= df & 0x00000400;
    bits |= eflags;
    return bits;
}
void x86Internal::set_EFLAGS(int bits, int mask) {
    osm_src = bits & (0x0800 | 0x0080 | 0x0040 | 0x0010 | 0x0004 | 0x0001);
    osm_dst = ((osm_src >> 6) & 1) ^ 1;
    osm = 24;
    df = 1 - (2 * ((bits >> 10) & 1));
    eflags = (eflags & ~mask) | (bits & mask);
}
