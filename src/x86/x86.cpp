#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <sys/types.h>

#include "x86.h"

x86Internal::x86Internal(int mem_size) {
    this->mem_size = mem_size;
    // size plus maximum possible number of bytes per instruction,
    // rounded up to the nearest multiple of 32 bits, as buffer
    // for instructions that span page boundaries.
    phys_mem8 = (uint8_t *)calloc(1, mem_size + ((15 + 3) & ~3));
    phys_mem16 = reinterpret_cast<uint16_t *>(phys_mem8);
    phys_mem32 = reinterpret_cast<uint32_t *>(phys_mem8);
    tlb_read_kernel = new int[tlb_size];
    tlb_write_kernel = new int[tlb_size];
    tlb_read_user = new int[tlb_size];
    tlb_write_user = new int[tlb_size];
    for (int i = 0; i < tlb_size; i++) {
        tlb_clear(i);
    }
    reset(); // chip
    set_current_permission_level(0); // PM (1986), 10.3
}
x86Internal::~x86Internal() {
    free(phys_mem8);
    delete[] tlb_read_kernel;
    delete[] tlb_write_kernel;
    delete[] tlb_read_user;
    delete[] tlb_write_user;
}
void x86Internal::update_SSB() {
    CS_base = segs[1].base;
    if (segs[1].flags & (1 << 22)) {
        ipr_default = 0;
    } else {
        ipr_default = 0x0100 | 0x0080;
    }
    SS_base = segs[2].base;
    if (segs[2].flags & (1 << 22)) {
        SS_mask = -1;
    } else {
        SS_mask = 0xffff;
    }
    x86_64_long_mode = (((segs[0].base | CS_base | SS_base | segs[3].base) == 0) && SS_mask == -1);
}
void x86Internal::check_opcode() {
    eip = eip + far - far_start;
    eip_linear = check_real__v86() ? (eip + CS_base) & 0xfffff : eip + CS_base;
    int64_t eip_tlb_hash = tlb_read[eip_linear >> 12];
    // `eip_tlb_hash' equals -1 or instruction with maximum bytes (15)
    // would extend across the page boundary.
    // combined check ok because bits 0-11 in `eip_tlb_hash' always 0.
    if (((eip_tlb_hash | eip_linear) & 0xfff) > (4096 - 15)) {
        if (eip_tlb_hash == -1) {
            do_tlb_set_page(eip_linear, 0, cpl == 3);
        }
        eip_tlb_hash = tlb_read[eip_linear >> 12];
        far = far_start = eip_linear ^ eip_tlb_hash;
        opcode = phys_mem8[far++];
        int page_offset = eip_linear & 0xfff;
        if (page_offset > (4096 - 15)) {
            x = instruction_length(opcode, eip_linear);
            if ((page_offset + x) > 4096) { // instruction extends page boundary
                far = far_start = mem_size;
                for (y = 0; y < x; y++) { // copy instruction to dedicated buffer on top of memory
                    mem8_loc = eip_linear + y;
                    phys_mem8[far + y] = (((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                             ? __ld_8bits_mem8_read()
                             : phys_mem8[mem8_loc ^ tlb_hash]);
                }
                far++;
            }
        }
    } else {
        far = far_start = eip_linear ^ eip_tlb_hash;
        opcode = phys_mem8[far++];
    }
}
void x86Internal::do_tlb_set_page(int linear_address, int write, bool user) {
    int pde_address, pde, pte_address, pte, pxe, tlb_set_write = 0, tlb_set_user = 1, clean, error_code;
    if (!(cr0 & (1 << 31))) {
        tlb_set_write = 1;
        tlb_set_user = 0;
        tlb_set_page(linear_address & -4096, linear_address & -4096, tlb_set_write, tlb_set_user);
    } else { // paging enabled
        pde_address = (cr3 & -4096) + ((linear_address >> 20) & 0xffc);
        pde = ld32_phys(pde_address);
        if (!(pde & 0x00000001)) { // page not present
            error_code = 0;
        } else {
            pte_address = (pde & -4096) + ((linear_address >> 10) & 0xffc);
            pte = ld32_phys(pte_address);
            if (!(pte & 0x00000001)) { // page not present
                error_code = 0;
            } else {
                pxe = pde & pte;
                if (user && !(pxe & 0x00000004)) { // user request and page supervisor
                    error_code = 0x01;
                // user request or RF set and WR request and RO page
                } else if ((user || (cr0 & (1 << 16))) && write && !(pxe & 0x00000002)) {
                    error_code = 0x01;
                } else {
                    if (!(pde & 0x00000020)) { // page not accessed
                        pde |= 0x00000020;
                        st32_phys(pde_address, pde);
                    }
                    clean = (write && !(pte & 0x00000040)); // WR request and page not dirty
                    if (!(pte & 0x00000020) || clean) {  // page not accessed or previous
                        pte |= 0x00000020;     // set page accessed
                        if (clean) {
                            pte |= 0x00000040; // set page dirty
                        }
                        st32_phys(pte_address, pte);
                    }
                    // page dirty and supervisor request and page not RO
                    if ((pte & 0x00000040) && (!user || (pxe & 0x00000002))) {
                        tlb_set_write = 1;
                    }
                    // page not supervisor
                    if (pxe & 0x00000004) {
                        tlb_set_user = 1;
                    }
                    tlb_set_page(linear_address & -4096, pte & -4096, tlb_set_write, tlb_set_user);
                    return;
                }
            }
        }
        error_code |= write << 1;
        if (user) {
            error_code |= 0x04;
        }
        cr2 = linear_address;
        abort(14, error_code);
    }
}
int x86Internal::segment_translation(int modRM) {
    int base, mem8_loc, Qb, Rb, Sb, Tb;
    if (x86_64_long_mode && (ipr & (0x000f | 0x0080)) == 0) {
        switch ((modRM & 7) | ((modRM >> 3) & 0x18)) {
        case 0x04:
            Qb = phys_mem8[far++];
            base = Qb & 7;
            if (base == 5) {
                mem8_loc = phys_mem8[far] |
                           (phys_mem8[far + 1] << 8) |
                           (phys_mem8[far + 2] << 16) |
                           (phys_mem8[far + 3] << 24);
                far += 4;
            } else {
                mem8_loc = regs[base];
            }
            Rb = (Qb >> 3) & 7;
            if (Rb != 4) {
                mem8_loc = mem8_loc + (regs[Rb] << (Qb >> 6));
            }
            break;
        case 0x0c:
            Qb = phys_mem8[far++];
            mem8_loc = ((phys_mem8[far++] << 24) >> 24);
            base = Qb & 7;
            mem8_loc = mem8_loc + regs[base];
            Rb = (Qb >> 3) & 7;
            if (Rb != 4) {
                mem8_loc = mem8_loc + (regs[Rb] << (Qb >> 6));
            }
            break;
        case 0x14:
            Qb = phys_mem8[far++];
            mem8_loc = phys_mem8[far] |
                       (phys_mem8[far + 1] << 8) |
                       (phys_mem8[far + 2] << 16) |
                       (phys_mem8[far + 3] << 24);
            far += 4;
            base = Qb & 7;
            mem8_loc = mem8_loc + regs[base];
            Rb = (Qb >> 3) & 7;
            if (Rb != 4) {
                mem8_loc = mem8_loc + (regs[Rb] << (Qb >> 6));
            }
            break;
        case 0x05:
            mem8_loc = phys_mem8[far] |
                       (phys_mem8[far + 1] << 8) |
                       (phys_mem8[far + 2] << 16) |
                       (phys_mem8[far + 3] << 24);
            far += 4;
            break;
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x06:
        case 0x07:
            base = modRM & 7;
            mem8_loc = regs[base];
            break;
        case 0x08:
        case 0x09:
        case 0x0a:
        case 0x0b:
        case 0x0d:
        case 0x0e:
        case 0x0f:
            mem8_loc = ((phys_mem8[far++] << 24) >> 24);
            base = modRM & 7;
            mem8_loc = mem8_loc + regs[base];
            break;
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x15:
        case 0x16:
        case 0x17:
        default:
            mem8_loc = phys_mem8[far] |
                       (phys_mem8[far + 1] << 8) |
                       (phys_mem8[far + 2] << 16) |
                       (phys_mem8[far + 3] << 24);
            far += 4;
            base = modRM & 7;
            mem8_loc = mem8_loc + regs[base];
            break;
        }
        return mem8_loc;
    } else if (ipr & 0x0080) {
        if ((modRM & 0xc7) == 0x06) {
            mem8_loc = ld16_mem8_direct();
            Tb = 3;
        } else {
            switch (modRM >> 6) {
            case 0:
                mem8_loc = 0;
                break;
            case 1:
                mem8_loc = ((phys_mem8[far++] << 24) >> 24);
                break;
            default:
                mem8_loc = ld16_mem8_direct();
                break;
            }
            switch (modRM & 7) {
            case 0:
                mem8_loc = (mem8_loc + regs[3] + regs[6]) & 0xffff;
                Tb = 3;
                break;
            case 1:
                mem8_loc = (mem8_loc + regs[3] + regs[7]) & 0xffff;
                Tb = 3;
                break;
            case 2:
                mem8_loc = (mem8_loc + regs[5] + regs[6]) & 0xffff;
                Tb = 2;
                break;
            case 3:
                mem8_loc = (mem8_loc + regs[5] + regs[7]) & 0xffff;
                Tb = 2;
                break;
            case 4:
                mem8_loc = (mem8_loc + regs[6]) & 0xffff;
                Tb = 3;
                break;
            case 5:
                mem8_loc = (mem8_loc + regs[7]) & 0xffff;
                Tb = 3;
                break;
            case 6:
                mem8_loc = (mem8_loc + regs[5]) & 0xffff;
                Tb = 2;
                break;
            case 7:
            default:
                mem8_loc = (mem8_loc + regs[3]) & 0xffff;
                Tb = 3;
                break;
            }
        }
        Sb = ipr & 0x000f;
        if (Sb == 0) {
            Sb = Tb;
        } else {
            Sb--;
        }
        mem8_loc = mem8_loc + segs[Sb].base;
        return mem8_loc;
    } else {
        switch ((modRM & 7) | ((modRM >> 3) & 0x18)) {
        case 0x04:
            Qb = phys_mem8[far++];
            base = Qb & 7;
            if (base == 5) {
                mem8_loc = phys_mem8[far] |
                           (phys_mem8[far + 1] << 8) |
                           (phys_mem8[far + 2] << 16) |
                           (phys_mem8[far + 3] << 24);
                far += 4;
                base = 0;
            } else {
                mem8_loc = regs[base];
            }
            Rb = (Qb >> 3) & 7;
            if (Rb != 4) {
                mem8_loc = mem8_loc + (regs[Rb] << (Qb >> 6));
            }
            break;
        case 0x0c:
            Qb = phys_mem8[far++];
            mem8_loc = ((phys_mem8[far++] << 24) >> 24);
            base = Qb & 7;
            mem8_loc = mem8_loc + regs[base];
            Rb = (Qb >> 3) & 7;
            if (Rb != 4) {
                mem8_loc = mem8_loc + (regs[Rb] << (Qb >> 6));
            }
            break;
        case 0x14:
            Qb = phys_mem8[far++];
            mem8_loc = phys_mem8[far] |
                       (phys_mem8[far + 1] << 8) |
                       (phys_mem8[far + 2] << 16) |
                       (phys_mem8[far + 3] << 24);
            far += 4;
            base = Qb & 7;
            mem8_loc = mem8_loc + regs[base];
            Rb = (Qb >> 3) & 7;
            if (Rb != 4) {
                mem8_loc = mem8_loc + (regs[Rb] << (Qb >> 6));
            }
            break;
        case 0x05:
            mem8_loc = phys_mem8[far] |
                       (phys_mem8[far + 1] << 8) |
                       (phys_mem8[far + 2] << 16) |
                       (phys_mem8[far + 3] << 24);
            far += 4;
            base = 0;
            break;
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x06:
        case 0x07:
            base = modRM & 7;
            mem8_loc = regs[base];
            break;
        case 0x08:
        case 0x09:
        case 0x0a:
        case 0x0b:
        case 0x0d:
        case 0x0e:
        case 0x0f: // 2-byte instruction escape
            mem8_loc = ((phys_mem8[far++] << 24) >> 24);
            base = modRM & 7;
            mem8_loc = mem8_loc + regs[base];
            break;
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x15:
        case 0x16:
        case 0x17:
        default:
            mem8_loc = phys_mem8[far] |
                       (phys_mem8[far + 1] << 8) |
                       (phys_mem8[far + 2] << 16) |
                       (phys_mem8[far + 3] << 24);
            far += 4;
            base = modRM & 7;
            mem8_loc = mem8_loc + regs[base];
            break;
        }
        Sb = ipr & 0x000f;
        if (Sb == 0) {
            if (base == 4 || base == 5) {
                Sb = 2;
            } else {
                Sb = 3;
            }
        } else {
            Sb--;
        }
        mem8_loc = mem8_loc + segs[Sb].base;
        return mem8_loc;
    }
    return 0;
}
int x86Internal::convert_offset_to_linear(bool writable) {
    uint64_t mem8_loc;
    int Sb, Ls, Tc, Lc;
    if (ipr & 0x0080) {
        mem8_loc = ld16_mem8_direct() & 0xffff;
        Ls = 1; // 16 bit mode
    } else {
        mem8_loc = phys_mem8[far] |
                   (phys_mem8[far + 1] << 8) |
                   (phys_mem8[far + 2] << 16) |
                   (phys_mem8[far + 3] << 24) & 0xffffffff;
        far += 4;
        Ls = 3; // 32 bit mode
    }
    if (!(opcode & 0x01)) {
        Ls = 0; // byte mode, opcodes A0, A2
    }
    Sb = ipr & 0x000f;
    if (Sb == 0) {
        Sb = 3;
    } else {
        Sb--;
    }
    // type checking
    if (Sb == 1) { // CS
        Tc = writable || !(segs[Sb].flags & (1 << 9));
    } else { // data segment
        Tc = writable && !(segs[Sb].flags & (1 << 9));
    }
    if (Tc) {
        abort(13, 0);
    }
    mem8_loc = segs[Sb].base + mem8_loc;
    // limit checking
    if (segs[Sb].flags & (1 << 10)) { // expand-down segment
        Lc = mem8_loc < (uint64_t)segs[Sb].base + segs[Sb].limit + 1;
    } else {
        Lc = mem8_loc > (uint64_t)segs[Sb].base + segs[Sb].limit - Ls;
    }
    if (Lc) {
        if (Sb == 2) {
            abort(12, 0); // #SS(0)
        } else {
            abort(13, 0); // #GP(0)
        }
    }
    return mem8_loc;
}
void x86Internal::set_lower_word_bytes(int reg_idx, int x) {
    if (reg_idx & 4) { // ESP, EBP, ESI, EDI: set AH, CH, DH, BH
        regs[reg_idx & 3] = (regs[reg_idx & 3] & -65281) | ((x & 0xff) << 8);
    } else { // set AL, CL, DL, BL
        regs[reg_idx & 3] = (regs[reg_idx & 3] & -256) | (x & 0xff);
    }
}
void x86Internal::set_lower_word(int reg_idx, int x) {
    regs[reg_idx] = (regs[reg_idx] & -65536) | (x & 0xffff);
}
int x86Internal::do_32bit_math(int operation, int Yb, int Zb) {
    int ac;
    switch (operation & 7) {
    case 0:
        osm_src = Zb;
        Yb = Yb + Zb;
        osm_dst = Yb;
        osm = 2;
        break;
    case 1:
        Yb = Yb | Zb;
        osm_dst = Yb;
        osm = 14;
        break;
    case 2:
        ac = is_CF();
        osm_src = Zb;
        Yb = Yb + Zb + ac;
        osm_dst = Yb;
        osm = ac ? 5 : 2;
        break;
    case 3:
        ac = is_CF();
        osm_src = Zb;
        Yb = Yb - Zb - ac;
        osm_dst = Yb;
        osm = ac ? 11 : 8;
        break;
    case 4:
        Yb = Yb & Zb;
        osm_dst = Yb;
        osm = 14;
        break;
    case 5:
        osm_src = Zb;
        Yb = Yb - Zb;
        osm_dst = Yb;
        osm = 8;
        break;
    case 6:
        Yb = Yb ^ Zb;
        osm_dst = Yb;
        osm = 14;
        break;
    case 7:
        osm_src = Zb;
        osm_dst = Yb - Zb;
        osm = 8;
        break;
    }
    return Yb;
}
int x86Internal::do_16bit_math(int operation, int Yb, int Zb) {
    int ac;
    switch (operation & 7) {
    case 0:
        osm_src = Zb;
        Yb = (((Yb + Zb) << 16) >> 16);
        osm_dst = Yb;
        osm = 1;
        break;
    case 1:
        Yb = (((Yb | Zb) << 16) >> 16);
        osm_dst = Yb;
        osm = 13;
        break;
    case 2:
        ac = is_CF();
        osm_src = Zb;
        Yb = (((Yb + Zb + ac) << 16) >> 16);
        osm_dst = Yb;
        osm = ac ? 4 : 1;
        break;
    case 3:
        ac = is_CF();
        osm_src = Zb;
        Yb = (((Yb - Zb - ac) << 16) >> 16);
        osm_dst = Yb;
        osm = ac ? 10 : 7;
        break;
    case 4:
        Yb = (((Yb & Zb) << 16) >> 16);
        osm_dst = Yb;
        osm = 13;
        break;
    case 5:
        osm_src = Zb;
        Yb = (((Yb - Zb) << 16) >> 16);
        osm_dst = Yb;
        osm = 7;
        break;
    case 6:
        Yb = (((Yb ^ Zb) << 16) >> 16);
        osm_dst = Yb;
        osm = 13;
        break;
    case 7:
        osm_src = Zb;
        osm_dst = (((Yb - Zb) << 16) >> 16);
        osm = 7;
        break;
    }
    return Yb;
}
int x86Internal::do_8bit_math(int operation, int Yb, int Zb) {
    int ac;
    switch (operation & 7) {
    case 0:
        osm_src = Zb;
        Yb = (((Yb + Zb) << 24) >> 24);
        osm_dst = Yb;
        osm = 0;
        break;
    case 1:
        Yb = (((Yb | Zb) << 24) >> 24);
        osm_dst = Yb;
        osm = 12;
        break;
    case 2:
        ac = is_CF();
        osm_src = Zb;
        Yb = (((Yb + Zb + ac) << 24) >> 24);
        osm_dst = Yb;
        osm = ac ? 3 : 0;
        break;
    case 3:
        ac = is_CF();
        osm_src = Zb;
        Yb = (((Yb - Zb - ac) << 24) >> 24);
        osm_dst = Yb;
        osm = ac ? 9 : 6;
        break;
    case 4:
        Yb = (((Yb & Zb) << 24) >> 24);
        osm_dst = Yb;
        osm = 12;
        break;
    case 5:
        osm_src = Zb;
        Yb = (((Yb - Zb) << 24) >> 24);
        osm_dst = Yb;
        osm = 6;
        break;
    case 6:
        Yb = (((Yb ^ Zb) << 24) >> 24);
        osm_dst = Yb;
        osm = 12;
        break;
    case 7:
        osm_src = Zb;
        osm_dst = (((Yb - Zb) << 24) >> 24);
        osm = 6;
        break;
    }
    return Yb;
}
int x86Internal::increment_16bit(int x) {
    if (osm < 25) {
        ocm_preserved = osm;
        ocm_dst_preserved = osm_dst;
    }
    osm_dst = (((x + 1) << 16) >> 16);
    osm = 26;
    return osm_dst;
}
int x86Internal::decrement_16bit(int x) {
    if (osm < 25) {
        ocm_preserved = osm;
        ocm_dst_preserved = osm_dst;
    }
    osm_dst = (((x - 1) << 16) >> 16);
    osm = 29;
    return osm_dst;
}
int x86Internal::increment_8bit(int x) {
    if (osm < 25) {
        ocm_preserved = osm;
        ocm_dst_preserved = osm_dst;
    }
    osm_dst = (((x + 1) << 24) >> 24);
    osm = 25;
    return osm_dst;
}
int x86Internal::decrement_8bit(int x) {
    if (osm < 25) {
        ocm_preserved = osm;
        ocm_dst_preserved = osm_dst;
    }
    osm_dst = (((x - 1) << 24) >> 24);
    osm = 28;
    return osm_dst;
}
int x86Internal::shift8(int operation, int Yb, int Zb) {
    int kc, ac;
    switch (operation & 7) {
    case 0:
        if (Zb & 0x1f) {
            Zb &= 0x7;
            Yb &= 0xff;
            kc = Yb;
            Yb = (Yb << Zb) | (Yb >> (8 - Zb));
            osm_src = compile_flags(true);
            osm_src |= (Yb & 0x0001);
            if (Zb == 1) {
                osm_src |= (((kc ^ Yb) << 4) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 1:
        if (Zb & 0x1f) {
            Zb &= 0x7;
            Yb &= 0xff;
            kc = Yb;
            Yb = (Yb >> Zb) | (Yb << (8 - Zb));
            osm_src = compile_flags(true);
            osm_src |= ((Yb >> 7) & 0x0001);
            if (Zb == 1) {
                osm_src |= (((kc ^ Yb) << 4) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 2:
        Zb = shift8_LUT[Zb & 0x1f];
        if (Zb) {
            Yb &= 0xff;
            kc = Yb;
            ac = is_CF();
            Yb = (Yb << Zb) | (ac << (Zb - 1));
            if (Zb > 1) {
                Yb |= kc >> (9 - Zb);
            }
            osm_src = compile_flags(true);
            osm_src |= ((kc >> (8 - Zb)) & 0x0001);
            if (Zb == 1) {
                osm_src |= (((kc ^ Yb) << 4) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 3:
        Zb = shift8_LUT[Zb & 0x1f];
        if (Zb) {
            Yb &= 0xff;
            kc = Yb;
            ac = is_CF();
            Yb = (Yb >> Zb) | (ac << (8 - Zb));
            if (Zb > 1) {
                Yb |= kc << (9 - Zb);
            }
            osm_src = compile_flags(true);
            osm_src |= ((kc >> (Zb - 1)) & 0x0001);
            if (Zb == 1) {
                osm_src |= (((kc ^ Yb) << 4) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 4:
    case 6:
        Zb &= 0x1f;
        if (Zb) {
            osm_src = Yb << (Zb - 1);
            osm_dst = Yb = (((Yb << Zb) << 24) >> 24);
            osm = 15;
        }
        break;
    case 5:
        Zb &= 0x1f;
        if (Zb) {
            Yb &= 0xff;
            osm_src = Yb >> (Zb - 1);
            osm_dst = Yb = (((Yb >> Zb) << 24) >> 24);
            osm = 18;
        }
        break;
    case 7:
        Zb &= 0x1f;
        if (Zb) {
            Yb = (Yb << 24) >> 24;
            osm_src = Yb >> (Zb - 1);
            osm_dst = Yb = (((Yb >> Zb) << 24) >> 24);
            osm = 18;
        }
        break;
    }
    return Yb;
}
int x86Internal::shift16(int operation, int Yb, int Zb) {
    int kc, ac;
    switch (operation & 7) {
    case 0:
        if (Zb & 0x1f) {
            Zb &= 0xf;
            Yb &= 0xffff;
            kc = Yb;
            Yb = (Yb << Zb) | (Yb >> (16 - Zb));
            osm_src = compile_flags(true);
            osm_src |= (Yb & 0x0001);
            if (Zb == 1) {
                osm_src |= (((kc ^ Yb) >> 4) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 1:
        if (Zb & 0x1f) {
            Zb &= 0xf;
            Yb &= 0xffff;
            kc = Yb;
            Yb = (Yb >> Zb) | (Yb << (16 - Zb));
            osm_src = compile_flags(true);
            osm_src |= ((Yb >> 15) & 0x0001);
            if (Zb == 1) {
                osm_src |= (((kc ^ Yb) >> 4) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 2:
        Zb = shift16_LUT[Zb & 0x1f];
        if (Zb) {
            Yb &= 0xffff;
            kc = Yb;
            ac = is_CF();
            Yb = (Yb << Zb) | (ac << (Zb - 1));
            if (Zb > 1) {
                Yb |= kc >> (17 - Zb);
            }
            osm_src = compile_flags(true);
            osm_src |= ((kc >> (16 - Zb)) & 0x0001);
            if (Zb == 1) {
                osm_src |= (((kc ^ Yb) >> 4) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 3:
        Zb = shift16_LUT[Zb & 0x1f];
        if (Zb) {
            Yb &= 0xffff;
            kc = Yb;
            ac = is_CF();
            Yb = (Yb >> Zb) | (ac << (16 - Zb));
            if (Zb > 1) {
                Yb |= kc << (17 - Zb);
            }
            osm_src = compile_flags(true);
            osm_src |= ((kc >> (Zb - 1)) & 0x0001);
            if (Zb == 1) {
                osm_src |= (((kc ^ Yb) >> 4) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 4:
    case 6:
        Zb &= 0x1f;
        if (Zb) {
            osm_src = Yb << (Zb - 1);
            osm_dst = Yb = (((Yb << Zb) << 16) >> 16);
            osm = 16;
        }
        break;
    case 5:
        Zb &= 0x1f;
        if (Zb) {
            Yb &= 0xffff;
            osm_src = Yb >> (Zb - 1);
            osm_dst = Yb = (((Yb >> Zb) << 16) >> 16);
            osm = 19;
        }
        break;
    case 7:
        Zb &= 0x1f;
        if (Zb) {
            Yb = (Yb << 16) >> 16;
            osm_src = Yb >> (Zb - 1);
            osm_dst = Yb = (((Yb >> Zb) << 16) >> 16);
            osm = 19;
        }
        break;
    }
    return Yb;
}
int x86Internal::shift32(int operation, uint32_t Yb, int Zb) {
    uint32_t kc;
    int ac;
    switch (operation & 7) {
    case 0:
        Zb &= 0x1f;
        if (Zb) {
            kc = Yb;
            Yb = (Yb << Zb) | (Yb >> (32 - Zb));
            osm_src = compile_flags(true);
            osm_src |= (Yb & 0x0001);
            if (Zb == 1) {
                osm_src |= (((kc ^ Yb) >> 20) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 1:
        Zb &= 0x1f;
        if (Zb) {
            kc = Yb;
            Yb = (Yb >> Zb) | (Yb << (32 - Zb));
            osm_src = compile_flags(true);
            osm_src |= ((Yb >> 31) & 0x0001);
            if (Zb == 1) {
                osm_src |= (((kc ^ Yb) >> 20) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 2:
        Zb &= 0x1f;
        if (Zb) {
            kc = Yb;
            ac = is_CF();
            Yb = (Yb << Zb) | (ac << (Zb - 1));
            if (Zb > 1) {
                Yb |= kc >> (33 - Zb);
            }
            osm_src = compile_flags(true);
            osm_src |= ((kc >> (32 - Zb)) & 0x0001);
            if (Zb == 1) {
                osm_src |= (((kc ^ Yb) >> 20) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 3:
        Zb &= 0x1f;
        if (Zb) {
            kc = Yb;
            ac = is_CF();
            Yb = (Yb >> Zb) | (ac << (32 - Zb));
            if (Zb > 1) {
                Yb |= kc << (33 - Zb);
            }
            osm_src = compile_flags(true);
            osm_src |= ((kc >> (Zb - 1)) & 0x0001);
            if (Zb == 1) {
                osm_src |= (((kc ^ Yb) >> 20) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 4:
    case 6:
        Zb &= 0x1f;
        if (Zb) {
            osm_src = Yb << (Zb - 1);
            osm_dst = Yb = Yb << Zb;
            osm = 17;
        }
        break;
    case 5:
        Zb &= 0x1f;
        if (Zb) {
            osm_src = Yb >> (Zb - 1);
            osm_dst = Yb = Yb >> Zb;
            osm = 20;
        }
        break;
    case 7:
        Zb &= 0x1f;
        if (Zb) {
            int Ybi = Yb;
            osm_src = Ybi >> (Zb - 1);
            osm_dst = Yb = Ybi >> Zb;
            osm = 20;
        }
        break;
    }
    return Yb;
}
int x86Internal::op_16_SHRD_SHLD(int operation, int Yb, int Zb, int pc) {
    int flg;
    pc &= 0x1f;
    if (pc) {
        if (operation == 0) { // SHLD
            Zb &= 0xffff;
            flg = Zb | (Yb << 16);
            osm_src = flg >> (32 - pc);
            flg = flg << pc;
            if (pc > 16) {
                flg |= Zb << (pc - 16);
            }
            Yb = osm_dst = flg >> 16;
            osm = 19;
        } else { // SHRD
            flg = (Yb & 0xffff) | (Zb << 16);
            osm_src = flg >> (pc - 1);
            flg = flg >> pc;
            if (pc > 16) {
                flg |= Zb << (32 - pc);
            }
            Yb = osm_dst = (((flg) << 16) >> 16);
            osm = 19;
        }
    }
    return Yb;
}
int x86Internal::op_SHLD(int Yb, int Zb, int pc) {
    pc &= 0x1f;
    if (pc) {
        osm_src = Yb << (pc - 1);
        uint32_t Zbu = Zb;
        uint32_t lval = (Yb << pc);
        uint32_t rval = (Zbu >> (32 - pc));
        osm_dst = Yb = lval | rval;
        osm = 17;
    }
    return Yb;
}
int x86Internal::op_SHRD(int Yb, int Zb, int pc) {
    pc &= 0x1f;
    if (pc) {
        osm_src = Yb >> (pc - 1);
        uint32_t Zbu = Zb;
        uint32_t Ybu = Yb;
        uint32_t lval = (Ybu >> pc);
        uint32_t rval = (Zbu << (32 - pc));
        osm_dst = Yb = lval | rval;
        osm = 20;
    }
    return Yb;
}
void x86Internal::op_16_BT(int Yb, int Zb) {
    Zb &= 0xf;
    osm_src = Yb >> Zb;
    osm = 19;
}
void x86Internal::op_BT(int Yb, int Zb) {
    Zb &= 0x1f;
    osm_src = Yb >> Zb;
    osm = 20;
}
int x86Internal::op_16_BTS_BTR_BTC(int operation, int Yb, int Zb) {
    int wc;
    Zb &= 0xf;
    osm_src = Yb >> Zb;
    wc = 1 << Zb;
    switch (operation) {
    case 1: // BTS
        Yb |= wc;
        break;
    case 2: // BTR
        Yb &= ~wc;
        break;
    case 3: // BTC
    default:
        Yb ^= wc;
        break;
    }
    osm = 19;
    return Yb;
}
int x86Internal::op_BTS_BTR_BTC(int operation, int Yb, int Zb) {
    int wc;
    Zb &= 0x1f;
    osm_src = Yb >> Zb;
    wc = 1 << Zb;
    switch (operation) {
    case 1: // BTS
        Yb |= wc;
        break;
    case 2: // BTR
        Yb &= ~wc;
        break;
    case 3: // BTC
    default:
        Yb ^= wc;
        break;
    }
    osm = 20;
    return Yb;
}
int x86Internal::op_16_BSF(int Yb, int Zb) {
    Zb &= 0xffff;
    if (Zb) {
        Yb = 0;
        while ((Zb & 1) == 0) {
            Yb++;
            Zb >>= 1;
        }
        osm_dst = 1;
    } else {
        osm_dst = 0;
    }
    osm = 14;
    return Yb;
}
int x86Internal::op_BSF(int Yb, int Zb) {
    if (Zb) {
        Yb = 0;
        while ((Zb & 1) == 0) {
            Yb++;
            Zb >>= 1;
        }
        osm_dst = 1;
    } else {
        osm_dst = 0;
    }
    osm = 14;
    return Yb;
}
int x86Internal::op_16_BSR(int Yb, int Zb) {
    Zb &= 0xffff;
    if (Zb) {
        Yb = 15;
        while ((Zb & 0x8000) == 0) {
            Yb--;
            Zb <<= 1;
        }
        osm_dst = 1;
    } else {
        osm_dst = 0;
    }
    osm = 14;
    return Yb;
}
int x86Internal::op_BSR(int Yb, int Zb) {
    if (Zb) {
        Yb = 31;
        while (Zb >= 0) {
            Yb--;
            Zb <<= 1;
        }
        osm_dst = 1;
    } else {
        osm_dst = 0;
    }
    osm = 14;
    return Yb;
}
void x86Internal::op_DIV(int opcode) {
    int a, q, r;
    a = regs[0] & 0xffff;
    opcode &= 0xff;
    if ((a >> 8) >= opcode) {
        abort(0);
    }
    q = a / opcode;
    r = (a % opcode);
    set_lower_word(0, (q & 0xff) | (r << 8));
}
void x86Internal::op_IDIV(int opcode) {
    int a, q, r;
    a = (regs[0] << 16) >> 16;
    opcode = (opcode << 24) >> 24;
    if (opcode == 0) {
        abort(0);
    }
    q = a / opcode;
    if (((q << 24) >> 24) != q) {
        abort(0);
    }
    r = (a % opcode);
    set_lower_word(0, (q & 0xff) | (r << 8));
}
void x86Internal::op_16_DIV(int opcode) {
    int a, q, r;
    a = (regs[2] << 16) | (regs[0] & 0xffff);
    opcode &= 0xffff;
    uint32_t au = a;
    if ((au >> 16) >= opcode) {
        abort(0);
    }
    q = au / opcode;
    r = (au % opcode);
    set_lower_word(0, q);
    set_lower_word(2, r);
}
void x86Internal::op_16_IDIV(int opcode) {
    int a, q, r;
    a = (regs[2] << 16) | (regs[0] & 0xffff);
    opcode = (opcode << 16) >> 16;
    if (opcode == 0) {
        abort(0);
    }
    q = a / opcode;
    if (((q << 16) >> 16) != q) {
        abort(0);
    }
    r = (a % opcode);
    set_lower_word(0, q);
    set_lower_word(2, r);
}
int x86Internal::op_DIV32(uint32_t Ic, uint32_t Jc, uint32_t opcode) {
    uint64_t a;
    uint32_t i, Kc;
    if (Ic >= opcode) {
        abort(0);
    }
    if (Ic >= 0 && Ic <= 0x200000) {
        a = Ic * 4294967296 + Jc;
        v = a % opcode;
        return a / opcode;
    } else {
        for (i = 0; i < 32; i++) {
            Kc = Ic >> 31;
            Ic = (Ic << 1) | (Jc >> 31);
            if (Kc || Ic >= opcode) {
                Ic = Ic - opcode;
                Jc = (Jc << 1) | 1;
            } else {
                Jc = Jc << 1;
            }
        }
        v = Ic;
        return Jc;
    }
}
int x86Internal::op_IDIV32(int Ic, int Jc, int opcode) {
    int Mc, Nc, q;
    if (Ic < 0) {
        Mc = 1;
        Ic = ~Ic;
        Jc = -Jc;
        if (Jc == 0) {
            Ic = Ic + 1;
        }
    } else {
        Mc = 0;
    }
    if (opcode < 0) {
        opcode = -opcode;
        Nc = 1;
    } else {
        Nc = 0;
    }
    q = op_DIV32(Ic, Jc, opcode);
    Nc ^= Mc;
    if (Nc) {
        if (q > 0x80000000) {
            abort(0);
        }
        q = -q;
    } else {
        if (q >= 0x80000000) {
            abort(0);
        }
    }
    if (Mc) {
        v = -v;
    }
    return q;
}
int x86Internal::op_MUL(int a, int opcode) {
    int flg;
    a &= 0xff;
    opcode &= 0xff;
    flg = (regs[0] & 0xff) * (opcode & 0xff);
    osm_src = flg >> 8;
    osm_dst = (((flg) << 24) >> 24);
    osm = 21;
    return flg;
}
int x86Internal::op_IMUL(int a, int opcode) {
    int flg;
    a = (((a) << 24) >> 24);
    opcode = (((opcode) << 24) >> 24);
    flg = a * opcode;
    osm_dst = (((flg) << 24) >> 24);
    osm_src = flg != osm_dst;
    osm = 21;
    return flg;
}
int x86Internal::op_16_MUL(int a, int opcode) {
    int flg;
    flg = (a & 0xffff) * (opcode & 0xffff);
    osm_src = flg >> 16;
    osm_dst = (((flg) << 16) >> 16);
    osm = 22;
    return flg;
}
int x86Internal::op_16_IMUL(int a, int opcode) {
    int flg;
    a = (a << 16) >> 16;
    opcode = (opcode << 16) >> 16;
    flg = a * opcode;
    osm_dst = (((flg) << 16) >> 16);
    osm_src = flg != osm_dst;
    osm = 22;
    return flg;
}
int x86Internal::do_multiply32(int _a, int cc_opbyte) {
    uint32_t Jc, Ic, Tc, Uc, m;
    uint64_t a = _a;
    uint32_t au = _a;
    uint32_t opcode = cc_opbyte;
    uint64_t r = a * opcode;
    if (r <= 0xffffffff) {
        v = 0;
        r &= -1;
    } else {
        Jc = a & 0xffff;
        Ic = au >> 16;
        Tc = opcode & 0xffff;
        Uc = opcode >> 16;
        r = Jc * Tc;
        v = Ic * Uc;
        m = Jc * Uc;
        r += ((m & 0xffff) << 16);
        v += (m >> 16);
        if (r >= 4294967296) {
            r -= 4294967296;
            v++;
        }
        m = Ic * Tc;
        r += ((m & 0xffff) << 16);
        v += (m >> 16);
        if (r >= 4294967296) {
            r -= 4294967296;
            v++;
        }
        r &= -1;
        v &= -1;
    }
    return r;
}
int x86Internal::op_MUL32(int a, int opcode) {
    osm_dst = do_multiply32(a, opcode);
    osm_src = v;
    osm = 23;
    return osm_dst;
}
int x86Internal::op_IMUL32(int a, int opcode) {
    int s, r;
    s = 0;
    if (a < 0) {
        a = -a;
        s = 1;
    }
    if (opcode < 0) {
        opcode = -opcode;
        s ^= 1;
    }
    r = do_multiply32(a, opcode);
    if (s) {
        v = ~v;
        r = -r;
        if (r == 0) {
            v = v + 1;
        }
    }
    osm_dst = r;
    osm_src = v - (r >> 31);
    osm = 23;
    return r;
}
void x86Internal::set_current_permission_level(int value) {
    cpl = value;
    if (cpl == 3) {
        tlb_read = tlb_read_user;
        tlb_write = tlb_write_user;
    } else {
        tlb_read = tlb_read_kernel;
        tlb_write = tlb_write_kernel;
    }
}
int x86Internal::do_tlb_lookup(int mem8_loc, int ud) {
    int tlb_lookup;
    if (ud) {
        tlb_lookup = tlb_write[mem8_loc >> 12];
    } else {
        tlb_lookup = tlb_read[mem8_loc >> 12];
    }
    if (tlb_lookup == -1) {
        do_tlb_set_page(mem8_loc, ud, cpl == 3);
        if (ud) {
            tlb_lookup = tlb_write[mem8_loc >> 12];
        } else {
            tlb_lookup = tlb_read[mem8_loc >> 12];
        }
    }
    return tlb_lookup ^ mem8_loc;
}
int x86Internal::instruction_length(int opcode, int eip_linear) {
    int ipr, mem8, operation, stride;
    int n = 1;
    ipr = ipr_default;
    if (ipr & 0x0100) {
        stride = 2;
    } else {
        stride = 4;
    }
    while (true) {
        switch (opcode) {
        case 0x66: // operand-size override prefix
            if (ipr_default & 0x0100) {
                stride = 4;
                ipr &= ~0x0100;
            } else {
                stride = 2;
                ipr |= 0x0100;
            }
        case 0xf0: // LOCK prefix
        case 0xf2: // REPN[EZ] repeat string operation prefix
        case 0xf3: // REP[EZ] repeat string operation prefix
        case 0x26: // ES segment override prefix
        case 0x2e: // CS segment override prefix
        case 0x36: // SS segment override prefix
        case 0x3e: // DS segment override prefix
        case 0x64: // FS segment override prefix
        case 0x65: // GS segment override prefix
            if ((n + 1) > 15) {
                abort(13);
            }
            mem8_loc = eip_linear + (n++);
            opcode = (check_real__v86() || ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                     ? __ld_8bits_mem8_read()
                     : phys_mem8[mem8_loc ^ tlb_hash]);
            break;
        case 0x67: // address-size override prefix
            if (ipr_default & 0x0080) {
                ipr &= ~0x0080;
            } else {
                ipr |= 0x0080;
            }
            if ((n + 1) > 15) {
                abort(13);
            }
            mem8_loc = eip_linear + (n++);
            opcode = (check_real__v86() || ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                     ? __ld_8bits_mem8_read()
                     : phys_mem8[mem8_loc ^ tlb_hash]);
            break;
        case 0x91: // XCHG C
        case 0x92: // XCHG D
        case 0x93: // XCHG B
        case 0x94: // XCHG SP
        case 0x95: // XCHG BP
        case 0x96: // XCHG SI
        case 0x97: // XCHG DI
        case 0x40: // INC A
        case 0x41: // INC C
        case 0x42: // INC D
        case 0x43: // INC B
        case 0x44: // INC SP
        case 0x45: // INC BP
        case 0x46: // INC SI
        case 0x47: // INC DI
        case 0x48: // DEC A
        case 0x49: // DEC C
        case 0x4a: // DEC D
        case 0x4b: // DEC B
        case 0x4c: // DEC SP
        case 0x4d: // DEC BP
        case 0x4e: // DEC SI
        case 0x4f: // DEC DI
        case 0x50: // PUSH A
        case 0x51: // PUSH C
        case 0x52: // PUSH D
        case 0x53: // PUSH B
        case 0x54: // PUSH SP
        case 0x55: // PUSH BP
        case 0x56: // PUSH SI
        case 0x57: // PUSH DI
        case 0x58: // POP A
        case 0x59: // POP C
        case 0x5a: // POP D
        case 0x5b: // POP B
        case 0x5c: // POP SP
        case 0x5d: // POP BP
        case 0x5e: // POP SI
        case 0x5f: // POP DI
        case 0x98: // CBW
        case 0x99: // CWD
        case 0xc9: // LEAVE
        case 0x9c: // PUSHF
        case 0x9d: // POPF
        case 0x06: // PUSH
        case 0x0e: // PUSH
        case 0x16: // PUSH
        case 0x1e: // PUSH
        case 0x07: // POP
        case 0x17: // POP
        case 0x1f: // POP
        case 0xc3: // RET
        case 0xcb: // RET
        case 0x90: // NOP
        case 0xcc: // INT
        case 0xce: // INTO
        case 0xcf: // IRET
        case 0xf5: // CMC
        case 0xf8: // CLC
        case 0xf9: // STC
        case 0xfc: // CLD
        case 0xfd: // STD
        case 0xfa: // CLI
        case 0xfb: // STI
        case 0x9e: // SAHF
        case 0x9f: // LAHF
        case 0xf4: // HLT
        case 0xa4: // MOVSB
        case 0xa5: // MOVSW/D
        case 0xaa: // STOSB
        case 0xab: // STOSW/D
        case 0xa6: // CMPSB
        case 0xa7: // CMPSW/D
        case 0xac: // LOSB
        case 0xad: // LOSW/D
        case 0xae: // SCASB
        case 0xaf: // SCASW/D
        case 0x9b: // FWAIT/WAIT
        case 0xec: // IN AL,DX
        case 0xed: // IN AX,DX
        case 0xee: // OUT DX,AL
        case 0xef: // OUT DX,AX
        case 0xd7: // XLAT
        case 0x27: // DAA
        case 0x2f: // DAS
        case 0x37: // AAA
        case 0x3f: // AAS
        case 0x60: // PUSHA
        case 0x61: // POPA
        case 0x6c: // INSB
        case 0x6d: // INSW/D
        case 0x6e: // OUTSB
        case 0x6f: // OUTSW/D
            goto EXEC_LOOP;
        case 0xb0: // MOV AL
        case 0xb1: // MOV CL
        case 0xb2: // MOV DL
        case 0xb3: // MOV BL
        case 0xb4: // MOV AH
        case 0xb5: // MOV CH
        case 0xb6: // MOV DH
        case 0xb7: // MOV BH
        case 0x04: // ADD
        case 0x0c: // OR
        case 0x14: // ADC
        case 0x1c: // SBB
        case 0x24: // AND
        case 0x2c: // SUB
        case 0x34: // XOR
        case 0x3c: // CMP
        case 0xa8: // TEST
        case 0x6a: // PUSH
        case 0xeb: // JMP
        case 0x70: // JO
        case 0x71: // JNO
        case 0x72: // JB
        case 0x73: // JNB
        case 0x76: // JBE
        case 0x77: // JNBE
        case 0x78: // JS
        case 0x79: // JNS
        case 0x7a: // JP
        case 0x7b: // JNP
        case 0x7c: // JL
        case 0x7d: // JNL
        case 0x7e: // JLE
        case 0x7f: // JNLE
        case 0x74: // JZ
        case 0x75: // JNZ
        case 0xe0: // LOOPNE
        case 0xe1: // LOOPE
        case 0xe2: // LOOP
        case 0xe3: // JCXZ
        case 0xcd: // INT
        case 0xe4: // IN AL,
        case 0xe5: // IN AX,
        case 0xe6: // OUT ,AL
        case 0xe7: // OUT ,AX
        case 0xd4: // AAM
        case 0xd5: // AAD
            n++;
            if (n > 15) {
                abort(13);
            }
            goto EXEC_LOOP;
        case 0xb8: // MOV A
        case 0xb9: // MOV C
        case 0xba: // MOV D
        case 0xbb: // MOV B
        case 0xbc: // MOV SP
        case 0xbd: // MOV BP
        case 0xbe: // MOV SI
        case 0xbf: // MOV DI
        case 0x05: // ADD
        case 0x0d: // OR
        case 0x15: // ADC
        case 0x1d: // SBB
        case 0x25: // AND
        case 0x2d: // SUB
        case 0x35: // XOR
        case 0x3d: // CMP
        case 0xa9: // TEST
        case 0x68: // PUSH
        case 0xe9: // JMP
        case 0xe8: // CALL
            n += stride;
            if (n > 15) {
                abort(13);
            }
            goto EXEC_LOOP;
        case 0x88: // MOV
        case 0x89: // MOV
        case 0x8a: // MOV
        case 0x8b: // MOV
        case 0x86: // XCHG
        case 0x87: // XCHG
        case 0x8e: // MOV
        case 0x8c: // MOV
        case 0xc4: // LES
        case 0xc5: // LDS
        case 0x00: // ADD
        case 0x08: // OR
        case 0x10: // ADC
        case 0x18: // SBB
        case 0x20: // AND
        case 0x28: // SUB
        case 0x30: // XOR
        case 0x38: // CMP
        case 0x01: // ADD
        case 0x09: // OR
        case 0x11: // ADC
        case 0x19: // SBB
        case 0x21: // AND
        case 0x29: // SUB
        case 0x31: // XOR
        case 0x39: // CMP
        case 0x02: // ADD
        case 0x0a: // OR
        case 0x12: // ADC
        case 0x1a: // SBB
        case 0x22: // AND
        case 0x2a: // SUB
        case 0x32: // XOR
        case 0x3a: // CMP
        case 0x03: // ADD
        case 0x0b: // OR
        case 0x13: // ADC
        case 0x1b: // SBB
        case 0x23: // AND
        case 0x2b: // SUB
        case 0x33: // XOR
        case 0x3b: // CMP
        case 0x84: // TEST
        case 0x85: // TEST
        case 0xd0: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
        case 0xd1: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
        case 0xd2: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
        case 0xd3: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
        case 0x8f: // POP
        case 0x8d: // LEA
        case 0xfe: // G4 (INC, DEC, -, -, -, -, -)
        case 0xff: // G5 (INC, DEC, CALL, CALL, JMP, JMP, PUSH, -)
        case 0xd8: // ESC (80387)
        case 0xd9: // ESC (80387)
        case 0xda: // ESC (80387)
        case 0xdb: // ESC (80387)
        case 0xdc: // ESC (80387)
        case 0xdd: // ESC (80387)
        case 0xde: // ESC (80387)
        case 0xdf: // ESC (80387)
        case 0x62: // BOUND
        case 0x63: // ARPL
            if ((n + 1) > 15) {
                abort(13);
            }
            mem8_loc = eip_linear + (n++);
            mem8 = (check_real__v86() || ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                     ? __ld_8bits_mem8_read()
                     : phys_mem8[mem8_loc ^ tlb_hash]);
            if (ipr & 0x0080) {
                switch (mem8 >> 6) {
                case 0:
                    if ((mem8 & 7) == 6) {
                        n += 2;
                    }
                    break;
                case 1:
                    n++;
                    break;
                default:
                    n += 2;
                    break;
                }
            } else {
                switch ((mem8 & 7) | ((mem8 >> 3) & 0x18)) {
                case 0x04:
                    if ((n + 1) > 15) {
                        abort(13);
                    }
                    mem8_loc = eip_linear + (n++);
                    mem8 = (check_real__v86() || ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                             ? __ld_8bits_mem8_read()
                             : phys_mem8[mem8_loc ^ tlb_hash]);
                    if ((mem8 & 7) == 5) {
                        n += 4;
                    }
                    break;
                case 0x0c:
                    n += 2;
                    break;
                case 0x14:
                    n += 5;
                    break;
                case 0x05:
                    n += 4;
                    break;
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x06:
                case 0x07:
                    break;
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0d:
                case 0x0e:
                case 0x0f:
                    n++;
                    break;
                case 0x10:
                case 0x11:
                case 0x12:
                case 0x13:
                case 0x15:
                case 0x16:
                case 0x17:
                    n += 4;
                    break;
                }
            }
            if (n > 15) {
                abort(13);
            }
            goto EXEC_LOOP;
        case 0xa0: // MOV AL,
        case 0xa1: // MOV AX,
        case 0xa2: // MOV ,AL
        case 0xa3: // MOV ,AX
            if (ipr & 0x0100) {
                n += 2;
            } else {
                n += 4;
            }
            if (n > 15) {
                abort(13);
            }
            goto EXEC_LOOP;
        case 0xc6: // MOV
        case 0x80: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
        case 0x82: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
        case 0x83: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
        case 0x6b: // IMUL
        case 0xc0: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
        case 0xc1: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
            if ((n + 1) > 15) {
                abort(13);
            }
            mem8_loc = eip_linear + (n++);
            mem8 = (check_real__v86() || ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                     ? __ld_8bits_mem8_read()
                     : phys_mem8[mem8_loc ^ tlb_hash]);
            if (ipr & 0x0080) {
                switch (mem8 >> 6) {
                case 0:
                    if ((mem8 & 7) == 6) {
                        n += 2;
                    }
                    break;
                case 1:
                    n++;
                    break;
                default:
                    n += 2;
                    break;
                }
            } else {
                switch ((mem8 & 7) | ((mem8 >> 3) & 0x18)) {
                case 0x04:
                    if ((n + 1) > 15) {
                        abort(13);
                    }
                    mem8_loc = eip_linear + (n++);
                    mem8 = (check_real__v86() || ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                             ? __ld_8bits_mem8_read()
                             : phys_mem8[mem8_loc ^ tlb_hash]);
                    if ((mem8 & 7) == 5) {
                        n += 4;
                    }
                    break;
                case 0x0c:
                    n += 2;
                    break;
                case 0x14:
                    n += 5;
                    break;
                case 0x05:
                    n += 4;
                    break;
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x06:
                case 0x07:
                    break;
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0d:
                case 0x0e:
                case 0x0f:
                    n++;
                    break;
                case 0x10:
                case 0x11:
                case 0x12:
                case 0x13:
                case 0x15:
                case 0x16:
                case 0x17:
                    n += 4;
                    break;
                }
            }
            if (n > 15) {
                abort(13);
            }
            n++;
            if (n > 15) {
                abort(13);
            }
            goto EXEC_LOOP;
        case 0xc7: // MOV
        case 0x81: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
        case 0x69: // IMUL
            if ((n + 1) > 15) {
                abort(13);
            }
            mem8_loc = eip_linear + (n++);
            mem8 = (check_real__v86() || ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                        ? __ld_8bits_mem8_read()
                        : phys_mem8[mem8_loc ^ tlb_hash]);
            if (ipr & 0x0080) {
                switch (mem8 >> 6) {
                case 0:
                    if ((mem8 & 7) == 6) {
                        n += 2;
                    }
                    break;
                case 1:
                    n++;
                    break;
                default:
                    n += 2;
                    break;
                }
            } else {
                switch ((mem8 & 7) | ((mem8 >> 3) & 0x18)) {
                case 0x04:
                    if ((n + 1) > 15) {
                        abort(13);
                    }
                    mem8_loc = eip_linear + (n++);
                    mem8 = (check_real__v86() || ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                             ? __ld_8bits_mem8_read()
                             : phys_mem8[mem8_loc ^ tlb_hash]);
                    if ((mem8 & 7) == 5) {
                        n += 4;
                    }
                    break;
                case 0x0c:
                    n += 2;
                    break;
                case 0x14:
                    n += 5;
                    break;
                case 0x05:
                    n += 4;
                    break;
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x06:
                case 0x07:
                    break;
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0d:
                case 0x0e:
                case 0x0f:
                    n++;
                    break;
                case 0x10:
                case 0x11:
                case 0x12:
                case 0x13:
                case 0x15:
                case 0x16:
                case 0x17:
                    n += 4;
                    break;
                }
            }
            if (n > 15) {
                abort(13);
            }
            n += stride;
            if (n > 15) {
                abort(13);
            }
            goto EXEC_LOOP;
        case 0xf6: // G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
            if ((n + 1) > 15) {
                abort(13);
            }
            mem8_loc = eip_linear + (n++);
            mem8 = (check_real__v86() || ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                     ? __ld_8bits_mem8_read()
                     : phys_mem8[mem8_loc ^ tlb_hash]);
            if (ipr & 0x0080) {
                switch (mem8 >> 6) {
                case 0:
                    if ((mem8 & 7) == 6) {
                        n += 2;
                    }
                    break;
                case 1:
                    n++;
                    break;
                default:
                    n += 2;
                    break;
                }
            } else {
                switch ((mem8 & 7) | ((mem8 >> 3) & 0x18)) {
                case 0x04:
                    if ((n + 1) > 15) {
                        abort(13);
                    }
                    mem8_loc = eip_linear + (n++);
                    mem8 = (check_real__v86() || ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                             ? __ld_8bits_mem8_read()
                             : phys_mem8[mem8_loc ^ tlb_hash]);
                    if ((mem8 & 7) == 5) {
                        n += 4;
                    }
                    break;
                case 0x0c:
                    n += 2;
                    break;
                case 0x14:
                    n += 5;
                    break;
                case 0x05:
                    n += 4;
                    break;
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x06:
                case 0x07:
                    break;
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0d:
                case 0x0e:
                case 0x0f:
                    n++;
                    break;
                case 0x10:
                case 0x11:
                case 0x12:
                case 0x13:
                case 0x15:
                case 0x16:
                case 0x17:
                    n += 4;
                    break;
                }
            }
            if (n > 15) {
                abort(13);
            }
            operation = (mem8 >> 3) & 7;
            if (operation == 0) {
                n++;
                if (n > 15) {
                    abort(13);
                }
            }
            goto EXEC_LOOP;
        case 0xf7: // G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
            if ((n + 1) > 15) {
                abort(13);
            }
            mem8_loc = eip_linear + (n++);
            mem8 = (check_real__v86() || ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                     ? __ld_8bits_mem8_read()
                     : phys_mem8[mem8_loc ^ tlb_hash]);
            if (ipr & 0x0080) {
                switch (mem8 >> 6) {
                case 0:
                    if ((mem8 & 7) == 6) {
                        n += 2;
                    }
                    break;
                case 1:
                    n++;
                    break;
                default:
                    n += 2;
                    break;
                }
            } else {
                switch ((mem8 & 7) | ((mem8 >> 3) & 0x18)) {
                case 0x04:
                    if ((n + 1) > 15) {
                        abort(13);
                    }
                    mem8_loc = eip_linear + (n++);
                    mem8 = (check_real__v86() || ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                             ? __ld_8bits_mem8_read()
                             : phys_mem8[mem8_loc ^ tlb_hash]);
                    if ((mem8 & 7) == 5) {
                        n += 4;
                    }
                    break;
                case 0x0c:
                    n += 2;
                    break;
                case 0x14:
                    n += 5;
                    break;
                case 0x05:
                    n += 4;
                    break;
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x06:
                case 0x07:
                    break;
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0d:
                case 0x0e:
                case 0x0f:
                    n++;
                    break;
                case 0x10:
                case 0x11:
                case 0x12:
                case 0x13:
                case 0x15:
                case 0x16:
                case 0x17:
                    n += 4;
                    break;
                }
            }
            if (n > 15) {
                abort(13);
            }
            operation = (mem8 >> 3) & 7;
            if (operation == 0) {
                n += stride;
                if (n > 15) {
                    abort(13);
                }
            }
            goto EXEC_LOOP;
        case 0xea: // JMPF
        case 0x9a: // CALLF
            n += 2 + stride;
            if (n > 15) {
                abort(13);
            }
            goto EXEC_LOOP;
        case 0xc2: // RET
        case 0xca: // RET
            n += 2;
            if (n > 15) {
                abort(13);
            }
            goto EXEC_LOOP;
        case 0xc8: // ENTER
            n += 3;
            if (n > 15) {
                abort(13);
            }
            goto EXEC_LOOP;
        case 0xd6: // -
        case 0xf1: // -
        default:
            abort(6);
        case 0x0f: // 2-byte instruction escape
            if ((n + 1) > 15) {
                abort(13);
            }
            mem8_loc = eip_linear + (n++);
            opcode = (check_real__v86() || ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                          ? __ld_8bits_mem8_read()
                          : phys_mem8[mem8_loc ^ tlb_hash]);
            switch (opcode) {
            case 0x06: // CLTS
            case 0xa2: // -
            case 0x31: // -
            case 0xa0: // PUSH FS
            case 0xa8: // PUSH GS
            case 0xa1: // POP FS
            case 0xa9: // POP GS
            case 0xc8: // -
            case 0xc9: // -
            case 0xca: // -
            case 0xcb: // -
            case 0xcc: // -
            case 0xcd: // -
            case 0xce: // -
            case 0xcf: // -
                goto EXEC_LOOP;
            case 0x80: // JO
            case 0x81: // JNO
            case 0x82: // JB
            case 0x83: // JNB
            case 0x84: // JZ
            case 0x85: // JNZ
            case 0x86: // JBE
            case 0x87: // JNBE
            case 0x88: // JS
            case 0x89: // JNS
            case 0x8a: // JP
            case 0x8b: // JNP
            case 0x8c: // JL
            case 0x8d: // JNL
            case 0x8e: // JLE
            case 0x8f: // JNLE
                n += stride;
                if (n > 15) {
                    abort(13);
                }
                goto EXEC_LOOP;
            case 0x90: // SETO
            case 0x91: // SETNO
            case 0x92: // SETB
            case 0x93: // SETNB
            case 0x94: // SETZ
            case 0x95: // SETNZ
            case 0x96: // SETBE
            case 0x97: // SETNBE
            case 0x98: // SETS
            case 0x99: // SETNS
            case 0x9a: // SETP
            case 0x9b: // SETNP
            case 0x9c: // SETL
            case 0x9d: // SETNL
            case 0x9e: // SETLE
            case 0x9f: // SETNLE
            case 0x40: // -
            case 0x41: // -
            case 0x42: // -
            case 0x43: // -
            case 0x44: // -
            case 0x45: // -
            case 0x46: // -
            case 0x47: // -
            case 0x48: // -
            case 0x49: // -
            case 0x4a: // -
            case 0x4b: // -
            case 0x4c: // -
            case 0x4d: // -
            case 0x4e: // -
            case 0x4f: // -
            case 0xb6: // MOVZX
            case 0xb7: // MOVZX
            case 0xbe: // MOVSX
            case 0xbf: // MOVSX
            case 0x00: // G6 (SLDT, STR, LLDT, LTR, VERR, VERW, -)
            case 0x01: // G7 (SGDT, SIDT, LGDT, LIDT, SMSW, -, LMSW, -)
            case 0x02: // LAR
            case 0x03: // LSL
            case 0x20: // MOV
            case 0x22: // MOV
            case 0x23: // MOV
            case 0xb2: // LSS
            case 0xb4: // LFS
            case 0xb5: // LGS
            case 0xa5: // SHLD
            case 0xad: // SHRD
            case 0xa3: // BT
            case 0xab: // BTS
            case 0xb3: // BTR
            case 0xbb: // BTC
            case 0xbc: // BSF
            case 0xbd: // BSR
            case 0xaf: // IMUL
            case 0xc0: // -
            case 0xc1: // -
            case 0xb0: // -
            case 0xb1: // -
                if ((n + 1) > 15) {
                    abort(13);
                }
                mem8_loc = eip_linear + (n++);
                mem8 = (check_real__v86() || ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                            ? __ld_8bits_mem8_read()
                            : phys_mem8[mem8_loc ^ tlb_hash]);
                if (ipr & 0x0080) {
                    switch (mem8 >> 6) {
                    case 0:
                        if ((mem8 & 7) == 6) {
                            n += 2;
                        }
                        break;
                    case 1:
                        n++;
                        break;
                    default:
                        n += 2;
                        break;
                    }
                } else {
                    switch ((mem8 & 7) | ((mem8 >> 3) & 0x18)) {
                    case 0x04:
                        if ((n + 1) > 15) {
                            abort(13);
                        }
                        mem8_loc = eip_linear + (n++);
                        mem8 = (check_real__v86() || ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                                 ? __ld_8bits_mem8_read()
                                 : phys_mem8[mem8_loc ^ tlb_hash]);
                        if ((mem8 & 7) == 5) {
                            n += 4;
                        }
                        break;
                    case 0x0c:
                        n += 2;
                        break;
                    case 0x14:
                        n += 5;
                        break;
                    case 0x05:
                        n += 4;
                        break;
                    case 0x00:
                    case 0x01:
                    case 0x02:
                    case 0x03:
                    case 0x06:
                    case 0x07:
                        break;
                    case 0x08:
                    case 0x09:
                    case 0x0a:
                    case 0x0b:
                    case 0x0d:
                    case 0x0e:
                    case 0x0f:
                        n++;
                        break;
                    case 0x10:
                    case 0x11:
                    case 0x12:
                    case 0x13:
                    case 0x15:
                    case 0x16:
                    case 0x17:
                        n += 4;
                        break;
                    }
                }
                if (n > 15) {
                    abort(13);
                }
                goto EXEC_LOOP;
            case 0xa4: // SHLD
            case 0xac: // SHRD
            case 0xba: // G8 (-, -, -, -, BT, BTS, BTR, BTC)
                if ((n + 1) > 15) {
                    abort(13);
                }
                mem8_loc = eip_linear + (n++);
                mem8 = (check_real__v86() || ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                            ? __ld_8bits_mem8_read()
                            : phys_mem8[mem8_loc ^ tlb_hash]);
                if (ipr & 0x0080) {
                    switch (mem8 >> 6) {
                    case 0:
                        if ((mem8 & 7) == 6) {
                            n += 2;
                        }
                        break;
                    case 1:
                        n++;
                        break;
                    default:
                        n += 2;
                        break;
                    }
                } else {
                    switch ((mem8 & 7) | ((mem8 >> 3) & 0x18)) {
                    case 0x04:
                        if ((n + 1) > 15) {
                            abort(13);
                        }
                        mem8_loc = eip_linear + (n++);
                        mem8 = (check_real__v86() || ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                                 ? __ld_8bits_mem8_read()
                                 : phys_mem8[mem8_loc ^ tlb_hash]);
                        if ((mem8 & 7) == 5) {
                            n += 4;
                        }
                        break;
                    case 0x0c:
                        n += 2;
                        break;
                    case 0x14:
                        n += 5;
                        break;
                    case 0x05:
                        n += 4;
                        break;
                    case 0x00:
                    case 0x01:
                    case 0x02:
                    case 0x03:
                    case 0x06:
                    case 0x07:
                        break;
                    case 0x08:
                    case 0x09:
                    case 0x0a:
                    case 0x0b:
                    case 0x0d:
                    case 0x0e:
                    case 0x0f:
                        n++;
                        break;
                    case 0x10:
                    case 0x11:
                    case 0x12:
                    case 0x13:
                    case 0x15:
                    case 0x16:
                    case 0x17:
                        n += 4;
                        break;
                    }
                }
                if (n > 15) {
                    abort(13);
                }
                n++;
                if (n > 15) {
                    abort(13);
                }
                goto EXEC_LOOP;
            case 0x04: // -
            case 0x05: // -
            case 0x07: // -
            case 0x08: // -
            case 0x09: // -
            case 0x0a: // -
            case 0x0b: // -
            case 0x0c: // -
            case 0x0d: // -
            case 0x0e: // -
            case 0x0f: // -
            case 0x10: // -
            case 0x11: // -
            case 0x12: // -
            case 0x13: // -
            case 0x14: // -
            case 0x15: // -
            case 0x16: // -
            case 0x17: // -
            case 0x18: // -
            case 0x19: // -
            case 0x1a: // -
            case 0x1b: // -
            case 0x1c: // -
            case 0x1d: // -
            case 0x1e: // -
            case 0x1f: // -
            case 0x21: // MOV
            case 0x24: // MOV
            case 0x25: // -
            case 0x26: // MOV
            case 0x27: // -
            case 0x28: // -
            case 0x29: // -
            case 0x2a: // -
            case 0x2b: // -
            case 0x2c: // -
            case 0x2d: // -
            case 0x2e: // -
            case 0x2f: // -
            case 0x30: // -
            case 0x32: // -
            case 0x33: // -
            case 0x34: // -
            case 0x35: // -
            case 0x36: // -
            case 0x37: // -
            case 0x38: // -
            case 0x39: // -
            case 0x3a: // -
            case 0x3b: // -
            case 0x3c: // -
            case 0x3d: // -
            case 0x3e: // -
            case 0x3f: // -
            case 0x50: // -
            case 0x51: // -
            case 0x52: // -
            case 0x53: // -
            case 0x54: // -
            case 0x55: // -
            case 0x56: // -
            case 0x57: // -
            case 0x58: // -
            case 0x59: // -
            case 0x5a: // -
            case 0x5b: // -
            case 0x5c: // -
            case 0x5d: // -
            case 0x5e: // -
            case 0x5f: // -
            case 0x60: // -
            case 0x61: // -
            case 0x62: // -
            case 0x63: // -
            case 0x64: // -
            case 0x65: // -
            case 0x66: // -
            case 0x67: // -
            case 0x68: // -
            case 0x69: // -
            case 0x6a: // -
            case 0x6b: // -
            case 0x6c: // -
            case 0x6d: // -
            case 0x6e: // -
            case 0x6f: // -
            case 0x70: // -
            case 0x71: // -
            case 0x72: // -
            case 0x73: // -
            case 0x74: // -
            case 0x75: // -
            case 0x76: // -
            case 0x77: // -
            case 0x78: // -
            case 0x79: // -
            case 0x7a: // -
            case 0x7b: // -
            case 0x7c: // -
            case 0x7d: // -
            case 0x7e: // -
            case 0x7f: // -
            case 0xa6: // -
            case 0xa7: // -
            case 0xaa: // -
            case 0xae: // -
            case 0xb8: // -
            case 0xb9: // -
            case 0xc2: // -
            case 0xc3: // -
            case 0xc4: // -
            case 0xc5: // -
            case 0xc6: // -
            case 0xc7: // -
            default:
                abort(6);
            }
            break;
        }
    }
EXEC_LOOP:;
    return n;
}
void x86Internal::set_CR0(int Qd) {
    // if changing flags 31, 16, or 0, must flush tlb
    if ((Qd & ((1 << 31) | (1 << 16) | (1 << 0))) != (cr0 & ((1 << 31) | (1 << 16) | (1 << 0)))) {
        tlb_flush_all();
    }
    cr0 = Qd | (1 << 4); // keep bit 4 set to 1 (80387 present)
}
void x86Internal::set_CR3(int new_pdb) {
    cr3 = new_pdb;
    if (cr0 & (1 << 31)) { // if in paging mode must reset tables
        tlb_flush_all();
    }
}
void x86Internal::set_CR4(int newval) {
    cr4 = newval;
}
bool x86Internal::check_real__v86() {
    return !check_protected();
}
bool x86Internal::check_protected() {
    return cr0 & (1 << 0);
}
int x86Internal::compile_sizemask(int dte_upper_dword) {
    if (dte_upper_dword & (1 << 22)) {
        return -1;
    } else {
        return 0xffff;
    }
}
void x86Internal::load_xdt_descriptor(int *descriptor_table_entry, int selector) {
    SegmentDescriptor xdt;
    int Rb, dte_lower_dword, dte_upper_dword;
    if (selector & 0x4) {
        xdt = ldt;
    } else {
        xdt = gdt;
    }
    Rb = selector & ~7;
    if ((Rb + 7) > xdt.limit) {
        return;
    }
    mem8_loc = xdt.base + Rb;
    dte_lower_dword = ld32_mem8_kernel_read();
    mem8_loc += 4;
    dte_upper_dword = ld32_mem8_kernel_read();
    descriptor_table_entry[0] = dte_lower_dword;
    descriptor_table_entry[1] = dte_upper_dword;
}
int x86Internal::compile_dte_limit(int dte_lower_dword, int dte_upper_dword) {
    int limit = (dte_lower_dword & 0xffff) | (dte_upper_dword & 0x000f0000);
    if (dte_upper_dword & (1 << 23)) {
        limit = (limit << 12) | 0xfff;
    }
    return limit;
}
int x86Internal::compile_dte_base(int dte_lower_dword, int dte_upper_dword) {
    return ((((dte_lower_dword >> 16) & 0xffff) | ((dte_upper_dword & 0xff) << 16) | (dte_upper_dword & 0xff000000))) & -1;
}
void x86Internal::compile_segment_descriptor(SegmentDescriptor *sd, int dte_lower_dword, int dte_upper_dword) {
    sd->base = compile_dte_base(dte_lower_dword, dte_upper_dword);
    sd->limit = compile_dte_limit(dte_lower_dword, dte_upper_dword);
    sd->flags = dte_upper_dword;
}
void x86Internal::update_segment_register(int reg_idx, int selector, uint32_t base, uint32_t limit, int flags) {
    segs[reg_idx] = {.selector = selector, .base = base, .limit = limit, .flags = flags};
    update_SSB();
}
void x86Internal::set_segment_register_real__v86(int reg_idx, int selector) {
    if (eflags & 0x00020000) { // v86 mode
        update_segment_register(reg_idx, selector, (selector << 4), 0xffff, (1 << 15) | (3 << 13) | (1 << 12) | (1 << 9) | (1 << 8));
    } else { // real mode
        segs[reg_idx].selector = selector;
        segs[reg_idx].base = selector << 4;
        segs[reg_idx].limit = 0xffff;
    }
}
void x86Internal::load_tss_descriptor(int *descriptor_table_entry, int dpl) {
    int tr_type, Rb, is_32_bit, dte_lower_dword, dte_upper_dword;
    if (!(tr.flags & (1 << 15))) {
        abort(11, tr.selector & 0xfffc);
    }
    tr_type = (tr.flags >> 8) & 0xf;
    if ((tr_type & 7) != 1) {
        abort(13, tr.selector & 0xfffc);
    }
    is_32_bit = tr_type >> 3;
    Rb = (dpl * 4 + 2) << is_32_bit;
    if (Rb + (4 << is_32_bit) - 1 > tr.limit) {
        abort(10, tr.selector & 0xfffc);
    }
    mem8_loc = (tr.base + Rb) & -1;
    if (is_32_bit == 0) {
        dte_upper_dword = ld16_mem8_kernel_read();
        mem8_loc += 2;
    } else {
        dte_upper_dword = ld32_mem8_kernel_read();
        mem8_loc += 4;
    }
    dte_lower_dword = ld16_mem8_kernel_read();
    descriptor_table_entry[0] = dte_lower_dword;
    descriptor_table_entry[1] = dte_upper_dword;
}
void x86Internal::do_interrupt_protected_mode(int interrupt_id, int ne, int error_code, int oe, int pe) {
    int qe, descriptor_type, selector, re;
    int te, ue, is_32_bit;
    int dte_lower_dword, dte_upper_dword, ve, ke, le, we, xe;
    int ye, SS_mask;
    int e[2];
    te = 0;
    if (!ne && !pe) {
        switch (interrupt_id) {
        case 8:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 17:
            te = 1;
            break;
        }
    }
    if (ne) {
        ye = oe;
    } else {
        ye = eip;
    }
    if (interrupt_id * 8 + 7 > idt.limit) {
        abort(13, interrupt_id * 8 + 2);
    }
    mem8_loc = (idt.base + interrupt_id * 8) & -1;
    dte_lower_dword = ld32_mem8_kernel_read();
    mem8_loc += 4;
    dte_upper_dword = ld32_mem8_kernel_read();
    descriptor_type = (dte_upper_dword >> 8) & 0x1f;
    switch (descriptor_type) {
    case 14: // 32 bit interrupt gate
    case 15: // 32 bit trap gate
        break;
    case 5: // task gate
    case 6: // 16 bit interrupt gate
    case 7: // 16 bit trap gate
        throw "fatal: unsupported gate type";
        break;
    default:
        abort(13, interrupt_id * 8 + 2);
        break;
    }
    dpl = (dte_upper_dword >> 13) & 3;
    if (ne && dpl < cpl) {
        abort(13, interrupt_id * 8 + 2);
    }
    if (!(dte_upper_dword & (1 << 15))) {
        abort(11, interrupt_id * 8 + 2);
    }
    selector = dte_lower_dword >> 16;
    ve = (dte_upper_dword & -65536) | (dte_lower_dword & 0x0000ffff);
    if ((selector & 0xfffc) == 0) {
        abort(13, 0);
    }
    load_xdt_descriptor(e, selector);
    if (e[0] == 0 && e[1] == 0) {
        abort(13, selector & 0xfffc);
    }
    dte_lower_dword = e[0];
    dte_upper_dword = e[1];
    if (!(dte_upper_dword & (1 << 12)) || !(dte_upper_dword & ((1 << 11)))) {
        abort(13, selector & 0xfffc);
    }
    dpl = (dte_upper_dword >> 13) & 3;
    if (dpl > cpl) {
        abort(13, selector & 0xfffc);
    }
    if (!(dte_upper_dword & (1 << 15))) {
        abort(11, selector & 0xfffc);
    }
    if (!(dte_upper_dword & (1 << 10)) && dpl < cpl) {
        load_tss_descriptor(e, dpl);
        ke = e[0];
        le = e[1];
        if ((ke & 0xfffc) == 0) {
            abort(10, ke & 0xfffc);
        }
        if ((ke & 3) != dpl) {
            abort(10, ke & 0xfffc);
        }
        load_xdt_descriptor(e, ke);
        if (e[0] == 0 && e[1] == 0) {
            abort(10, ke & 0xfffc);
        }
        we = e[0];
        xe = e[1];
        re = (xe >> 13) & 3;
        if (re != dpl) {
            abort(10, ke & 0xfffc);
        }
        if (!(xe & (1 << 12)) || (xe & (1 << 11)) || !(xe & (1 << 9))) {
            abort(10, ke & 0xfffc);
        }
        if (!(xe & (1 << 15))) {
            abort(10, ke & 0xfffc);
        }
        ue = 1;
        SS_mask = compile_sizemask(xe);
        qe = compile_dte_base(we, xe);
    } else if ((dte_upper_dword & (1 << 10)) || dpl == cpl) {
        if (eflags & 0x00020000) {
            abort(13, selector & 0xfffc);
        }
        ue = 0;
        SS_mask = compile_sizemask(segs[2].flags);
        qe = segs[2].base;
        le = regs[4];
        dpl = cpl;
    } else {
        abort(13, selector & 0xfffc);
        ue = 0;
        SS_mask = 0;
        qe = 0;
        le = 0;
    }
    is_32_bit = descriptor_type >> 3;
    if (is_32_bit == 1) {
        if (ue) {
            if (eflags & 0x00020000) {
                le = (le - 4) & -1;
                mem8_loc = (qe + (le & SS_mask)) & -1;
                st32_mem8_kernel_write(segs[5].selector);
                le = (le - 4) & -1;
                mem8_loc = (qe + (le & SS_mask)) & -1;
                st32_mem8_kernel_write(segs[4].selector);
                le = (le - 4) & -1;
                mem8_loc = (qe + (le & SS_mask)) & -1;
                st32_mem8_kernel_write(segs[3].selector);
                le = (le - 4) & -1;
                mem8_loc = (qe + (le & SS_mask)) & -1;
                st32_mem8_kernel_write(segs[0].selector);
            }
            le = (le - 4) & -1;
            mem8_loc = (qe + (le & SS_mask)) & -1;
            st32_mem8_kernel_write(segs[2].selector);
            le = (le - 4) & -1;
            mem8_loc = (qe + (le & SS_mask)) & -1;
            st32_mem8_kernel_write(regs[4]);
        }
        le = (le - 4) & -1;
        mem8_loc = (qe + (le & SS_mask)) & -1;
        st32_mem8_kernel_write(get_EFLAGS());
        le = (le - 4) & -1;
        mem8_loc = (qe + (le & SS_mask)) & -1;
        st32_mem8_kernel_write(segs[1].selector);
        le = (le - 4) & -1;
        mem8_loc = (qe + (le & SS_mask)) & -1;
        st32_mem8_kernel_write(ye);
        if (te) {
            le = (le - 4) & -1;
            mem8_loc = (qe + (le & SS_mask)) & -1;
            st32_mem8_kernel_write(error_code);
        }
    } else {
        if (ue) {
            if (eflags & 0x00020000) {
                le = (le - 2) & -1;
                mem8_loc = (qe + (le & SS_mask)) & -1;
                st16_mem8_kernel_write(segs[5].selector);
                le = (le - 2) & -1;
                mem8_loc = (qe + (le & SS_mask)) & -1;
                st16_mem8_kernel_write(segs[4].selector);
                le = (le - 2) & -1;
                mem8_loc = (qe + (le & SS_mask)) & -1;
                st16_mem8_kernel_write(segs[3].selector);
                le = (le - 2) & -1;
                mem8_loc = (qe + (le & SS_mask)) & -1;
                st16_mem8_kernel_write(segs[0].selector);
            }
            le = (le - 2) & -1;
            mem8_loc = (qe + (le & SS_mask)) & -1;
            st16_mem8_kernel_write(segs[2].selector);
            le = (le - 2) & -1;
            mem8_loc = (qe + (le & SS_mask)) & -1;
            st16_mem8_kernel_write(regs[4]);
        }
        le = (le - 2) & -1;
        mem8_loc = (qe + (le & SS_mask)) & -1;
        st16_mem8_kernel_write(get_EFLAGS());
        le = (le - 2) & -1;
        mem8_loc = (qe + (le & SS_mask)) & -1;
        st16_mem8_kernel_write(segs[1].selector);
        le = (le - 2) & -1;
        mem8_loc = (qe + (le & SS_mask)) & -1;
        st16_mem8_kernel_write(ye);
        if (te) {
            le = (le - 2) & -1;
            mem8_loc = (qe + (le & SS_mask)) & -1;
            st16_mem8_kernel_write(error_code);
        }
    }
    if (ue) {
        if (eflags & 0x00020000) {
            update_segment_register(0, 0, 0, 0, 0);
            update_segment_register(3, 0, 0, 0, 0);
            update_segment_register(4, 0, 0, 0, 0);
            update_segment_register(5, 0, 0, 0, 0);
        }
        ke = (ke & ~3) | dpl;
        update_segment_register(2, ke, qe, compile_dte_limit(we, xe), xe);
    }
    regs[4] = (regs[4] & ~SS_mask) | (le & SS_mask);
    selector = (selector & ~3) | dpl;
    update_segment_register(1, selector, compile_dte_base(dte_lower_dword, dte_upper_dword), compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
    set_current_permission_level(dpl);
    eip = ve, far = far_start = 0;
    if ((descriptor_type & 1) == 0) {
        eflags &= ~0x00000200;
    }
    eflags &= ~(0x00000100 | 0x00004000 | 0x00010000 | 0x00020000);
}
void x86Internal::do_interrupt_real__v86_mode(int interrupt_id, int ne, int error_code, int oe, int pe) {
    int selector, ve, le, ye;
    if (interrupt_id * 4 + 3 > idt.limit) {
        abort(13, interrupt_id * 8 + 2);
    }
    mem8_loc = idt.base + (interrupt_id << 2);
    ve = ld16_mem8_kernel_read();
    mem8_loc = mem8_loc + 2;
    selector = ld16_mem8_kernel_read();
    le = regs[4];
    if (ne) {
        ye = oe;
    } else {
        ye = eip;
    }
    le = le - 2;
    mem8_loc = (le & SS_mask) + SS_base;
    st16_mem8_write(get_EFLAGS());
    le = le - 2;
    mem8_loc = (le & SS_mask) + SS_base;
    st16_mem8_write(segs[1].selector);
    le = le - 2;
    mem8_loc = (le & SS_mask) + SS_base;
    st16_mem8_write(ye);
    regs[4] = (regs[4] & ~SS_mask) | (le & SS_mask);
    eip = ve, far = far_start = 0;
    segs[1].selector = selector;
    segs[1].base = (selector << 4);
    eflags &= ~(0x00000100 | 0x00000200 | 0x00010000 | 0x00040000);
}
void x86Internal::do_interrupt(int interrupt_id, int ne, int error_code, int oe, int pe) {
    if (interrupt_id == 0x06) {
        int eip_tmp = eip;
        int eip_linear;
        std::string str =
            "do_interrupt: intno=" + _2_bytes_(interrupt_id) +
            " error_code=" + _4_bytes_(error_code) +
            " EIP=" + _4_bytes_(eip_tmp) + " ESP=" + _4_bytes_(regs[4]) +
            " EAX=" + _4_bytes_(regs[0]) + " EBX=" + _4_bytes_(regs[3]) +
            " ECX=" + _4_bytes_(regs[1]);
        if (interrupt_id == 0x0e) {
            str += " CR2=" + _4_bytes_(cr2);
        }
        printf("%s\n", str.c_str());
        if (interrupt_id == 0x06) {
            int i, n;
            str = "Code:";
            eip_linear = eip_tmp + CS_base;
            n = 4096 - (eip_linear & 0xfff);
            n = std::min(n, 15);
            for (i = 0; i < n; i++) {
                mem8_loc = (eip_linear + i) & -1;
                str += " " + _2_bytes_(ld_8bits_mem8_read());
            }
            printf("%s\n", str.c_str());
        }
    }
    if (check_protected()) {
        do_interrupt_protected_mode(interrupt_id, ne, error_code, oe, pe);
    } else {
        do_interrupt_real__v86_mode(interrupt_id, ne, error_code, oe, pe);
    }
}
void x86Internal::op_LDTR(int selector) {
    int dte_lower_dword, dte_upper_dword, Rb, De;
    selector &= 0xffff;
    if ((selector & 0xfffc) == 0) {
        ldt.base = 0;
        ldt.limit = 0;
    } else {
        if (selector & 0x4) {
            abort(13, selector & 0xfffc);
        }
        Rb = selector & ~7;
        De = 7;
        if ((Rb + De) > gdt.limit) {
            abort(13, selector & 0xfffc);
        }
        mem8_loc = (gdt.base + Rb) & -1;
        dte_lower_dword = ld32_mem8_kernel_read();
        mem8_loc += 4;
        dte_upper_dword = ld32_mem8_kernel_read();
        if ((dte_upper_dword & (1 << 12)) || ((dte_upper_dword >> 8) & 0xf) != 2) {
            abort(13, selector & 0xfffc);
        }
        if (!(dte_upper_dword & (1 << 15))) {
            abort(11, selector & 0xfffc);
        }
        compile_segment_descriptor(&ldt, dte_lower_dword, dte_upper_dword);
    }
    ldt.selector = selector;
}
void x86Internal::op_LTR(int selector) {
    int dte_lower_dword, dte_upper_dword, Rb, descriptor_type, De;
    selector &= 0xffff;
    if ((selector & 0xfffc) == 0) {
        tr.base = 0;
        tr.limit = 0;
        tr.flags = 0;
    } else {
        if (selector & 0x4) {
            abort(13, selector & 0xfffc);
        }
        Rb = selector & ~7;
        De = 7;
        if ((Rb + De) > gdt.limit) {
            abort(13, selector & 0xfffc);
        }
        mem8_loc = (gdt.base + Rb) & -1;
        dte_lower_dword = ld32_mem8_kernel_read();
        mem8_loc += 4;
        dte_upper_dword = ld32_mem8_kernel_read();
        descriptor_type = (dte_upper_dword >> 8) & 0xf;
        if ((dte_upper_dword & (1 << 12)) || (descriptor_type != 1 && descriptor_type != 9)) {
            abort(13, selector & 0xfffc);
        }
        if (!(dte_upper_dword & (1 << 15))) {
            abort(11, selector & 0xfffc);
        }
        compile_segment_descriptor(&tr, dte_lower_dword, dte_upper_dword);
        dte_upper_dword |= (1 << 9);
        st32_mem8_kernel_write(dte_upper_dword);
    }
    tr.selector = selector;
}
void x86Internal::set_segment_register_protected(int reg_idx, int selector) {
    SegmentDescriptor xdt;
    int dte_lower_dword, dte_upper_dword, dpl, rpl, selector_index;
    if ((selector & 0xfffc) == 0) { // null selector
        if (reg_idx == 2) {
            abort(13, 0);
        }
        update_segment_register(reg_idx, selector, 0, 0, 0);
    } else {
        if (selector & 0x4) {
            xdt = ldt;
        } else {
            xdt = gdt;
        }
        selector_index = selector & ~7;
        if ((selector_index + 7) > xdt.limit) {
            abort(13, selector & 0xfffc);
        }
        mem8_loc = (xdt.base + selector_index) & -1;
        dte_lower_dword = ld32_mem8_kernel_read();
        mem8_loc += 4;
        dte_upper_dword = ld32_mem8_kernel_read();
        if (!(dte_upper_dword & (1 << 12))) {
            abort(13, selector & 0xfffc);
        }
        rpl = selector & 3;
        dpl = (dte_upper_dword >> 13) & 3;
        if (reg_idx == 2) {
            if ((dte_upper_dword & (1 << 11)) || !(dte_upper_dword & (1 << 9))) {
                abort(13, selector & 0xfffc);
            }
            if (rpl != cpl || dpl != cpl) {
                abort(13, selector & 0xfffc);
            }
        } else {
            if ((dte_upper_dword & ((1 << 11) | (1 << 9))) == (1 << 11)) {
                abort(13, selector & 0xfffc);
            }
            if (!(dte_upper_dword & (1 << 11)) || !(dte_upper_dword & (1 << 10))) {
                if (dpl < cpl || dpl < rpl) {
                    abort(13, selector & 0xfffc);
                }
            }
        }
        if (!(dte_upper_dword & (1 << 15))) {
            if (reg_idx == 2) {
                abort(12, selector & 0xfffc);
            } else {
                abort(11, selector & 0xfffc);
            }
        }
        if (!(dte_upper_dword & (1 << 8))) {
            dte_upper_dword |= (1 << 8);
            st32_mem8_kernel_write(dte_upper_dword);
        }
        update_segment_register(reg_idx, selector, compile_dte_base(dte_lower_dword, dte_upper_dword), compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
    }
}
void x86Internal::set_segment_register(int reg_idx, int selector) {
    selector &= 0xffff;
    if (check_protected()) {
        set_segment_register_protected(reg_idx, selector);
    } else { // real or v86 mode
        set_segment_register_real__v86(reg_idx, selector);
    }
}
void x86Internal::do_JMPF_virtual_mode(int selector, int Le) {
    eip = Le, far = far_start = 0;
    segs[1].selector = selector;
    segs[1].base = (selector << 4);
    update_SSB();
}
void x86Internal::do_JMPF(int selector, int Le) {
    int dte_lower_dword, dte_upper_dword, dpl, rpl;
    uint32_t limit;
    if ((selector & 0xfffc) == 0) {
        abort(13, 0);
    }
    int e[2];
    load_xdt_descriptor(e, selector);
    if (e[0] == 0 && e[1] == 0) {
        abort(13, selector & 0xfffc);
    }
    dte_lower_dword = e[0];
    dte_upper_dword = e[1];
    if (dte_upper_dword & (1 << 12)) {
        if (!(dte_upper_dword & (1 << 11))) {
            abort(13, selector & 0xfffc);
        }
        dpl = (dte_upper_dword >> 13) & 3;
        if (dte_upper_dword & (1 << 10)) {
            if (dpl > cpl) {
                abort(13, selector & 0xfffc);
            }
        } else {
            rpl = selector & 3;
            if (rpl > cpl) {
                abort(13, selector & 0xfffc);
            }
            if (dpl != cpl) {
                abort(13, selector & 0xfffc);
            }
        }
        if (!(dte_upper_dword & (1 << 15))) {
            abort(11, selector & 0xfffc);
        }
        limit = compile_dte_limit(dte_lower_dword, dte_upper_dword);
        if (Le > limit) {
            abort(13, selector & 0xfffc);
        }
        update_segment_register(1, (selector & 0xfffc) | cpl, compile_dte_base(dte_lower_dword, dte_upper_dword), limit,  dte_upper_dword);
        eip = Le, far = far_start = 0;
    } else {
        throw "fatal: unsupported TSS or task gate in JMP";
    }
}
void x86Internal::op_JMPF(int selector, int Le) {
    if (!check_protected() || (eflags & 0x00020000)) {
        do_JMPF_virtual_mode(selector, Le);
    } else {
        do_JMPF(selector, Le);
    }
}
void x86Internal::Pe(int reg_idx, int cpl) {
    int dpl, dte_upper_dword;
    if ((reg_idx == 4 || reg_idx == 5) && (segs[reg_idx].selector & 0xfffc) == 0) {
        return;
    }
    dte_upper_dword = segs[reg_idx].flags;
    dpl = (dte_upper_dword >> 13) & 3;
    if (!(dte_upper_dword & (1 << 11)) || !(dte_upper_dword & (1 << 10))) {
        if (dpl < cpl) {
            update_segment_register(reg_idx, 0, 0, 0, 0);
        }
    }
}
void x86Internal::op_CALLF_real__v86_mode(bool is_32_bit, int selector, int Le, int oe) {
    int le;
    le = regs[4];
    if (is_32_bit) {
        le = le - 4;
        mem8_loc = (le & SS_mask) + SS_base;
        st32_mem8_write(segs[1].selector);
        le = le - 4;
        mem8_loc = (le & SS_mask) + SS_base;
        st32_mem8_write(oe);
    } else {
        le = le - 2;
        mem8_loc = (le & SS_mask) + SS_base;
        st16_mem8_write(segs[1].selector);
        le = le - 2;
        mem8_loc = (le & SS_mask) + SS_base;
        st16_mem8_write(oe);
    }
    regs[4] = (regs[4] & ~SS_mask) | (le & SS_mask);
    eip = Le, far = far_start = 0;
    segs[1].selector = selector;
    segs[1].base = (selector << 4);
    update_SSB();
}
void x86Internal::op_CALLF_protected_mode(bool is_32_bit, int selector, int Le, int oe) {
    int ue, i;
    int dte_lower_dword, dte_upper_dword, dpl, rpl, ve, Se;
    int ke, we, xe, esp, descriptor_type, re, SS_mask;
    int x = 0, limit, Ue;
    int qe, Ve, We;
    if ((selector & 0xfffc) == 0) {
        abort(13, 0);
    }
    int e[2];
    load_xdt_descriptor(e, selector);
    if (e[0] == 0 && e[1] == 0) {
        abort(13, selector & 0xfffc);
    }
    dte_lower_dword = e[0];
    dte_upper_dword = e[1];
    We = regs[4];
    if (dte_upper_dword & (1 << 12)) {
        if (!(dte_upper_dword & (1 << 11))) {
            abort(13, selector & 0xfffc);
        }
        dpl = (dte_upper_dword >> 13) & 3;
        if (dte_upper_dword & (1 << 10)) {
            if (dpl > cpl) {
                abort(13, selector & 0xfffc);
            }
        } else {
            rpl = selector & 3;
            if (rpl > cpl) {
                abort(13, selector & 0xfffc);
            }
            if (dpl != cpl) {
                abort(13, selector & 0xfffc);
            }
        }
        if (!(dte_upper_dword & (1 << 15))) {
            abort(11, selector & 0xfffc);
        }
        esp = We;
        SS_mask = compile_sizemask(segs[2].flags);
        qe = segs[2].base;
        if (is_32_bit) {
            esp = (esp - 4) & -1;
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            st32_mem8_kernel_write(segs[1].selector);
            esp = (esp - 4) & -1;
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            st32_mem8_kernel_write(oe);
        } else {
            esp = (esp - 2) & -1;
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            st16_mem8_kernel_write(segs[1].selector);
            esp = (esp - 2) & -1;
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            st16_mem8_kernel_write(oe);
        }
        limit = compile_dte_limit(dte_lower_dword, dte_upper_dword);
        if (Le > limit) {
            abort(13, selector & 0xfffc);
        }
        regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
        update_segment_register(1, (selector & 0xfffc) | cpl, compile_dte_base(dte_lower_dword, dte_upper_dword), limit, dte_upper_dword);
        eip = Le, far = far_start = 0;
    } else {
        descriptor_type = (dte_upper_dword >> 8) & 0x1f;
        dpl = (dte_upper_dword >> 13) & 3;
        rpl = selector & 3;
        switch (descriptor_type) {
        case 4:  // call gate
        case 12: // 386 call gate
            break;
        case 1: // 16 bit task state segment
        case 9: // 32 bit task state segment
        case 5: // task gate
            throw "fatal: unsupported TSS or task gate in CALL";
            break;
        default:
            abort(13, selector & 0xfffc);
            break;
        }
        is_32_bit = descriptor_type >> 3;
        if (dpl < cpl || dpl < rpl) {
            abort(13, selector & 0xfffc);
        }
        if (!(dte_upper_dword & (1 << 15))) {
            abort(11, selector & 0xfffc);
        }
        selector = dte_lower_dword >> 16;
        ve = (dte_upper_dword & 0xffff0000) | (dte_lower_dword & 0x0000ffff);
        Se = dte_upper_dword & 0x1f;
        if ((selector & 0xfffc) == 0) {
            abort(13, 0);
        }
        load_xdt_descriptor(e, selector);
        if (e[0] == 0 && e[1] == 0) {
            abort(13, selector & 0xfffc);
        }
        dte_lower_dword = e[0];
        dte_upper_dword = e[1];
        if (!(dte_upper_dword & (1 << 12)) || !(dte_upper_dword & ((1 << 11)))) {
            abort(13, selector & 0xfffc);
        }
        dpl = (dte_upper_dword >> 13) & 3;
        if (dpl > cpl) {
            abort(13, selector & 0xfffc);
        }
        if (!(dte_upper_dword & (1 << 15))) {
            abort(11, selector & 0xfffc);
        }
        if (!(dte_upper_dword & (1 << 10)) && dpl < cpl) {
            load_tss_descriptor(e, dpl);
            ke = e[0];
            esp = e[1];
            if ((ke & 0xfffc) == 0) {
                abort(10, ke & 0xfffc);
            }
            if ((ke & 3) != dpl) {
                abort(10, ke & 0xfffc);
            }
            load_xdt_descriptor(e, ke);
            if (e[0] == 0 && e[1] == 0) {
                abort(10, ke & 0xfffc);
            }
            we = e[0];
            xe = e[1];
            re = (xe >> 13) & 3;
            if (re != dpl) {
                abort(10, ke & 0xfffc);
            }
            if (!(xe & (1 << 12)) || (xe & (1 << 11)) || !(xe & (1 << 9))) {
                abort(10, ke & 0xfffc);
            }
            if (!(xe & (1 << 15))) {
                abort(10, ke & 0xfffc);
            }
            Ue = compile_sizemask(segs[2].flags);
            Ve = segs[2].base;
            SS_mask = compile_sizemask(xe);
            qe = compile_dte_base(we, xe);
            if (is_32_bit) {
                esp = (esp - 4) & -1;
                mem8_loc = (qe + (esp & SS_mask)) & -1;
                st32_mem8_kernel_write(segs[2].selector);
                esp = (esp - 4) & -1;
                mem8_loc = (qe + (esp & SS_mask)) & -1;
                st32_mem8_kernel_write(We);
                for (i = Se - 1; i >= 0; i--) {
                    // x = Xe(Ve + ((We + i * 4) & Ue));
                    esp = (esp - 4) & -1;
                    mem8_loc = (qe + (esp & SS_mask)) & -1;
                    st32_mem8_kernel_write(x);
                }
            } else {
                esp = (esp - 2) & -1;
                mem8_loc = (qe + (esp & SS_mask)) & -1;
                st16_mem8_kernel_write(segs[2].selector);
                esp = (esp - 2) & -1;
                mem8_loc = (qe + (esp & SS_mask)) & -1;
                st16_mem8_kernel_write(We);
                for (i = Se - 1; i >= 0; i--) {
                    // x = Ye(Ve + ((We + i * 2) & Ue));
                    esp = (esp - 2) & -1;
                    mem8_loc = (qe + (esp & SS_mask)) & -1;
                    st16_mem8_kernel_write(x);
                }
            }
            ue = 1;
        } else {
            esp = We;
            SS_mask = compile_sizemask(segs[2].flags);
            qe = segs[2].base;
            ue = 0;
        }
        if (is_32_bit) {
            esp = (esp - 4) & -1;
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            st32_mem8_kernel_write(segs[1].selector);
            esp = (esp - 4) & -1;
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            st32_mem8_kernel_write(oe);
        } else {
            esp = (esp - 2) & -1;
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            st16_mem8_kernel_write(segs[1].selector);
            esp = (esp - 2) & -1;
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            st16_mem8_kernel_write(oe);
        }
        if (ue) {
            ke = (ke & ~3) | dpl;
            update_segment_register(2, ke, qe, compile_dte_limit(we, xe), xe);
        }
        selector = (selector & ~3) | dpl;
        update_segment_register(1, selector, compile_dte_base(dte_lower_dword, dte_upper_dword), compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
        set_current_permission_level(dpl);
        regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
        eip = ve, far = far_start = 0;
    }
}
void x86Internal::op_CALLF(bool is_32_bit, int selector, int Le, int oe) {
    if (!check_protected() || (eflags & 0x00020000)) {
        op_CALLF_real__v86_mode(is_32_bit, selector, Le, oe);
    } else {
        op_CALLF_protected_mode(is_32_bit, selector, Le, oe);
    }
}
void x86Internal::do_return_real__v86_mode(bool is_32_bit, bool is_iret, int imm16) {
    int esp, selector, stack_eip, stack_eflags, SS_mask, qe, ef;
    SS_mask = 0xffff;
    esp = regs[4];
    qe = segs[2].base;
    if (is_32_bit == 1) {
        mem8_loc = (qe + (esp & SS_mask)) & -1;
        stack_eip = ld32_mem8_kernel_read();
        esp = (esp + 4) & -1;
        mem8_loc = (qe + (esp & SS_mask)) & -1;
        selector = ld32_mem8_kernel_read();
        esp = (esp + 4) & -1;
        selector &= 0xffff;
        if (is_iret) {
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            stack_eflags = ld32_mem8_kernel_read();
            esp = (esp + 4) & -1;
        }
    } else {
        mem8_loc = (qe + (esp & SS_mask)) & -1;
        stack_eip = ld16_mem8_kernel_read();
        esp = (esp + 2) & -1;
        mem8_loc = (qe + (esp & SS_mask)) & -1;
        selector = ld16_mem8_kernel_read();
        esp = (esp + 2) & -1;
        if (is_iret) {
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            stack_eflags = ld16_mem8_kernel_read();
            esp = (esp + 2) & -1;
        }
    }
    regs[4] = (regs[4] & ~SS_mask) | ((esp + imm16) & SS_mask);
    segs[1].selector = selector;
    segs[1].base = (selector << 4);
    eip = stack_eip, far = far_start = 0;
    if (is_iret) {
        if (eflags & 0x00020000) {
            ef = 0x00000100 | 0x00000200 | 0x00004000 | 0x00010000 | 0x00040000 | 0x00200000;
        } else {
            ef = 0x00000100 | 0x00000200 | 0x00003000 | 0x00004000 | 0x00010000 | 0x00040000 | 0x00200000;
        }
        if (is_32_bit == 0) {
            ef &= 0xffff;
        }
        set_EFLAGS(stack_eflags, ef);
    }
    update_SSB();
}
void x86Internal::do_return_protected_mode(bool is_32_bit, bool is_iret, int imm16) {
    int selector, stack_eflags, gf;
    int hf, jf, kf, lf;
    int dte_lower_dword, dte_upper_dword, we, xe;
    int _cpl = cpl, dpl, rpl, ef, iopl;
    int qe, esp, stack_eip, wd, SS_mask;
    int e[2];
    SS_mask = compile_sizemask(segs[2].flags);
    esp = regs[4];
    qe = segs[2].base;
    stack_eflags = 0;
    if (is_32_bit == 1) {
        mem8_loc = (qe + (esp & SS_mask)) & -1;
        stack_eip = ld32_mem8_kernel_read();
        esp = (esp + 4) & -1;
        mem8_loc = (qe + (esp & SS_mask)) & -1;
        selector = ld32_mem8_kernel_read(); // CS
        esp = (esp + 4) & -1;
        selector &= 0xffff;
        if (is_iret) {
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            stack_eflags = ld32_mem8_kernel_read();
            esp = (esp + 4) & -1;
            if (stack_eflags & 0x00020000) {
                mem8_loc = (qe + (esp & SS_mask)) & -1;
                wd = ld32_mem8_kernel_read();
                esp = (esp + 4) & -1;
                // pop segment selectors from stack
                mem8_loc = (qe + (esp & SS_mask)) & -1;
                gf = ld32_mem8_kernel_read();
                esp = (esp + 4) & -1;
                mem8_loc = (qe + (esp & SS_mask)) & -1;
                hf = ld32_mem8_kernel_read();
                esp = (esp + 4) & -1;
                mem8_loc = (qe + (esp & SS_mask)) & -1;
                jf = ld32_mem8_kernel_read();
                esp = (esp + 4) & -1;
                mem8_loc = (qe + (esp & SS_mask)) & -1;
                kf = ld32_mem8_kernel_read();
                esp = (esp + 4) & -1;
                mem8_loc = (qe + (esp & SS_mask)) & -1;
                lf = ld32_mem8_kernel_read();
                esp = (esp + 4) & -1;
                // clang-format off
                set_EFLAGS(stack_eflags, 0x00000100 | 0x00000200 |
                                        0x00003000 | 0x00004000 |
                                        0x00020000 | 0x00040000 | 0x00080000 |
                                        0x00100000 | 0x00200000);
                // clang-format on
                set_segment_register_real__v86(1, selector & 0xffff);
                set_current_permission_level(3);
                set_segment_register_real__v86(0, hf & 0xffff);
                set_segment_register_real__v86(2, gf & 0xffff);
                set_segment_register_real__v86(3, jf & 0xffff);
                set_segment_register_real__v86(4, kf & 0xffff);
                set_segment_register_real__v86(5, lf & 0xffff);
                eip = stack_eip & 0xffff, far = far_start = 0;
                regs[4] = (regs[4] & ~SS_mask) | (wd & SS_mask);
                return;
            }
        }
    } else {
        mem8_loc = (qe + (esp & SS_mask)) & -1;
        stack_eip = ld16_mem8_kernel_read();
        esp = (esp + 2) & -1;
        mem8_loc = (qe + (esp & SS_mask)) & -1;
        selector = ld16_mem8_kernel_read();
        esp = (esp + 2) & -1;
        if (is_iret) {
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            stack_eflags = ld16_mem8_kernel_read();
            esp = (esp + 2) & -1;
        }
    }
    if ((selector & 0xfffc) == 0) {
        abort(13, selector & 0xfffc);
    }
    load_xdt_descriptor(e, selector);
    if (e[0] == 0 && e[1] == 0) {
        abort(13, selector & 0xfffc);
    }
    dte_lower_dword = e[0];
    dte_upper_dword = e[1];
    if (!(dte_upper_dword & (1 << 12)) || !(dte_upper_dword & (1 << 11))) {
        abort(13, selector & 0xfffc);
    }
    rpl = selector & 3;
    if (rpl < _cpl) {
        abort(13, selector & 0xfffc);
    }
    dpl = (dte_upper_dword >> 13) & 3;
    if (dte_upper_dword & (1 << 10)) {
        if (dpl > rpl) {
            abort(13, selector & 0xfffc);
        }
    } else {
        if (dpl != rpl) {
            abort(13, selector & 0xfffc);
        }
    }
    if (!(dte_upper_dword & (1 << 15))) {
        abort(11, selector & 0xfffc);
    }
    esp = (esp + imm16) & -1;
    if (rpl == _cpl) {
        update_segment_register(1, selector, compile_dte_base(dte_lower_dword, dte_upper_dword), compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
    } else {
        if (is_32_bit == 1) {
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            wd = ld32_mem8_kernel_read();
            esp = (esp + 4) & -1;
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            gf = ld32_mem8_kernel_read();
            esp = (esp + 4) & -1;
            gf &= 0xffff;
        } else {
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            wd = ld16_mem8_kernel_read();
            esp = (esp + 2) & -1;
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            gf = ld16_mem8_kernel_read();
            esp = (esp + 2) & -1;
        }
        if ((gf & 0xfffc) == 0) {
            abort(13, 0);
        } else {
            if ((gf & 3) != rpl) {
                abort(13, gf & 0xfffc);
            }
            load_xdt_descriptor(e, gf);
            if (e[0] == 0 && e[1] == 0) {
                abort(13, gf & 0xfffc);
            }
            we = e[0];
            xe = e[1];
            if (!(xe & (1 << 12)) || (xe & (1 << 11)) || !(xe & (1 << 9))) {
                abort(13, gf & 0xfffc);
            }
            dpl = (xe >> 13) & 3;
            if (dpl != rpl) {
                abort(13, gf & 0xfffc);
            }
            if (!(xe & (1 << 15))) {
                abort(11, gf & 0xfffc);
            }
            update_segment_register(2, gf, compile_dte_base(we, xe), compile_dte_limit(we, xe), xe);
        }
        update_segment_register(1, selector, compile_dte_base(dte_lower_dword, dte_upper_dword), compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
        set_current_permission_level(rpl);
        esp = wd;
        SS_mask = compile_sizemask(xe);
        Pe(0, rpl);
        Pe(3, rpl);
        Pe(4, rpl);
        Pe(5, rpl);
        esp = (esp + imm16) & -1;
    }
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
    eip = stack_eip, far = far_start = 0;
    if (is_iret) {
        ef = 0x00000100 | 0x00004000 | 0x00010000 | 0x00040000 | 0x00200000;
        if (_cpl == 0) {
            ef |= 0x00003000;
        }
        iopl = (eflags >> 12) & 3;
        if (_cpl <= iopl) {
            ef |= 0x00000200;
        }
        if (is_32_bit == 0) {
            ef &= 0xffff;
        }
        set_EFLAGS(stack_eflags, ef);
    }
}
void x86Internal::op_IRET(bool is_32_bit) {
    int iopl;
    if (!check_protected() || (eflags & 0x00020000)) {
        if (eflags & 0x00020000) {
            iopl = (eflags >> 12) & 3;
            if (iopl != 3) {
                abort(13);
            }
        }
        do_return_real__v86_mode(is_32_bit, 1, 0);
    } else {
        if (eflags & 0x00004000) {
            throw "fatal: unsupported EFLAGS.NT == 1 in IRET";
        } else {
            do_return_protected_mode(is_32_bit, 1, 0);
        }
    }
}
void x86Internal::op_RETF(bool is_32_bit, int imm16) {
    if (!check_protected() || (eflags & 0x00020000)) {
        do_return_real__v86_mode(is_32_bit, 0, imm16);
    } else {
        do_return_protected_mode(is_32_bit, 0, imm16);
    }
}
int x86Internal::of(int selector, bool is_lsl) {
    int dte_lower_dword, dte_upper_dword, rpl, dpl, descriptor_type;
    int e[2];
    if ((selector & 0xfffc) == 0) {
        return -1;
    }
    load_xdt_descriptor(e, selector);
    if (e[0] == 0 && e[1] == 0) {
        return -1;
    }
    dte_lower_dword = e[0];
    dte_upper_dword = e[1];
    rpl = selector & 3;
    dpl = (dte_upper_dword >> 13) & 3;
    if (dte_upper_dword & (1 << 12)) {
        if ((dte_upper_dword & (1 << 11)) && (dte_upper_dword & (1 << 10))) {
        } else {
            if (dpl < cpl || dpl < rpl) {
                return -1;
            }
        }
    } else {
        descriptor_type = (dte_upper_dword >> 8) & 0xf;
        switch (descriptor_type) {
        case 1:
        case 2:
        case 3:
        case 9:
        case 11:
            break;
        case 4:
        case 5:
        case 12:
            if (is_lsl) {
                return -1;
            }
            break;
        default:
            return -1;
        }
        if (dpl < cpl || dpl < rpl) {
            return -1;
        }
    }
    if (is_lsl) {
        return compile_dte_limit(dte_lower_dword, dte_upper_dword);
    } else {
        return dte_upper_dword & 0x00f0ff00;
    }
}
void x86Internal::op_LAR_LSL(bool is_32_bit, bool is_lsl) {
    int x, mem8, reg_idx1, selector;
    if (!check_protected() || (eflags & 0x00020000)) {
        abort(6);
    }
    mem8 = phys_mem8[far++];
    reg_idx1 = (mem8 >> 3) & 7;
    if ((mem8 >> 6) == 3) {
        selector = regs[mem8 & 7] & 0xffff;
    } else {
        mem8_loc = segment_translation(mem8);
        selector = ld_16bits_mem8_read();
    }
    x = of(selector, is_lsl);
    osm_src = compile_flags();
    if (x == -1) {
        osm_src &= ~0x0040;
    } else {
        osm_src |= 0x0040;
        if (is_32_bit) {
            regs[reg_idx1] = x;
        } else {
            set_lower_word(reg_idx1, x);
        }
    }
    osm_dst = ((osm_src >> 6) & 1) ^ 1;
    osm = 24;
}
int x86Internal::is_segment_accessible(int selector, bool writable) {
    int dte_lower_dword, dte_upper_dword, rpl, dpl;
    int e[2];
    if ((selector & 0xfffc) == 0) {
        return 1;
    }
    load_xdt_descriptor(e, selector);
    if (e[0] == 0 && e[1] == 0) {
        return 1;
    }
    dte_lower_dword = e[0];
    dte_upper_dword = e[1];
    if (!(dte_upper_dword & (1 << 12))) {
        return 1;
    }
    rpl = selector & 3;
    dpl = (dte_upper_dword >> 13) & 3;
    if (dte_upper_dword & (1 << 11)) { // code == 1, data == 0
        if (writable) {
            return 1;
        } else {
            if (!(dte_upper_dword & (1 << 9))) {
                return 0;
            }
            if (!(dte_upper_dword & (1 << 10))) {
                if (dpl < cpl || dpl < rpl) {
                    return 1;
                }
            }
        }
    } else {
        if (dpl < cpl || dpl < rpl) {
            return 1;
        }
        if (writable && !(dte_upper_dword & (1 << 9))) {
            return 1;
        }
    }
    return 0;
}
void x86Internal::op_VERR_VERW(int selector, bool writable) {
    int ok;
    ok = is_segment_accessible(selector, writable);
    osm_src = compile_flags();
    if (!ok) {
        osm_src |= 0x0040;
    } else {
        osm_src &= ~0x0040;
    }
    osm_dst = ((osm_src >> 6) & 1) ^ 1;
    osm = 24;
}
void x86Internal::op_ARPL() {
    int mem8, x, y, reg_idx0;
    if (!check_protected() || (eflags & 0x00020000)) {
        abort(6);
    }
    mem8 = phys_mem8[far++];
    if ((mem8 >> 6) == 3) {
        reg_idx0 = mem8 & 7;
        x = regs[reg_idx0] & 0xffff;
    } else {
        mem8_loc = segment_translation(mem8);
        x = ld_16bits_mem8_write();
    }
    y = regs[(mem8 >> 3) & 7];
    osm_src = compile_flags();
    if ((x & 3) < (y & 3)) {
        x = (x & ~3) | (y & 3);
        if ((mem8 >> 6) == 3) {
            set_lower_word(reg_idx0, x);
        } else {
            st16_mem8_write(x);
        }
        osm_src |= 0x0040;
    } else {
        osm_src &= ~0x0040;
    }
    osm_dst = ((osm_src >> 6) & 1) ^ 1;
    osm = 24;
}
void x86Internal::op_CPUID() {
    int eax;
    eax = regs[0];
    switch (eax) {
    case 0: // vendor ID
        regs[0] = 1;
        regs[3] = 0x756e6547 & -1;
        regs[2] = 0x49656e69 & -1;
        regs[1] = 0x6c65746e & -1;
        break;
    case 1: // processor info and feature flags
    default:
        regs[0] = (5 << 8) | (4 << 4) | 3; // family | model | stepping
        regs[3] = 8 << 8;
        regs[1] = 0;
        regs[2] = (1 << 4);
        break;
    }
}
void x86Internal::op_AAM(int base) {
    int wf, xf;
    if (base == 0) {
        abort(0);
    }
    wf = regs[0] & 0xff;
    xf = (wf / base) & -1;
    wf = (wf % base);
    regs[0] = (regs[0] & ~0xffff) | wf | (xf << 8);
    osm_dst = (((wf) << 24) >> 24);
    osm = 12;
}
void x86Internal::op_AAD(int base) {
    int wf, xf;
    wf = regs[0] & 0xff;
    xf = (regs[0] >> 8) & 0xff;
    wf = (xf * base + wf) & 0xff;
    regs[0] = (regs[0] & ~0xffff) | wf;
    osm_dst = (((wf) << 24) >> 24);
    osm = 12;
}
void x86Internal::op_AAA() {
    int Af, wf, xf, Bf, flag_bits;
    flag_bits = compile_flags();
    Bf = flag_bits & 0x0010;
    wf = regs[0] & 0xff;
    xf = (regs[0] >> 8) & 0xff;
    Af = (wf > 0xf9);
    if (((wf & 0x0f) > 9) || Bf) {
        wf = (wf + 6) & 0x0f;
        xf = (xf + 1 + Af) & 0xff;
        flag_bits |= 0x0001 | 0x0010;
    } else {
        flag_bits &= ~(0x0001 | 0x0010);
        wf &= 0x0f;
    }
    regs[0] = (regs[0] & ~0xffff) | wf | (xf << 8);
    osm_src = flag_bits;
    osm_dst = ((osm_src >> 6) & 1) ^ 1;
    osm = 24;
}
void x86Internal::op_AAS() {
    int Af, wf, xf, Bf, flag_bits;
    flag_bits = compile_flags();
    Bf = flag_bits & 0x0010;
    wf = regs[0] & 0xff;
    xf = (regs[0] >> 8) & 0xff;
    Af = (wf < 6);
    if (((wf & 0x0f) > 9) || Bf) {
        wf = (wf - 6) & 0x0f;
        xf = (xf - 1 - Af) & 0xff;
        flag_bits |= 0x0001 | 0x0010;
    } else {
        flag_bits &= ~(0x0001 | 0x0010);
        wf &= 0x0f;
    }
    regs[0] = (regs[0] & ~0xffff) | wf | (xf << 8);
    osm_src = flag_bits;
    osm_dst = ((osm_src >> 6) & 1) ^ 1;
    osm = 24;
}
void x86Internal::op_DAA() {
    int wf, Bf, Ef, flag_bits;
    flag_bits = compile_flags();
    Ef = flag_bits & 0x0001;
    Bf = flag_bits & 0x0010;
    wf = regs[0] & 0xff;
    flag_bits = 0;
    if (((wf & 0x0f) > 9) || Bf) {
        wf = (wf + 6) & 0xff;
        flag_bits |= 0x0010;
    }
    if ((wf > 0x9f) || Ef) {
        wf = (wf + 0x60) & 0xff;
        flag_bits |= 0x0001;
    }
    regs[0] = (regs[0] & ~0xff) | wf;
    flag_bits |= (wf == 0) << 6;
    flag_bits |= parity_LUT[wf] << 2;
    flag_bits |= (wf & 0x80);
    osm_src = flag_bits;
    osm_dst = ((osm_src >> 6) & 1) ^ 1;
    osm = 24;
}
void x86Internal::op_DAS() {
    int wf, Gf, Bf, Ef, flag_bits;
    flag_bits = compile_flags();
    Ef = flag_bits & 0x0001;
    Bf = flag_bits & 0x0010;
    wf = regs[0] & 0xff;
    flag_bits = 0;
    Gf = wf;
    if (((wf & 0x0f) > 9) || Bf) {
        flag_bits |= 0x0010;
        if (wf < 6 || Ef) {
            flag_bits |= 0x0001;
        }
        wf = (wf - 6) & 0xff;
    }
    if ((Gf > 0x99) || Ef) {
        wf = (wf - 0x60) & 0xff;
        flag_bits |= 0x0001;
    }
    regs[0] = (regs[0] & ~0xff) | wf;
    flag_bits |= (wf == 0) << 6;
    flag_bits |= parity_LUT[wf] << 2;
    flag_bits |= (wf & 0x80);
    osm_src = flag_bits;
    osm_dst = ((osm_src >> 6) & 1) ^ 1;
    osm = 24;
}
void x86Internal::checkOp_BOUND() {
    int mem8, x, y, z;
    mem8 = phys_mem8[far++];
    if ((mem8 >> 6) == 3) {
        abort(6);
    }
    mem8_loc = segment_translation(mem8);
    x = ld_32bits_mem8_read();
    mem8_loc = (mem8_loc + 4) & -1;
    y = ld_32bits_mem8_read();
    reg_idx1 = (mem8 >> 3) & 7;
    z = regs[reg_idx1];
    if (z < x || z > y) {
        abort(5);
    }
}
void x86Internal::op_16_BOUND() {
    int mem8, x, y, z;
    mem8 = phys_mem8[far++];
    if ((mem8 >> 6) == 3) {
        abort(6);
    }
    mem8_loc = segment_translation(mem8);
    x = (ld_16bits_mem8_read() << 16) >> 16;
    mem8_loc = (mem8_loc + 2) & -1;
    y = (ld_16bits_mem8_read() << 16) >> 16;
    reg_idx1 = (mem8 >> 3) & 7;
    z = (regs[reg_idx1] << 16) >> 16;
    if (z < x || z > y) {
        abort(5);
    }
}
void x86Internal::op_16_PUSHA() {
    int x, y, reg_idx1;
    y = regs[4] - 16;
    mem8_loc = (y & SS_mask) + SS_base;
    for (reg_idx1 = 7; reg_idx1 >= 0; reg_idx1--) {
        x = regs[reg_idx1];
        st16_mem8_write(x);
        mem8_loc = mem8_loc + 2;
    }
    regs[4] = (regs[4] & ~SS_mask) | (y & SS_mask);
}
void x86Internal::op_PUSHA() {
    int x, y, reg_idx1;
    y = regs[4] - 32;
    mem8_loc = (y & SS_mask) + SS_base;
    for (reg_idx1 = 7; reg_idx1 >= 0; reg_idx1--) {
        x = regs[reg_idx1];
        st32_mem8_write(x);
        mem8_loc = mem8_loc + 4;
    }
    regs[4] = (regs[4] & ~SS_mask) | (y & SS_mask);
}
void x86Internal::op_16_POPA() {
    int reg_idx1;
    mem8_loc = (regs[4] & SS_mask) + SS_base;
    for (reg_idx1 = 7; reg_idx1 >= 0; reg_idx1--) {
        if (reg_idx1 != 4) {
            set_lower_word(reg_idx1, ld_16bits_mem8_read());
        }
        mem8_loc = mem8_loc + 2;
    }
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 16) & SS_mask);
}
void x86Internal::op_POPA() {
    int reg_idx1;
    mem8_loc = (regs[4] & SS_mask) + SS_base;
    for (reg_idx1 = 7; reg_idx1 >= 0; reg_idx1--) {
        if (reg_idx1 != 4) {
            regs[reg_idx1] = ld_32bits_mem8_read();
        }
        mem8_loc = mem8_loc + 4;
    }
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 32) & SS_mask);
}
void x86Internal::op_16_LEAVE() {
    int x, y;
    y = regs[5];
    mem8_loc = (y & SS_mask) + SS_base;
    x = ld_16bits_mem8_read();
    set_lower_word(5, x);
    regs[4] = (regs[4] & ~SS_mask) | ((y + 2) & SS_mask);
}
void x86Internal::op_LEAVE() {
    int x, y;
    y = regs[5];
    mem8_loc = (y & SS_mask) + SS_base;
    x = ld_32bits_mem8_read();
    regs[5] = x;
    regs[4] = (regs[4] & ~SS_mask) | ((y + 4) & SS_mask);
}
void x86Internal::op_16_ENTER() {
    int cf, Qf, le, Rf, x, Sf;
    cf = ld16_mem8_direct();
    Qf = phys_mem8[far++];
    Qf &= 0x1f;
    le = regs[4];
    Rf = regs[5];
    le = le - 2;
    mem8_loc = (le & SS_mask) + SS_base;
    st16_mem8_write(Rf);
    Sf = le;
    if (Qf != 0) {
        while (Qf > 1) {
            Rf = Rf - 2;
            mem8_loc = (Rf & SS_mask) + SS_base;
            x = ld_16bits_mem8_read();
            le = le - 2;
            mem8_loc = (le & SS_mask) + SS_base;
            st16_mem8_write(x);
            Qf--;
        }
        le = le - 2;
        mem8_loc = (le & SS_mask) + SS_base;
        st16_mem8_write(Sf);
    }
    le = le - cf;
    mem8_loc = (le & SS_mask) + SS_base;
    ld_16bits_mem8_write();
    regs[5] = (regs[5] & ~SS_mask) | (Sf & SS_mask);
    regs[4] = (regs[4] & ~SS_mask) | (le & SS_mask);
}
void x86Internal::op_ENTER() {
    int cf, Qf, le, Rf, x, Sf;
    cf = ld16_mem8_direct();
    Qf = phys_mem8[far++];
    Qf &= 0x1f;
    le = regs[4];
    Rf = regs[5];
    le = le - 4;
    mem8_loc = (le & SS_mask) + SS_base;
    st32_mem8_write(Rf);
    Sf = (Rf & ~SS_mask) | (le & SS_mask);
    if (Qf != 0) {
        while (Qf > 1) {
            Rf = Rf - 4;
            mem8_loc = (Rf & SS_mask) + SS_base;
            x = ld_32bits_mem8_read();
            le = le - 4;
            mem8_loc = (le & SS_mask) + SS_base;
            st32_mem8_write(x);
            Qf--;
        }
        le = le - 4;
        mem8_loc = (le & SS_mask) + SS_base;
        st32_mem8_write(Sf);
    }
    le = le - cf;
    mem8_loc = (le & SS_mask) + SS_base;
    ld_32bits_mem8_write();
    regs[5] = (regs[5] & ~SS_mask) | (Sf & SS_mask);
    regs[4] = (regs[4] & ~SS_mask) | (le & SS_mask);
}
void x86Internal::op_16_load_far_pointer32(int Sb) {
    int x, y, mem8;
    mem8 = phys_mem8[far++];
    if ((mem8 >> 3) == 3) {
        ; // abort(6);
    }
    mem8_loc = segment_translation(mem8);
    x = ld_32bits_mem8_read();
    mem8_loc += 4;
    y = ld_16bits_mem8_read();
    set_segment_register(Sb, y);
    regs[(mem8 >> 3) & 7] = x;
}
void x86Internal::op_16_load_far_pointer16(int Sb) {
    int x, y, mem8;
    mem8 = phys_mem8[far++];
    if ((mem8 >> 3) == 3) {
        ; // abort(6);
    }
    mem8_loc = segment_translation(mem8);
    x = ld_16bits_mem8_read();
    mem8_loc += 2;
    y = ld_16bits_mem8_read();
    set_segment_register(Sb, y);
    set_lower_word((mem8 >> 3) & 7, x);
}
void x86Internal::op_16_INS() {
    int Xf, Yf, Zf, ag, iopl, x;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        Xf = 0xffff;
    } else {
        Xf = -1;
    }
    Yf = regs[7];
    Zf = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0) {
            return;
        }
        x = ld16_port(Zf);
        mem8_loc = (Yf & Xf) + segs[0].base;
        st16_mem8_write(x);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf) {
            far = far_start;
        }
    } else {
        x = ld16_port(Zf);
        mem8_loc = (Yf & Xf) + segs[0].base;
        st16_mem8_write(x);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
    }
}
void x86Internal::op_16_OUTS() {
    int Xf, cg, Sb, ag, Zf, iopl, x;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        Xf = 0xffff;
    } else {
        Xf = -1;
    }
    Sb = ipr & 0x000f;
    if (Sb == 0) {
        Sb = 3;
    } else {
        Sb--;
    }
    cg = regs[6];
    Zf = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0) {
            return;
        }
        mem8_loc = (cg & Xf) + segs[Sb].base;
        x = ld_16bits_mem8_read();
        st16_port(Zf, x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf) {
            far = far_start;
        }
    } else {
        mem8_loc = (cg & Xf) + segs[Sb].base;
        x = ld_16bits_mem8_read();
        st16_port(Zf, x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
    }
}
void x86Internal::op_16_MOVS() {
    int Xf, Yf, cg, ag, Sb, eg;
    if (ipr & 0x0080) {
        Xf = 0xffff;
    } else {
        Xf = -1;
    }
    Sb = ipr & 0x000f;
    if (Sb == 0) {
        Sb = 3;
    } else {
        Sb--;
    }
    cg = regs[6];
    Yf = regs[7];
    mem8_loc = (cg & Xf) + segs[Sb].base;
    eg = (Yf & Xf) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0) {
            return;
        }
        x = ld_16bits_mem8_read();
        mem8_loc = eg;
        st16_mem8_write(x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf) {
            far = far_start;
        }
    } else {
        x = ld_16bits_mem8_read();
        mem8_loc = eg;
        st16_mem8_write(x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
    }
}
void x86Internal::op_16_STOS() {
    int Xf, Yf, ag;
    if (ipr & 0x0080) {
        Xf = 0xffff;
    } else {
        Xf = -1;
    }
    Yf = regs[7];
    mem8_loc = (Yf & Xf) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0) {
            return;
        }
        st16_mem8_write(regs[0]);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf) {
            far = far_start;
        }
    } else {
        st16_mem8_write(regs[0]);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
    }
}
void x86Internal::op_16_CMPS() {
    int Xf, Yf, cg, ag, Sb, eg;
    if (ipr & 0x0080) {
        Xf = 0xffff;
    } else {
        Xf = -1;
    }
    Sb = ipr & 0x000f;
    if (Sb == 0) {
        Sb = 3;
    } else {
        Sb--;
    }
    cg = regs[6];
    Yf = regs[7];
    mem8_loc = (cg & Xf) + segs[Sb].base;
    eg = (Yf & Xf) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0) {
            return;
        }
        x = ld_16bits_mem8_read();
        mem8_loc = eg;
        y = ld_16bits_mem8_read();
        do_16bit_math(7, x, y);
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ag & Xf) {
            far = far_start;
        }
    } else {
        x = ld_16bits_mem8_read();
        mem8_loc = eg;
        y = ld_16bits_mem8_read();
        do_16bit_math(7, x, y);
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
    }
}
void x86Internal::op_16_LODS() {
    int Xf, cg, Sb, ag, x;
    if (ipr & 0x0080) {
        Xf = 0xffff;
    } else {
        Xf = -1;
    }
    Sb = ipr & 0x000f;
    if (Sb == 0) {
        Sb = 3;
    } else {
        Sb--;
    }
    cg = regs[6];
    mem8_loc = (cg & Xf) + segs[Sb].base;
    if (ipr & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0) {
            return;
        }
        x = ld_16bits_mem8_read();
        regs[0] = (regs[0] & -65536) | x;
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf) {
            far = far_start;
        }
    } else {
        x = ld_16bits_mem8_read();
        regs[0] = (regs[0] & -65536) | x;
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
    }
}
void x86Internal::op_16_SCAS() {
    int Xf, Yf, ag, x;
    if (ipr & 0x0080) {
        Xf = 0xffff;
    } else {
        Xf = -1;
    }
    Yf = regs[7];
    mem8_loc = (Yf & Xf) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0) {
            return;
        }
        x = ld_16bits_mem8_read();
        do_16bit_math(7, regs[0], x);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ipr & 0x0010) {
            if (!(osm_dst == 0)) {
                return;
            }
        } else {
            if (osm_dst == 0) {
                return;
            }
        }
        if (ag & Xf) {
            far = far_start;
        }
    } else {
        x = ld_16bits_mem8_read();
        do_16bit_math(7, regs[0], x);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
    }
}
int x86Internal::ld8_port(int port_num) {
    return ioport_read(port_num);
}
int x86Internal::ld16_port(int port_num) {
    return ioport_read(port_num);
}
int x86Internal::ld32_port(int port_num) {
    return ioport_read(port_num);
}
void x86Internal::st8_port(int port_num, int x) {
    ioport_write(port_num, x);
}
void x86Internal::st16_port(int port_num, int x) {
    ioport_write(port_num, x);
}
void x86Internal::st32_port(int port_num, int x) {
    ioport_write(port_num, x);
}
void x86Internal::abort(int interrupt_id, int error_code) {
    cycles_processed += (cycles_requested - cycles_remaining);
    interrupt = {interrupt_id, error_code};
    throw interrupt;
}
