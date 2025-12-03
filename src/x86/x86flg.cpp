#include "x86.h"

bool x86Internal::is_CF() {
    bool r = false;
    int _ocm;
    uint32_t _ocm_dst;
    if (osm >= 25) {
        _ocm = ocm_preserved;
        _ocm_dst = ocm_dst_preserved;
    } else {
        _ocm = osm;
        _ocm_dst = osm_dst;
    }
    switch (_ocm % 25) {
    case 0:
        r = (_ocm_dst & 0xff) < (osm_src & 0xff);
        break;
    case 1:
        r = (_ocm_dst & 0xffff) < (osm_src & 0xffff);
        break;
    case 2:
        r = _ocm_dst < osm_src;
        break;
    case 3:
        r = (_ocm_dst & 0xff) <= (osm_src & 0xff);
        break;
    case 4:
        r = (_ocm_dst & 0xffff) <= (osm_src & 0xffff);
        break;
    case 5:
        r = _ocm_dst <= osm_src;
        break;
    case 6:
        r = ((_ocm_dst + osm_src) & 0xff) < (osm_src & 0xff);
        break;
    case 7:
        r = ((_ocm_dst + osm_src) & 0xffff) < (osm_src & 0xffff);
        break;
    case 8:
        r = (_ocm_dst + osm_src) < osm_src;
        break;
    case 9:
        r = ((_ocm_dst + osm_src + 1) & 0xff) <= (osm_src & 0xff);
        break;
    case 10:
        r = ((_ocm_dst + osm_src + 1) & 0xffff) <= (osm_src & 0xffff);
        break;
    case 11:
        r = ((_ocm_dst + osm_src + 1) <= osm_src);
        break;
    case 12:
    case 13:
    case 14:
        r = 0;
        break;
    case 15:
        r = (osm_src >> 7) & 1;
        break;
    case 16:
        r = (osm_src >> 15) & 1;
        break;
    case 17:
        r = (osm_src >> 31) & 1;
        break;
    case 18:
    case 19:
    case 20:
        r = osm_src & 1;
        break;
    case 21:
    case 22:
    case 23:
        r = osm_src != 0;
        break;
    case 24:
        r = osm_src & 1;
        break;
    }
    return r;
}
int x86Internal::is_PF() {
    if (osm == 24) {
        return (osm_src >> 2) & 1;
    } else {
        return parity_LUT[osm_dst & 0xff];
    }
}
int x86Internal::is_AF() {
    int x, r = 0;
    switch (osm % 0x1f) {
    case 0:
    case 1:
    case 2:
        x = osm_dst - osm_src;
        r = (osm_dst ^ x ^ osm_src) & 0x10;
        break;
    case 3:
    case 4:
    case 5:
        x = osm_dst - osm_src - 1;
        r = (osm_dst ^ x ^ osm_src) & 0x10;
        break;
    case 6:
    case 7:
    case 8:
        x = osm_dst + osm_src;
        r = (osm_dst ^ x ^ osm_src) & 0x10;
        break;
    case 9:
    case 10:
    case 11:
        x = osm_dst + osm_src + 1;
        r = (osm_dst ^ x ^ osm_src) & 0x10;
        break;
    case 12:
    case 13:
    case 14:
        r = 0;
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
        r = 0;
        break;
    case 24:
        r = osm_src & 0x10;
        break;
    case 25:
    case 26:
    case 27:
        r = (osm_dst ^ (osm_dst - 1)) & 0x10;
        break;
    case 28:
    case 29:
    case 30:
        r = (osm_dst ^ (osm_dst + 1)) & 0x10;
        break;
    }
    return r;
}
bool x86Internal::is_OF() {
    bool r = false;
    int x;
    switch (osm % 0x1f) {
    case 0:
        x = osm_dst - osm_src;
        r = (((x ^ osm_src ^ -1) & (x ^ osm_dst)) >> 7) & 1;
        break;
    case 1:
        x = osm_dst - osm_src;
        r = (((x ^ osm_src ^ -1) & (x ^ osm_dst)) >> 15) & 1;
        break;
    case 2:
        x = osm_dst - osm_src;
        r = (((x ^ osm_src ^ -1) & (x ^ osm_dst)) >> 31) & 1;
        break;
    case 3:
        x = osm_dst - osm_src - 1;
        r = (((x ^ osm_src ^ -1) & (x ^ osm_dst)) >> 7) & 1;
        break;
    case 4:
        x = osm_dst - osm_src - 1;
        r = (((x ^ osm_src ^ -1) & (x ^ osm_dst)) >> 15) & 1;
        break;
    case 5:
        x = osm_dst - osm_src - 1;
        r = (((x ^ osm_src ^ -1) & (x ^ osm_dst)) >> 31) & 1;
        break;
    case 6:
        x = osm_dst + osm_src;
        r = (((x ^ osm_src) & (x ^ osm_dst)) >> 7) & 1;
        break;
    case 7:
        x = osm_dst + osm_src;
        r = (((x ^ osm_src) & (x ^ osm_dst)) >> 15) & 1;
        break;
    case 8:
        x = osm_dst + osm_src;
        r = (((x ^ osm_src) & (x ^ osm_dst)) >> 31) & 1;
        break;
    case 9:
        x = osm_dst + osm_src + 1;
        r = (((x ^ osm_src) & (x ^ osm_dst)) >> 7) & 1;
        break;
    case 10:
        x = osm_dst + osm_src + 1;
        r = (((x ^ osm_src) & (x ^ osm_dst)) >> 15) & 1;
        break;
    case 11:
        x = osm_dst + osm_src + 1;
        r = (((x ^ osm_src) & (x ^ osm_dst)) >> 31) & 1;
        break;
    case 12:
    case 13:
    case 14:
        r = 0;
        break;
    case 15:
    case 18:
        r = ((osm_src ^ osm_dst) >> 7) & 1;
        break;
    case 16:
    case 19:
        r = ((osm_src ^ osm_dst) >> 15) & 1;
        break;
    case 17:
    case 20:
        r = ((osm_src ^ osm_dst) >> 31) & 1;
        break;
    case 21:
    case 22:
    case 23:
        r = osm_src != 0;
        break;
    case 24:
        r = (osm_src >> 11) & 1;
        break;
    case 25:
        r = (osm_dst & 0xff) == 0x80;
        break;
    case 26:
        r = (osm_dst & 0xffff) == 0x8000;
        break;
    case 27:
        r = osm_dst == -2147483648;
        break;
    case 28:
        r = (osm_dst & 0xff) == 0x7f;
        break;
    case 29:
        r = (osm_dst & 0xffff) == 0x7fff;
        break;
    case 30:
        r = osm_dst == 0x7fffffff;
        break;
    }
    return r;
}
bool x86Internal::is_BE() { // `below' for signed comparison, PM p. 317
    bool r;
    switch (osm) {
    case 6:
        r = ((osm_dst + osm_src) & 0xff) <= (osm_src & 0xff);
        break;
    case 7:
        r = ((osm_dst + osm_src) & 0xffff) <= (osm_src & 0xffff);
        break;
    case 8: {
        uint32_t val = osm_dst + osm_src;
        r = val <= osm_src;
    } break;
    case 24:
        r = (osm_src & (0x0040 | 0x0001)) != 0;
        break;
    default:
        r = is_CF() | (osm_dst == 0);
        break;
    }
    return r;
}
int x86Internal::is_LE() { // `less' for unsigned comparison, PM p. 317
    bool r;
    switch (osm) {
    case 6:
        r = ((osm_dst + osm_src) << 24) <= (osm_src << 24);
        break;
    case 7:
        r = ((osm_dst + osm_src) << 16) <= (osm_src << 16);
        break;
    case 8:
        r = (osm_dst + osm_src) <= osm_src;
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
        r = osm_dst <= 0;
        break;
    case 24:
        r = (((osm_src >> 7) ^ (osm_src >> 11)) | (osm_src >> 6)) & 1;
        break;
    default:
        r = (((osm_dst < 0)) ^ is_OF()) | (osm_dst == 0);
        break;
    }
    return r;
}
int x86Internal::is_LT() {
    bool r;
    switch (osm) {
    case 6:
        r = ((osm_dst + osm_src) << 24) < (osm_src << 24);
        break;
    case 7:
        r = ((osm_dst + osm_src) << 16) < (osm_src << 16);
        break;
    case 8:
        r = (osm_dst + osm_src) < osm_src;
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
        r = osm_dst < 0;
        break;
    case 24:
        r = ((osm_src >> 7) ^ (osm_src >> 11)) & 1;
        break;
    default:
        r = ((osm_dst < 0)) ^ is_OF();
        break;
    }
    return r;
}
int x86Internal::can_jump(int condition) {
    bool r = false;
    switch ((condition >> 1) & 7) {
    case 0:
        r = is_OF();
        break;
    case 1:
        r = is_CF();
        break;
    case 2:
        r = osm_dst == 0;
        break;
    case 3:
        r = is_BE();
        break;
    case 4:
        r = osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst < 0);
        break;
    case 5:
        r = is_PF();
        break;
    case 6:
        r = is_LT();
        break;
    case 7:
        r = is_LE();
        break;
    }
    return r ^ (condition & 1);
}
int x86Internal::compile_flags(bool shift) {
    int f0 = 0, f11 = 0;
    if (!shift) {
        f0 = is_CF() << 0;
        f11 = is_OF() << 11;
    }
    int f2 = is_PF() << 2;
    int f4 = is_AF();
    int f6 = (osm_dst == 0) << 6;
    int f7 = (osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst < 0)) << 7;
    return f0 | f2 | f4 | f6 | f7 | f11;
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
