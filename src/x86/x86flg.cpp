#include "x86.h"

bool x86Internal::check_carry() {
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
bool x86Internal::check_overflow() {
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
bool x86Internal::check_below_or_equal() { // `below' for signed comparison, PM p. 317
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
        flg = check_carry() | (osm_dst == 0);
        break;
    }
    return flg;
}
int x86Internal::check_parity() {
    if (osm == 24) {
        return (osm_src >> 2) & 1;
    } else {
        return parity_LUT[osm_dst & 0xff];
    }
}
int x86Internal::check_less_than() {
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
        flg = (osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst < 0)) ^ check_overflow();
        break;
    }
    return flg;
}
int x86Internal::check_less_or_equal() { // `less' for unsigned comparison, PM p. 317
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
        flg = ((osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst < 0)) ^ check_overflow()) | (osm_dst == 0);
        break;
    }
    return flg;
}
int x86Internal::check_adjust_flag() {
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
int x86Internal::check_status_bits_for_jump(int gd) {
    bool flg;
    switch ((gd >> 1) & 7) {
    case 0:
        flg = check_overflow();
        break;
    case 1:
        flg = check_carry();
        break;
    case 2:
        flg = (osm_dst == 0);
        break;
    case 3:
        flg = check_below_or_equal();
        break;
    case 4:
        flg = (osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst < 0));
        break;
    case 5:
        flg = check_parity();
        break;
    case 6:
        flg = check_less_than();
        break;
    case 7:
        flg = check_less_or_equal();
        break;
    }
    return flg ^ (gd & 1);
}
int x86Internal::conditional_flags_for_rot_shiftcc_ops() {
    // int c0  = (check_carry() << 0);
    int c2 = (check_parity() << 2);
    int c4 = check_adjust_flag();
    int c6 = ((osm_dst == 0) << 6);
    int c7 = ((osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst < 0)) << 7);
    // int c11 = (check_overflow() << 11);
    int val = c2 | c4 | c6 | c7;
    return val;
}
int x86Internal::get_conditional_flags() {
    int c0 = (check_carry() << 0);
    int c2 = (check_parity() << 2);
    int c4 = check_adjust_flag();
    int c6 = ((osm_dst == 0) << 6);
    int c7 = ((osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst < 0)) << 7);
    int c11 = (check_overflow() << 11);
    int val = c0 | c2 | c4 | c6 | c7 | c11;
    return val;
}
int x86Internal::get_FLAGS() {
    int flag_bits = get_conditional_flags();
    flag_bits |= df & 0x00000400;
    flag_bits |= eflags;
    return flag_bits;
}
void x86Internal::set_FLAGS(int flag_bits, int ld) {
    osm_src = flag_bits & (0x0800 | 0x0080 | 0x0040 | 0x0010 | 0x0004 | 0x0001);
    osm_dst = ((osm_src >> 6) & 1) ^ 1;
    osm = 24;
    df = 1 - (2 * ((flag_bits >> 10) & 1));
    eflags = (eflags & ~ld) | (flag_bits & ld);
}
