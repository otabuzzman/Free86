#include "free86.h"

bool Free86::is_CF() {
    bool f;
    int osm;
    uint32_t osm_dst;
    if (this->osm >= 25) {
        osm = ocm_preserved;
        osm_dst = ocm_dst_preserved;
    } else {
        osm = this->osm;
        osm_dst = this->osm_dst;
    }
    switch (osm % 25) {
    case 0:
        f = (osm_dst & 0xff) < (osm_src & 0xff);
        break;
    case 1:
        f = (osm_dst & 0xffff) < (osm_src & 0xffff);
        break;
    case 2:
        f = osm_dst < osm_src;
        break;
    case 3:
        f = (osm_dst & 0xff) <= (osm_src & 0xff);
        break;
    case 4:
        f = (osm_dst & 0xffff) <= (osm_src & 0xffff);
        break;
    case 5:
        f = osm_dst <= osm_src;
        break;
    case 6:
        f = ((osm_dst + osm_src) & 0xff) < (osm_src & 0xff);
        break;
    case 7:
        f = ((osm_dst + osm_src) & 0xffff) < (osm_src & 0xffff);
        break;
    case 8:
        f = (osm_dst + osm_src) < osm_src;
        break;
    case 9:
        f = ((osm_dst + osm_src + 1) & 0xff) <= (osm_src & 0xff);
        break;
    case 10:
        f = ((osm_dst + osm_src + 1) & 0xffff) <= (osm_src & 0xffff);
        break;
    case 11:
        f = ((osm_dst + osm_src + 1) <= osm_src);
        break;
    case 12:
    case 13:
    case 14:
        f = false;
        break;
    case 15:
        f = (osm_src >> 7) & 1;
        break;
    case 16:
        f = (osm_src >> 15) & 1;
        break;
    case 17:
        f = (osm_src >> 31) & 1;
        break;
    case 18:
    case 19:
    case 20:
        f = osm_src & 1;
        break;
    case 21:
    case 22:
    case 23:
        f = osm_src != 0;
        break;
    case 24:
        f = osm_src & 1;
        break;
    default:
        f = false;
        break;
    }
    return f;
}
bool Free86::is_PF() {
    if (osm == 24) {
        return (osm_src >> 2) & 1;
    } else {
        return parity_LUT[osm_dst & 0xff];
    }
}
bool Free86::is_AF() {
    bool f;
    switch (osm % 0x1f) {
    case 0:
    case 1:
    case 2:
        u = osm_dst - osm_src;
        f = (osm_dst ^ u ^ osm_src) & 0x10;
        break;
    case 3:
    case 4:
    case 5:
        u = osm_dst - osm_src - 1;
        f = (osm_dst ^ u ^ osm_src) & 0x10;
        break;
    case 6:
    case 7:
    case 8:
        u = osm_dst + osm_src;
        f = (osm_dst ^ u ^ osm_src) & 0x10;
        break;
    case 9:
    case 10:
    case 11:
        u = osm_dst + osm_src + 1;
        f = (osm_dst ^ u ^ osm_src) & 0x10;
        break;
    case 12:
    case 13:
    case 14:
        f = false ;
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
        f = false;
        break;
    case 24:
        f = osm_src & 0x10;
        break;
    case 25:
    case 26:
    case 27:
        f = (osm_dst ^ (osm_dst - 1)) & 0x10;
        break;
    case 28:
    case 29:
    case 30:
        f = (osm_dst ^ (osm_dst + 1)) & 0x10;
        break;
    default:
        f = false;
        break;
    }
    return f;
}
bool Free86::is_OF() {
    bool f;
    switch (osm % 0x1f) {
    case 0:
        u = osm_dst - osm_src;
        f = (((u ^ osm_src ^ 0xffffffff) & (u ^ osm_dst)) >> 7) & 1;
        break;
    case 1:
        u = osm_dst - osm_src;
        f = (((u ^ osm_src ^ 0xffffffff) & (u ^ osm_dst)) >> 15) & 1;
        break;
    case 2:
        u = osm_dst - osm_src;
        f = (((u ^ osm_src ^ 0xffffffff) & (u ^ osm_dst)) >> 31) & 1;
        break;
    case 3:
        u = osm_dst - osm_src - 1;
        f = (((u ^ osm_src ^ 0xffffffff) & (u ^ osm_dst)) >> 7) & 1;
        break;
    case 4:
        u = osm_dst - osm_src - 1;
        f = (((u ^ osm_src ^ 0xffffffff) & (u ^ osm_dst)) >> 15) & 1;
        break;
    case 5:
        u = osm_dst - osm_src - 1;
        f = (((u ^ osm_src ^ 0xffffffff) & (u ^ osm_dst)) >> 31) & 1;
        break;
    case 6:
        u = osm_dst + osm_src;
        f = (((u ^ osm_src) & (u ^ osm_dst)) >> 7) & 1;
        break;
    case 7:
        u = osm_dst + osm_src;
        f = (((u ^ osm_src) & (u ^ osm_dst)) >> 15) & 1;
        break;
    case 8:
        u = osm_dst + osm_src;
        f = (((u ^ osm_src) & (u ^ osm_dst)) >> 31) & 1;
        break;
    case 9:
        u = osm_dst + osm_src + 1;
        f = (((u ^ osm_src) & (u ^ osm_dst)) >> 7) & 1;
        break;
    case 10:
        u = osm_dst + osm_src + 1;
        f = (((u ^ osm_src) & (u ^ osm_dst)) >> 15) & 1;
        break;
    case 11:
        u = osm_dst + osm_src + 1;
        f = (((u ^ osm_src) & (u ^ osm_dst)) >> 31) & 1;
        break;
    case 12:
    case 13:
    case 14:
        f = false;
        break;
    case 15:
    case 18:
        f = ((osm_src ^ osm_dst) >> 7) & 1;
        break;
    case 16:
    case 19:
        f = ((osm_src ^ osm_dst) >> 15) & 1;
        break;
    case 17:
    case 20:
        f = ((osm_src ^ osm_dst) >> 31) & 1;
        break;
    case 21:
    case 22:
    case 23:
        f = osm_src != 0;
        break;
    case 24:
        f = (osm_src >> 11) & 1;
        break;
    case 25:
        f = (osm_dst & 0xff) == 0x80;
        break;
    case 26:
        f = (osm_dst & 0xffff) == 0x8000;
        break;
    case 27:
        f = osm_dst == 0x80000000;
        break;
    case 28:
        f = (osm_dst & 0xff) == 0x7f;
        break;
    case 29:
        f = (osm_dst & 0xffff) == 0x7fff;
        break;
    case 30:
        f = osm_dst == 0x7fffffff;
        break;
    default:
        f = false;
        break;
    }
    return f;
}
bool Free86::is_BE() { // `below' for signed comparison, PM p. 317
    bool f;
    switch (osm) {
    case 6:
        f = ((osm_dst + osm_src) & 0xff) <= (osm_src & 0xff);
        break;
    case 7:
        f = ((osm_dst + osm_src) & 0xffff) <= (osm_src & 0xffff);
        break;
    case 8:
        f = (osm_dst + osm_src) <= osm_src;
        break;
    case 24:
        f = (osm_src & (0x0040 | 0x0001)) != 0;
        break;
    default:
        f = is_CF() || (osm_dst == 0);
        break;
    }
    return f;
}
bool Free86::is_LE() { // `less' for unsigned comparison, PM p. 317
    int osm_dst = static_cast<int>(this->osm_dst); // signed arithmetic easier here
    int osm_src = static_cast<int>(this->osm_src);
    bool f;
    switch (osm) {
    case 6:
        f = ((osm_dst + osm_src) << 24) <= (osm_src << 24);
        break;
    case 7:
        f = ((osm_dst + osm_src) << 16) <= (osm_src << 16);
        break;
    case 8:
        f = (osm_dst + osm_src) <= osm_src;
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
        f = osm_dst <= 0;
        break;
    case 24:
        f = (((osm_src >> 7) ^ (osm_src >> 11)) | (osm_src >> 6)) & 1;
        break;
    default:
        f = ((osm_dst < 0) ^ is_OF()) | (osm_dst == 0);
        break;
    }
    return f;
}
bool Free86::is_LT() {
    int osm_dst = static_cast<int>(this->osm_dst); // signed arithmetic easier here
    int osm_src = static_cast<int>(this->osm_src);
    bool f;
    switch (osm) {
    case 6:
        f = ((osm_dst + osm_src) << 24) < (osm_src << 24);
        break;
    case 7:
        f = ((osm_dst + osm_src) << 16) < (osm_src << 16);
        break;
    case 8:
        f = (osm_dst + osm_src) < osm_src;
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
        f = osm_dst < 0;
        break;
    case 24:
        f = ((osm_src >> 7) ^ (osm_src >> 11)) & 1;
        break;
    default:
        f = (osm_dst < 0) ^ is_OF();
        break;
    }
    return f;
}
bool Free86::can_jump(int condition) {
    bool f;
    switch ((condition >> 1) & 7) {
    case 0:
        f = is_OF();
        break;
    case 1:
        f = is_CF();
        break;
    case 2:
        f = osm_dst == 0;
        break;
    case 3:
        f = is_BE();
        break;
    case 4:
        f = osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst & 0x80000000 ? 1 : 0);
        break;
    case 5:
        f = is_PF();
        break;
    case 6:
        f = is_LT();
        break;
    case 7:
        f = is_LE();
        break;
    default:
        f = false;
        break;
    }
    return f ^ (condition & 1);
}
uint32_t Free86::compile_eflags(bool shift) {
    uint32_t f0 = 0, f11 = 0;
    if (!shift) {
        f0 = is_CF() << 0;
        f11 = is_OF() << 11;
    }
    uint32_t f2 = is_PF() << 2;
    uint32_t f4 = is_AF() << 4;
    uint32_t f6 = (osm_dst == 0) << 6;
    uint32_t f7 = (osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst & 0x80000000 ? 1 : 0)) << 7;
    return f0 | f2 | f4 | f6 | f7 | f11;
}
uint32_t Free86::get_EFLAGS() {
    uint32_t bits = compile_eflags();
    bits |= df & 0x00000400;
    bits |= eflags;
    return bits;
}
void Free86::set_EFLAGS(uint32_t bits, uint32_t mask) {
    osm_src = bits & (0x0800 | 0x0080 | 0x0040 | 0x0010 | 0x0004 | 0x0001);
    osm_dst = ((osm_src >> 6) & 1) ^ 1;
    osm = 24;
    df = 1 - (2 * ((bits >> 10) & 1));
    eflags = (eflags & ~mask) | (bits & mask);
}
