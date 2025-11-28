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
    set_current_privilege_level(0); // PM (1986), 10.3
}
x86Internal::~x86Internal() {
    free(phys_mem8);
    delete[] tlb_read_kernel;
    delete[] tlb_write_kernel;
    delete[] tlb_read_user;
    delete[] tlb_write_user;
}
void x86Internal::abort(int interrupt_id, int error_code) {
    cycles_processed += (cycles_requested - cycles_remaining);
    interrupt = {interrupt_id, error_code};
    throw interrupt;
}
void x86Internal::do_tlb_set_page(int linear_address, int writable, bool user) {
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
                } else if ((user || (cr0 & (1 << 16))) && writable && !(pxe & 0x00000002)) {
                    error_code = 0x01;
                } else {
                    if (!(pde & 0x00000020)) { // page not accessed
                        pde |= 0x00000020;
                        st32_phys(pde_address, pde);
                    }
                    clean = (writable && !(pte & 0x00000040)); // WR request and page not dirty
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
        error_code |= writable << 1;
        if (user) {
            error_code |= 0x04;
        }
        cr2 = linear_address;
        abort(14, error_code);
    }
}
int x86Internal::do_tlb_lookup(int mem8_loc, int writable) {
    int tlb_lookup;
    if (writable) {
        tlb_lookup = tlb_write[mem8_loc >> 12];
    } else {
        tlb_lookup = tlb_read[mem8_loc >> 12];
    }
    if (tlb_lookup == -1) {
        do_tlb_set_page(mem8_loc, writable, cpl == 3);
        if (writable) {
            tlb_lookup = tlb_write[mem8_loc >> 12];
        } else {
            tlb_lookup = tlb_read[mem8_loc >> 12];
        }
    }
    return tlb_lookup ^ mem8_loc;
}
void x86Internal::fetch_opcode() {
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
                             ? __ld8_mem8_read()
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
                     ? __ld8_mem8_read()
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
                     ? __ld8_mem8_read()
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
                     ? __ld8_mem8_read()
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
                             ? __ld8_mem8_read()
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
                     ? __ld8_mem8_read()
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
                             ? __ld8_mem8_read()
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
                        ? __ld8_mem8_read()
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
                             ? __ld8_mem8_read()
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
                     ? __ld8_mem8_read()
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
                             ? __ld8_mem8_read()
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
                     ? __ld8_mem8_read()
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
                             ? __ld8_mem8_read()
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
                          ? __ld8_mem8_read()
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
            case 0xc0: // XADD (80486)
            case 0xc1: // XADD (80486)
            case 0xb0: // CMPXCHG (80486)
            case 0xb1: // CMPXCHG (80486)
                if ((n + 1) > 15) {
                    abort(13);
                }
                mem8_loc = eip_linear + (n++);
                mem8 = (check_real__v86() || ((tlb_hash = tlb_read[mem8_loc >> 12]) == -1)
                            ? __ld8_mem8_read()
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
                                 ? __ld8_mem8_read()
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
                            ? __ld8_mem8_read()
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
                                 ? __ld8_mem8_read()
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
void x86Internal::set_CR0(int x) {
    // if changing flags 31, 16, or 0, must flush tlb
    if ((x & ((1 << 31) | (1 << 16) | (1 << 0))) != (cr0 & ((1 << 31) | (1 << 16) | (1 << 0)))) {
        tlb_flush_all();
    }
    cr0 = x | (1 << 4); // keep bit 4 set to 1 (80387 present)
}
void x86Internal::set_CR3(int x) {
    cr3 = x;
    if (cr0 & (1 << 31)) { // if in paging mode must reset tables
        tlb_flush_all();
    }
}
void x86Internal::set_CR4(int x) {
    cr4 = x;
}
bool x86Internal::check_real__v86() {
    return !check_protected();
}
bool x86Internal::check_protected() {
    return cr0 & (1 << 0);
}
void x86Internal::set_current_privilege_level(int x) {
    cpl = x;
    if (cpl == 3) {
        tlb_read = tlb_read_user;
        tlb_write = tlb_write_user;
    } else {
        tlb_read = tlb_read_kernel;
        tlb_write = tlb_write_kernel;
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
void x86Internal::set_lower_byte(int reg, int x) {
    if (reg & 4) { // ESP, EBP, ESI, EDI: set AH, CH, DH, BH
        regs[reg & 3] = (regs[reg & 3] & -65281) | ((x & 0xff) << 8);
    } else { // set AL, CL, DL, BL
        regs[reg & 3] = (regs[reg & 3] & -256) | (x & 0xff);
    }
}
void x86Internal::set_lower_word(int reg, int x) {
    regs[reg] = (regs[reg] & -65536) | (x & 0xffff);
}
int x86Internal::segment_translation(int modRM) {
    int mem8_loc, sib, sib_base, sib_index, sreg;
    if (x86_64_long_mode && (ipr & (0x000f | 0x0080)) == 0) {
        switch ((modRM & 7) | ((modRM >> 3) & 0x18)) {
        case 0x04:
            sib = phys_mem8[far++];
            sib_base = sib & 7;
            if (sib_base == 5) {
                mem8_loc = phys_mem8[far] |
                           (phys_mem8[far + 1] << 8) |
                           (phys_mem8[far + 2] << 16) |
                           (phys_mem8[far + 3] << 24);
                far += 4;
            } else {
                mem8_loc = regs[sib_base];
            }
            sib_index = (sib >> 3) & 7;
            if (sib_index != 4) {
                mem8_loc = mem8_loc + (regs[sib_index] << (sib >> 6));
            }
            break;
        case 0x0c:
            sib = phys_mem8[far++];
            mem8_loc = ((phys_mem8[far++] << 24) >> 24);
            sib_base = sib & 7;
            mem8_loc = mem8_loc + regs[sib_base];
            sib_index = (sib >> 3) & 7;
            if (sib_index != 4) {
                mem8_loc = mem8_loc + (regs[sib_index] << (sib >> 6));
            }
            break;
        case 0x14:
            sib = phys_mem8[far++];
            mem8_loc = phys_mem8[far] |
                       (phys_mem8[far + 1] << 8) |
                       (phys_mem8[far + 2] << 16) |
                       (phys_mem8[far + 3] << 24);
            far += 4;
            sib_base = sib & 7;
            mem8_loc = mem8_loc + regs[sib_base];
            sib_index = (sib >> 3) & 7;
            if (sib_index != 4) {
                mem8_loc = mem8_loc + (regs[sib_index] << (sib >> 6));
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
            sib_base = modRM & 7;
            mem8_loc = regs[sib_base];
            break;
        case 0x08:
        case 0x09:
        case 0x0a:
        case 0x0b:
        case 0x0d:
        case 0x0e:
        case 0x0f:
            mem8_loc = ((phys_mem8[far++] << 24) >> 24);
            sib_base = modRM & 7;
            mem8_loc = mem8_loc + regs[sib_base];
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
            sib_base = modRM & 7;
            mem8_loc = mem8_loc + regs[sib_base];
            break;
        }
        return mem8_loc;
    } else if (ipr & 0x0080) {
        int _sreg; // if no data segement override prefix
        if ((modRM & 0xc7) == 0x06) {
            mem8_loc = ld16_mem8_direct();
            _sreg = 3;
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
                _sreg = 3;
                break;
            case 1:
                mem8_loc = (mem8_loc + regs[3] + regs[7]) & 0xffff;
                _sreg = 3;
                break;
            case 2:
                mem8_loc = (mem8_loc + regs[5] + regs[6]) & 0xffff;
                _sreg = 2;
                break;
            case 3:
                mem8_loc = (mem8_loc + regs[5] + regs[7]) & 0xffff;
                _sreg = 2;
                break;
            case 4:
                mem8_loc = (mem8_loc + regs[6]) & 0xffff;
                _sreg = 3;
                break;
            case 5:
                mem8_loc = (mem8_loc + regs[7]) & 0xffff;
                _sreg = 3;
                break;
            case 6:
                mem8_loc = (mem8_loc + regs[5]) & 0xffff;
                _sreg = 2;
                break;
            case 7:
            default:
                mem8_loc = (mem8_loc + regs[3]) & 0xffff;
                _sreg = 3;
                break;
            }
        }
        sreg = ipr & 0x000f;
        if (sreg == 0) {
            sreg = _sreg;
        } else {
            sreg--;
        }
        mem8_loc = mem8_loc + segs[sreg].base;
        return mem8_loc;
    } else {
        switch ((modRM & 7) | ((modRM >> 3) & 0x18)) {
        case 0x04:
            sib = phys_mem8[far++];
            sib_base = sib & 7;
            if (sib_base == 5) {
                mem8_loc = phys_mem8[far] |
                           (phys_mem8[far + 1] << 8) |
                           (phys_mem8[far + 2] << 16) |
                           (phys_mem8[far + 3] << 24);
                far += 4;
                sib_base = 0;
            } else {
                mem8_loc = regs[sib_base];
            }
            sib_index = (sib >> 3) & 7;
            if (sib_index != 4) {
                mem8_loc = mem8_loc + (regs[sib_index] << (sib >> 6));
            }
            break;
        case 0x0c:
            sib = phys_mem8[far++];
            mem8_loc = ((phys_mem8[far++] << 24) >> 24);
            sib_base = sib & 7;
            mem8_loc = mem8_loc + regs[sib_base];
            sib_index = (sib >> 3) & 7;
            if (sib_index != 4) {
                mem8_loc = mem8_loc + (regs[sib_index] << (sib >> 6));
            }
            break;
        case 0x14:
            sib = phys_mem8[far++];
            mem8_loc = phys_mem8[far] |
                       (phys_mem8[far + 1] << 8) |
                       (phys_mem8[far + 2] << 16) |
                       (phys_mem8[far + 3] << 24);
            far += 4;
            sib_base = sib & 7;
            mem8_loc = mem8_loc + regs[sib_base];
            sib_index = (sib >> 3) & 7;
            if (sib_index != 4) {
                mem8_loc = mem8_loc + (regs[sib_index] << (sib >> 6));
            }
            break;
        case 0x05:
            mem8_loc = phys_mem8[far] |
                       (phys_mem8[far + 1] << 8) |
                       (phys_mem8[far + 2] << 16) |
                       (phys_mem8[far + 3] << 24);
            far += 4;
            sib_base = 0;
            break;
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x06:
        case 0x07:
            sib_base = modRM & 7;
            mem8_loc = regs[sib_base];
            break;
        case 0x08:
        case 0x09:
        case 0x0a:
        case 0x0b:
        case 0x0d:
        case 0x0e:
        case 0x0f: // 2-byte instruction escape
            mem8_loc = ((phys_mem8[far++] << 24) >> 24);
            sib_base = modRM & 7;
            mem8_loc = mem8_loc + regs[sib_base];
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
            sib_base = modRM & 7;
            mem8_loc = mem8_loc + regs[sib_base];
            break;
        }
        sreg = ipr & 0x000f;
        if (sreg == 0) {
            if (sib_base == 4 || sib_base == 5) {
                sreg = 2;
            } else {
                sreg = 3;
            }
        } else {
            sreg--;
        }
        mem8_loc = mem8_loc + segs[sreg].base;
        return mem8_loc;
    }
    return 0;
}
int x86Internal::convert_offset_to_linear(bool writable) {
    uint64_t mem8_loc;
    int sreg, Ls, Tc, Lc;
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
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    // type checking
    if (sreg == 1) { // CS
        Tc = writable || !(segs[sreg].flags & (1 << 9));
    } else { // data segment
        Tc = writable && !(segs[sreg].flags & (1 << 9));
    }
    if (Tc) {
        abort(13, 0);
    }
    mem8_loc = segs[sreg].base + mem8_loc;
    // limit checking
    if (segs[sreg].flags & (1 << 10)) { // expand-down segment
        Lc = mem8_loc < (uint64_t)segs[sreg].base + segs[sreg].limit + 1;
    } else {
        Lc = mem8_loc > (uint64_t)segs[sreg].base + segs[sreg].limit - Ls;
    }
    if (Lc) {
        if (sreg == 2) {
            abort(12, 0); // #SS(0)
        } else {
            abort(13, 0); // #GP(0)
        }
    }
    return mem8_loc;
}
void x86Internal::update_segment_register(int sreg, int selector, uint32_t base, uint32_t limit, int flags) {
    segs[sreg] = {.selector = selector, .base = base, .limit = limit, .flags = flags};
    update_SSB();
}
void x86Internal::set_segment_register(int sreg, int selector) {
    selector &= 0xffff;
    if (check_protected()) {
        set_segment_register_protected(sreg, selector);
    } else { // real or v86 mode
        set_segment_register_real__v86(sreg, selector);
    }
}
void x86Internal::set_segment_register_real__v86(int sreg, int selector) {
    if (eflags & 0x00020000) { // v86 mode
        update_segment_register(sreg, selector, (selector << 4), 0xffff, (1 << 15) | (3 << 13) | (1 << 12) | (1 << 9) | (1 << 8));
    } else { // real mode
        segs[sreg].selector = selector;
        segs[sreg].base = selector << 4;
        segs[sreg].limit = 0xffff;
    }
}
void x86Internal::set_segment_register_protected(int sreg, int selector) {
    SegmentDescriptor xdt;
    int dte_lower_dword, dte_upper_dword, dpl, rpl, selector_index;
    if ((selector & 0xfffc) == 0) { // null selector
        if (sreg == 2) {
            abort(13, 0);
        }
        update_segment_register(sreg, selector, 0, 0, 0);
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
        if (sreg == 2) {
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
            if (sreg == 2) {
                abort(12, selector & 0xfffc);
            } else {
                abort(11, selector & 0xfffc);
            }
        }
        if (!(dte_upper_dword & (1 << 8))) {
            dte_upper_dword |= (1 << 8);
            st32_mem8_kernel_write(dte_upper_dword);
        }
        update_segment_register(sreg, selector, compile_dte_base(dte_lower_dword, dte_upper_dword), compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
    }
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
void x86Internal::load_xdt_descriptor(int *descriptor_table_entry, int selector) {
    SegmentDescriptor xdt;
    int index, dte_lower_dword, dte_upper_dword;
    if (selector & 0x4) {
        xdt = ldt;
    } else {
        xdt = gdt;
    }
    index = selector & ~7;
    if ((index + 7) > xdt.limit) {
        return;
    }
    mem8_loc = xdt.base + index;
    dte_lower_dword = ld32_mem8_kernel_read();
    mem8_loc += 4;
    dte_upper_dword = ld32_mem8_kernel_read();
    descriptor_table_entry[0] = dte_lower_dword;
    descriptor_table_entry[1] = dte_upper_dword;
}
void x86Internal::load_tss_interlevel(int *descriptor_table_entry, int privilege_level) {
    int type, offset, is_386, dte_lower_dword, dte_upper_dword;
    if (!(tr.flags & (1 << 15))) { // present (P bit)
        abort(11, tr.selector & 0xfffc);
    }
    type = (tr.flags >> 8) & 0xf;
    if ((type & 7) != 1) { // 0000_10B1 (B bit)
        abort(13, tr.selector & 0xfffc);
    }
    is_386 = type >> 3; // type 1/ 9 == 286/ 386 TSS (Inroduction to the 80386, p. 50)
    offset = (privilege_level * 4 + 2) << is_386; // offset of privileged SP in TSS
    if (offset + (4 << is_386) - 1 > tr.limit) {
        abort(10, tr.selector & 0xfffc);
    }
    mem8_loc = (tr.base + offset) & -1;
    if (is_386 == 0) {
        dte_upper_dword = ld16_mem8_kernel_read(); // privileged SP
        mem8_loc += 2;
    } else {
        dte_upper_dword = ld32_mem8_kernel_read(); // privileged ESP
        mem8_loc += 4;
    }
    dte_lower_dword = ld16_mem8_kernel_read(); // privileged SS
    descriptor_table_entry[0] = dte_lower_dword;
    descriptor_table_entry[1] = dte_upper_dword;
}
int x86Internal::compile_dte_base(int dte_lower_dword, int dte_upper_dword) {
    return ((((dte_lower_dword >> 16) & 0xffff) | ((dte_upper_dword & 0xff) << 16) | (dte_upper_dword & 0xff000000))) & -1;
}
int x86Internal::compile_dte_limit(int dte_lower_dword, int dte_upper_dword) {
    int limit = (dte_lower_dword & 0xffff) | (dte_upper_dword & 0x000f0000);
    if (dte_upper_dword & (1 << 23)) {
        limit = (limit << 12) | 0xfff;
    }
    return limit;
}
void x86Internal::compile_segment_descriptor(SegmentDescriptor *sd, int dte_lower_dword, int dte_upper_dword) {
    sd->base = compile_dte_base(dte_lower_dword, dte_upper_dword);
    sd->limit = compile_dte_limit(dte_lower_dword, dte_upper_dword);
    sd->flags = dte_upper_dword;
}
int x86Internal::compile_sizemask(int dte_upper_dword) {
    if (dte_upper_dword & (1 << 22)) {
        return -1;
    } else {
        return 0xffff;
    }
}
int x86Internal::op_INC8(int x) {
    if (osm < 25) {
        ocm_preserved = osm;
        ocm_dst_preserved = osm_dst;
    }
    osm_dst = (((x + 1) << 24) >> 24);
    osm = 25;
    return osm_dst;
}
int x86Internal::op_INC16(int x) {
    if (osm < 25) {
        ocm_preserved = osm;
        ocm_dst_preserved = osm_dst;
    }
    osm_dst = (((x + 1) << 16) >> 16);
    osm = 26;
    return osm_dst;
}
int x86Internal::op_DEC8(int x) {
    if (osm < 25) {
        ocm_preserved = osm;
        ocm_dst_preserved = osm_dst;
    }
    osm_dst = (((x - 1) << 24) >> 24);
    osm = 28;
    return osm_dst;
}
int x86Internal::op_DEC16(int x) {
    if (osm < 25) {
        ocm_preserved = osm;
        ocm_dst_preserved = osm_dst;
    }
    osm_dst = (((x - 1) << 16) >> 16);
    osm = 29;
    return osm_dst;
}
int x86Internal::op_SHRD_SHLD16(int operation, int dst, int src, int count) {
    int x;
    count &= 0x1f;
    if (count) {
        if (operation == 0) { // SHLD
            src &= 0xffff;
            x = src | (dst << 16);
            osm_src = x >> (32 - count);
            x = x << count;
            if (count > 16) {
                x |= src << (count - 16);
            }
            dst = osm_dst = x >> 16;
            osm = 19;
        } else { // SHRD
            x = (dst & 0xffff) | (src << 16);
            osm_src = x >> (count - 1);
            x = x >> count;
            if (count > 16) {
                x |= src << (32 - count);
            }
            dst = osm_dst = ((x << 16) >> 16);
            osm = 19;
        }
    }
    return dst;
}
int x86Internal::op_SHRD(int dst, int src, int count) {
    count &= 0x1f;
    if (count) {
        osm_src = dst >> (count - 1);
        uint32_t Zbu = src;
        uint32_t Ybu = dst;
        uint32_t lval = (Ybu >> count);
        uint32_t rval = (Zbu << (32 - count));
        osm_dst = dst = lval | rval;
        osm = 20;
    }
    return dst;
}
int x86Internal::op_SHLD(int dst, int src, int count) {
    count &= 0x1f;
    if (count) {
        osm_src = dst << (count - 1);
        uint32_t Zbu = src;
        uint32_t lval = (dst << count);
        uint32_t rval = (Zbu >> (32 - count));
        osm_dst = dst = lval | rval;
        osm = 17;
    }
    return dst;
}
void x86Internal::op_BT16(int bit_base, int bit_offset) {
    bit_offset &= 0xf;
    osm_src = bit_base >> bit_offset;
    osm = 19;
}
void x86Internal::op_BT(int bit_base, int bit_offset) {
    bit_offset &= 0x1f;
    osm_src = bit_base >> bit_offset;
    osm = 20;
}
int x86Internal::op_BTS_BTR_BTC16(int operation, int bit_base, int bit_offset) {
    int wc;
    bit_offset &= 0xf;
    osm_src = bit_base >> bit_offset;
    wc = 1 << bit_offset;
    switch (operation) {
    case 1: // BTS
        bit_base |= wc;
        break;
    case 2: // BTR
        bit_base &= ~wc;
        break;
    case 3: // BTC
    default:
        bit_base ^= wc;
        break;
    }
    osm = 19;
    return bit_base;
}
int x86Internal::op_BTS_BTR_BTC(int operation, int bit_base, int bit_offset) {
    int wc;
    bit_offset &= 0x1f;
    osm_src = bit_base >> bit_offset;
    wc = 1 << bit_offset;
    switch (operation) {
    case 1: // BTS
        bit_base |= wc;
        break;
    case 2: // BTR
        bit_base &= ~wc;
        break;
    case 3: // BTC
    default:
        bit_base ^= wc;
        break;
    }
    osm = 20;
    return bit_base;
}
int x86Internal::op_BSF16(int dst, int src) {
    src &= 0xffff;
    if (src) {
        dst = 0;
        while ((src & 1) == 0) {
            dst++;
            src >>= 1;
        }
        osm_dst = 1;
    } else {
        osm_dst = 0;
    }
    osm = 14;
    return dst;
}
int x86Internal::op_BSF(int dst, int src) {
    if (src) {
        dst = 0;
        while ((src & 1) == 0) {
            dst++;
            src >>= 1;
        }
        osm_dst = 1;
    } else {
        osm_dst = 0;
    }
    osm = 14;
    return dst;
}
int x86Internal::op_BSR16(int dst, int src) {
    src &= 0xffff;
    if (src) {
        dst = 15;
        while ((src & 0x8000) == 0) {
            dst--;
            src <<= 1;
        }
        osm_dst = 1;
    } else {
        osm_dst = 0;
    }
    osm = 14;
    return dst;
}
int x86Internal::op_BSR(int dst, int src) {
    if (src) {
        dst = 31;
        while (src >= 0) {
            dst--;
            src <<= 1;
        }
        osm_dst = 1;
    } else {
        osm_dst = 0;
    }
    osm = 14;
    return dst;
}
void x86Internal::op_DIV8(int divisor) {
    int a, q, r;
    a = regs[0] & 0xffff;
    divisor &= 0xff;
    if ((a >> 8) >= divisor) {
        abort(0);
    }
    q = a / divisor;
    r = (a % divisor);
    set_lower_word(0, (q & 0xff) | (r << 8));
}
void x86Internal::op_IDIV8(int divisor) {
    int a, q, r;
    a = (regs[0] << 16) >> 16;
    divisor = (divisor << 24) >> 24;
    if (divisor == 0) {
        abort(0);
    }
    q = a / divisor;
    if (((q << 24) >> 24) != q) {
        abort(0);
    }
    r = (a % divisor);
    set_lower_word(0, (q & 0xff) | (r << 8));
}
void x86Internal::op_DIV16(int divisor) {
    int a, q, r;
    a = (regs[2] << 16) | (regs[0] & 0xffff);
    divisor &= 0xffff;
    uint32_t au = a;
    if ((au >> 16) >= divisor) {
        abort(0);
    }
    q = au / divisor;
    r = (au % divisor);
    set_lower_word(0, q);
    set_lower_word(2, r);
}
void x86Internal::op_IDIV16(int divisor) {
    int a, q, r;
    a = (regs[2] << 16) | (regs[0] & 0xffff);
    divisor = (divisor << 16) >> 16;
    if (divisor == 0) {
        abort(0);
    }
    q = a / divisor;
    if (((q << 16) >> 16) != q) {
        abort(0);
    }
    r = (a % divisor);
    set_lower_word(0, q);
    set_lower_word(2, r);
}
int x86Internal::op_DIV32(uint32_t dividend_upper, uint32_t dividend_lower, uint32_t divisor) {
    uint64_t a;
    uint32_t i;
    int negative_dividend;
    if (dividend_upper >= divisor) {
        abort(0);
    }
    if (dividend_upper >= 0 && dividend_upper <= 0x200000) {
        a = dividend_upper * 4294967296 + dividend_lower;
        v = a % divisor;
        return a / divisor;
    } else {
        for (i = 0; i < 32; i++) {
            negative_dividend = dividend_upper >> 31;
            dividend_upper = (dividend_upper << 1) | (dividend_lower >> 31);
            if (negative_dividend || (dividend_upper >= divisor)) {
                dividend_upper = dividend_upper - divisor;
                dividend_lower = (dividend_lower << 1) | 1;
            } else {
                dividend_lower = dividend_lower << 1;
            }
        }
        v = dividend_upper;
        return dividend_lower;
    }
}
int x86Internal::op_IDIV32(int dividend_upper, int dividend_lower, int divisor) {
    int negative_dividend, negative_divisor, q;
    if (dividend_upper < 0) {
        negative_dividend = 1;
        dividend_upper = ~dividend_upper;
        dividend_lower = -dividend_lower;
        if (dividend_lower == 0) {
            dividend_upper = dividend_upper + 1;
        }
    } else {
        negative_dividend = 0;
    }
    if (divisor < 0) {
        divisor = -divisor;
        negative_divisor = 1;
    } else {
        negative_divisor = 0;
    }
    q = op_DIV32(dividend_upper, dividend_lower, divisor);
    negative_divisor ^= negative_dividend;
    if (negative_divisor) {
        if (q > 0x80000000) {
            abort(0);
        }
        q = -q;
    } else {
        if (q >= 0x80000000) {
            abort(0);
        }
    }
    if (negative_dividend) {
        v = -v;
    }
    return q;
}
int x86Internal::op_MUL8(int multiplicand, int multiplier) {
    int x;
    multiplicand &= 0xff;
    multiplier &= 0xff;
    x = (regs[0] & 0xff) * (multiplier & 0xff);
    osm_src = x >> 8;
    osm_dst = ((x << 24) >> 24);
    osm = 21;
    return x;
}
int x86Internal::op_IMUL8(int multiplicand, int multiplier) {
    int x;
    multiplicand = ((multiplicand << 24) >> 24);
    multiplier = ((multiplier << 24) >> 24);
    x = multiplicand * multiplier;
    osm_dst = ((x << 24) >> 24);
    osm_src = x != osm_dst;
    osm = 21;
    return x;
}
int x86Internal::op_MUL16(int multiplicand, int multiplier) {
    int x;
    x = (multiplicand & 0xffff) * (multiplier & 0xffff);
    osm_src = x >> 16;
    osm_dst = ((x << 16) >> 16);
    osm = 22;
    return x;
}
int x86Internal::op_IMUL16(int multiplicand, int multiplier) {
    int x;
    multiplicand = (multiplicand << 16) >> 16;
    multiplier = (multiplier << 16) >> 16;
    x = multiplicand * multiplier;
    osm_dst = ((x << 16) >> 16);
    osm_src = x != osm_dst;
    osm = 22;
    return x;
}
int x86Internal::op_MUL32(int multiplicand, int multiplier) {
    osm_dst = do_multiply32(multiplicand, multiplier);
    osm_src = v;
    osm = 23;
    return osm_dst;
}
int x86Internal::op_IMUL32(int multiplicand, int multiplier) {
    int s, r;
    s = 0;
    if (multiplicand < 0) {
        multiplicand = -multiplicand;
        s = 1;
    }
    if (multiplier < 0) {
        multiplier = -multiplier;
        s ^= 1;
    }
    r = do_multiply32(multiplicand, multiplier);
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
int x86Internal::do_multiply32(int multiplicand, int multiplier) {
    uint32_t Jc, Ic, Tc, Uc, m;
    uint64_t _a = multiplicand;
    uint32_t au = multiplicand;
    uint32_t _opcode = multiplier;
    uint64_t r = _a * _opcode;
    if (r <= 0xffffffff) {
        v = 0;
        r &= -1;
    } else {
        Jc = _a & 0xffff;
        Ic = au >> 16;
        Tc = _opcode & 0xffff;
        Uc = _opcode >> 16;
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
int x86Internal::do_arithmetic8(int operation, int dst, int src) {
    int cf;
    switch (operation & 7) {
    case 0:
        osm_src = src;
        dst = (((dst + src) << 24) >> 24);
        osm_dst = dst;
        osm = 0;
        break;
    case 1:
        dst = (((dst | src) << 24) >> 24);
        osm_dst = dst;
        osm = 12;
        break;
    case 2:
        cf = is_CF();
        osm_src = src;
        dst = (((dst + src + cf) << 24) >> 24);
        osm_dst = dst;
        osm = cf ? 3 : 0;
        break;
    case 3:
        cf = is_CF();
        osm_src = src;
        dst = (((dst - src - cf) << 24) >> 24);
        osm_dst = dst;
        osm = cf ? 9 : 6;
        break;
    case 4:
        dst = (((dst & src) << 24) >> 24);
        osm_dst = dst;
        osm = 12;
        break;
    case 5:
        osm_src = src;
        dst = (((dst - src) << 24) >> 24);
        osm_dst = dst;
        osm = 6;
        break;
    case 6:
        dst = (((dst ^ src) << 24) >> 24);
        osm_dst = dst;
        osm = 12;
        break;
    case 7:
        osm_src = src;
        osm_dst = (((dst - src) << 24) >> 24);
        osm = 6;
        break;
    }
    return dst;
}
int x86Internal::do_arithmetic16(int operation, int dst, int src) {
    int cf;
    switch (operation & 7) {
    case 0:
        osm_src = src;
        dst = (((dst + src) << 16) >> 16);
        osm_dst = dst;
        osm = 1;
        break;
    case 1:
        dst = (((dst | src) << 16) >> 16);
        osm_dst = dst;
        osm = 13;
        break;
    case 2:
        cf = is_CF();
        osm_src = src;
        dst = (((dst + src + cf) << 16) >> 16);
        osm_dst = dst;
        osm = cf ? 4 : 1;
        break;
    case 3:
        cf = is_CF();
        osm_src = src;
        dst = (((dst - src - cf) << 16) >> 16);
        osm_dst = dst;
        osm = cf ? 10 : 7;
        break;
    case 4:
        dst = (((dst & src) << 16) >> 16);
        osm_dst = dst;
        osm = 13;
        break;
    case 5:
        osm_src = src;
        dst = (((dst - src) << 16) >> 16);
        osm_dst = dst;
        osm = 7;
        break;
    case 6:
        dst = (((dst ^ src) << 16) >> 16);
        osm_dst = dst;
        osm = 13;
        break;
    case 7:
        osm_src = src;
        osm_dst = (((dst - src) << 16) >> 16);
        osm = 7;
        break;
    }
    return dst;
}
int x86Internal::do_arithmetic32(int operation, int dst, int src) {
    int cf;
    switch (operation & 7) {
    case 0:
        osm_src = src;
        dst = dst + src;
        osm_dst = dst;
        osm = 2;
        break;
    case 1:
        dst = dst | src;
        osm_dst = dst;
        osm = 14;
        break;
    case 2:
        cf = is_CF();
        osm_src = src;
        dst = dst + src + cf;
        osm_dst = dst;
        osm = cf ? 5 : 2;
        break;
    case 3:
        cf = is_CF();
        osm_src = src;
        dst = dst - src - cf;
        osm_dst = dst;
        osm = cf ? 11 : 8;
        break;
    case 4:
        dst = dst & src;
        osm_dst = dst;
        osm = 14;
        break;
    case 5:
        osm_src = src;
        dst = dst - src;
        osm_dst = dst;
        osm = 8;
        break;
    case 6:
        dst = dst ^ src;
        osm_dst = dst;
        osm = 14;
        break;
    case 7:
        osm_src = src;
        osm_dst = dst - src;
        osm = 8;
        break;
    }
    return dst;
}
int x86Internal::do_shift8(int operation, int src, int count) {
    int kc, cf;
    switch (operation & 7) {
    case 0:
        if (count & 0x1f) {
            count &= 0x7;
            src &= 0xff;
            kc = src;
            src = (src << count) | (src >> (8 - count));
            osm_src = compile_flags(true);
            osm_src |= (src & 0x0001);
            if (count == 1) {
                osm_src |= (((kc ^ src) << 4) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 1:
        if (count & 0x1f) {
            count &= 0x7;
            src &= 0xff;
            kc = src;
            src = (src >> count) | (src << (8 - count));
            osm_src = compile_flags(true);
            osm_src |= ((src >> 7) & 0x0001);
            if (count == 1) {
                osm_src |= (((kc ^ src) << 4) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 2:
        count = do_shift8_LUT[count & 0x1f];
        if (count) {
            src &= 0xff;
            kc = src;
            cf = is_CF();
            src = (src << count) | (cf << (count - 1));
            if (count > 1) {
                src |= kc >> (9 - count);
            }
            osm_src = compile_flags(true);
            osm_src |= ((kc >> (8 - count)) & 0x0001);
            if (count == 1) {
                osm_src |= (((kc ^ src) << 4) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 3:
        count = do_shift8_LUT[count & 0x1f];
        if (count) {
            src &= 0xff;
            kc = src;
            cf = is_CF();
            src = (src >> count) | (cf << (8 - count));
            if (count > 1) {
                src |= kc << (9 - count);
            }
            osm_src = compile_flags(true);
            osm_src |= ((kc >> (count - 1)) & 0x0001);
            if (count == 1) {
                osm_src |= (((kc ^ src) << 4) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 4:
    case 6:
        count &= 0x1f;
        if (count) {
            osm_src = src << (count - 1);
            osm_dst = src = (((src << count) << 24) >> 24);
            osm = 15;
        }
        break;
    case 5:
        count &= 0x1f;
        if (count) {
            src &= 0xff;
            osm_src = src >> (count - 1);
            osm_dst = src = (((src >> count) << 24) >> 24);
            osm = 18;
        }
        break;
    case 7:
        count &= 0x1f;
        if (count) {
            src = (src << 24) >> 24;
            osm_src = src >> (count - 1);
            osm_dst = src = (((src >> count) << 24) >> 24);
            osm = 18;
        }
        break;
    }
    return src;
}
int x86Internal::do_shift16(int operation, int src, int count) {
    int kc, cf;
    switch (operation & 7) {
    case 0:
        if (count & 0x1f) {
            count &= 0xf;
            src &= 0xffff;
            kc = src;
            src = (src << count) | (src >> (16 - count));
            osm_src = compile_flags(true);
            osm_src |= (src & 0x0001);
            if (count == 1) {
                osm_src |= (((kc ^ src) >> 4) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 1:
        if (count & 0x1f) {
            count &= 0xf;
            src &= 0xffff;
            kc = src;
            src = (src >> count) | (src << (16 - count));
            osm_src = compile_flags(true);
            osm_src |= ((src >> 15) & 0x0001);
            if (count == 1) {
                osm_src |= (((kc ^ src) >> 4) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 2:
        count = do_shift16_LUT[count & 0x1f];
        if (count) {
            src &= 0xffff;
            kc = src;
            cf = is_CF();
            src = (src << count) | (cf << (count - 1));
            if (count > 1) {
                src |= kc >> (17 - count);
            }
            osm_src = compile_flags(true);
            osm_src |= ((kc >> (16 - count)) & 0x0001);
            if (count == 1) {
                osm_src |= (((kc ^ src) >> 4) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 3:
        count = do_shift16_LUT[count & 0x1f];
        if (count) {
            src &= 0xffff;
            kc = src;
            cf = is_CF();
            src = (src >> count) | (cf << (16 - count));
            if (count > 1) {
                src |= kc << (17 - count);
            }
            osm_src = compile_flags(true);
            osm_src |= ((kc >> (count - 1)) & 0x0001);
            if (count == 1) {
                osm_src |= (((kc ^ src) >> 4) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 4:
    case 6:
        count &= 0x1f;
        if (count) {
            osm_src = src << (count - 1);
            osm_dst = src = (((src << count) << 16) >> 16);
            osm = 16;
        }
        break;
    case 5:
        count &= 0x1f;
        if (count) {
            src &= 0xffff;
            osm_src = src >> (count - 1);
            osm_dst = src = (((src >> count) << 16) >> 16);
            osm = 19;
        }
        break;
    case 7:
        count &= 0x1f;
        if (count) {
            src = (src << 16) >> 16;
            osm_src = src >> (count - 1);
            osm_dst = src = (((src >> count) << 16) >> 16);
            osm = 19;
        }
        break;
    }
    return src;
}
int x86Internal::do_shift32(int operation, uint32_t src, int count) {
    uint32_t kc;
    int cf;
    switch (operation & 7) {
    case 0:
        count &= 0x1f;
        if (count) {
            kc = src;
            src = (src << count) | (src >> (32 - count));
            osm_src = compile_flags(true);
            osm_src |= (src & 0x0001);
            if (count == 1) {
                osm_src |= (((kc ^ src) >> 20) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 1:
        count &= 0x1f;
        if (count) {
            kc = src;
            src = (src >> count) | (src << (32 - count));
            osm_src = compile_flags(true);
            osm_src |= ((src >> 31) & 0x0001);
            if (count == 1) {
                osm_src |= (((kc ^ src) >> 20) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 2:
        count &= 0x1f;
        if (count) {
            kc = src;
            cf = is_CF();
            src = (src << count) | (cf << (count - 1));
            if (count > 1) {
                src |= kc >> (33 - count);
            }
            osm_src = compile_flags(true);
            osm_src |= ((kc >> (32 - count)) & 0x0001);
            if (count == 1) {
                osm_src |= (((kc ^ src) >> 20) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 3:
        count &= 0x1f;
        if (count) {
            kc = src;
            cf = is_CF();
            src = (src >> count) | (cf << (32 - count));
            if (count > 1) {
                src |= kc << (33 - count);
            }
            osm_src = compile_flags(true);
            osm_src |= ((kc >> (count - 1)) & 0x0001);
            if (count == 1) {
                osm_src |= (((kc ^ src) >> 20) & 0x0800);
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 4:
    case 6:
        count &= 0x1f;
        if (count) {
            osm_src = src << (count - 1);
            osm_dst = src = src << count;
            osm = 17;
        }
        break;
    case 5:
        count &= 0x1f;
        if (count) {
            osm_src = src >> (count - 1);
            osm_dst = src = src >> count;
            osm = 20;
        }
        break;
    case 7:
        count &= 0x1f;
        if (count) {
            int Ybi = src;
            osm_src = Ybi >> (count - 1);
            osm_dst = src = Ybi >> count;
            osm = 20;
        }
        break;
    }
    return src;
}
void x86Internal::op_LDTR(int selector) {
    int dte_lower_dword, dte_upper_dword, index;
    selector &= 0xffff;
    if ((selector & 0xfffc) == 0) {
        ldt.base = 0;
        ldt.limit = 0;
    } else {
        if (selector & 0x4) {
            abort(13, selector & 0xfffc);
        }
        index = selector & ~7;
        if ((index + 7) > gdt.limit) {
            abort(13, selector & 0xfffc);
        }
        mem8_loc = (gdt.base + index) & -1;
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
    int dte_lower_dword, dte_upper_dword, index, descriptor_type;
    selector &= 0xffff;
    if ((selector & 0xfffc) == 0) {
        tr.base = 0;
        tr.limit = 0;
        tr.flags = 0;
    } else {
        if (selector & 0x4) {
            abort(13, selector & 0xfffc);
        }
        index = selector & ~7;
        if ((index + 7) > gdt.limit) {
            abort(13, selector & 0xfffc);
        }
        mem8_loc = (gdt.base + index) & -1;
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
void x86Internal::do_JMPF(int selector, int address) {
    if (!check_protected() || (eflags & 0x00020000)) {
        op_JMPF_virtual_mode(selector, address);
    } else {
        op_JMPF(selector, address);
    }
}
void x86Internal::op_JMPF_virtual_mode(int selector, int address) {
    eip = address, far = far_start = 0;
    segs[1].selector = selector;
    segs[1].base = (selector << 4);
    update_SSB();
}
void x86Internal::op_JMPF(int selector, int address) {
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
        if (address > limit) {
            abort(13, selector & 0xfffc);
        }
        update_segment_register(1, (selector & 0xfffc) | cpl, compile_dte_base(dte_lower_dword, dte_upper_dword), limit,  dte_upper_dword);
        eip = address, far = far_start = 0;
    } else {
        throw "fatal: unsupported TSS or task gate in JMP";
    }
}
void x86Internal::do_CALLF(bool is_operand_size32, int selector, int address, int return_address) {
    if (!check_protected() || (eflags & 0x00020000)) {
        op_CALLF_real__v86_mode(is_operand_size32, selector, address, return_address);
    } else {
        op_CALLF_protected_mode(is_operand_size32, selector, address, return_address);
    }
}
void x86Internal::op_CALLF_real__v86_mode(bool is_operand_size32, int selector, int address, int return_address) {
    int esp = regs[4];
    if (is_operand_size32) {
        esp = esp - 4;
        mem8_loc = (esp & SS_mask) + SS_base;
        st32_mem8_write(segs[1].selector);
        esp = esp - 4;
        mem8_loc = (esp & SS_mask) + SS_base;
        st32_mem8_write(return_address);
    } else {
        esp = esp - 2;
        mem8_loc = (esp & SS_mask) + SS_base;
        st16_mem8_write(segs[1].selector);
        esp = esp - 2;
        mem8_loc = (esp & SS_mask) + SS_base;
        st16_mem8_write(return_address);
    }
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
    eip = address, far = far_start = 0;
    segs[1].selector = selector;
    segs[1].base = (selector << 4);
    update_SSB();
}
void x86Internal::op_CALLF_protected_mode(bool is_operand_size32, int selector, int address, int return_address) {
    int descriptor_table_entry[2], dte_lower_dword, dte_upper_dword, descriptor_type;
    int dpl, rpl, offset, count;
    int SS_base, SS_mask, ss, esp, start_esp, spl;
    // int Ue, Ve;
    if ((selector & 0xfffc) == 0) {
        abort(13, 0);
    }
    load_xdt_descriptor(descriptor_table_entry, selector);
    dte_lower_dword = descriptor_table_entry[0];
    dte_upper_dword = descriptor_table_entry[1];
    if (dte_lower_dword == 0 && dte_upper_dword == 0) {
        abort(13, selector & 0xfffc);
    }
    start_esp = regs[4];
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
        esp = start_esp;
        SS_mask = compile_sizemask(segs[2].flags);
        SS_base = segs[2].base;
        if (is_operand_size32) {
            esp = (esp - 4) & -1;
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            st32_mem8_kernel_write(segs[1].selector);
            esp = (esp - 4) & -1;
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            st32_mem8_kernel_write(return_address);
        } else {
            esp = (esp - 2) & -1;
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            st16_mem8_kernel_write(segs[1].selector);
            esp = (esp - 2) & -1;
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            st16_mem8_kernel_write(return_address);
        }
        int limit = compile_dte_limit(dte_lower_dword, dte_upper_dword);
        if (address > limit) {
            abort(13, selector & 0xfffc);
        }
        regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
        update_segment_register(1, (selector & 0xfffc) | cpl, compile_dte_base(dte_lower_dword, dte_upper_dword), limit, dte_upper_dword);
        eip = address, far = far_start = 0;
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
        is_operand_size32 = descriptor_type >> 3;
        if (dpl < cpl || dpl < rpl) {
            abort(13, selector & 0xfffc);
        }
        if (!(dte_upper_dword & (1 << 15))) {
            abort(11, selector & 0xfffc);
        }
        selector = dte_lower_dword >> 16;
        offset = (dte_upper_dword & 0xffff0000) | (dte_lower_dword & 0x0000ffff);
        count = dte_upper_dword & 0x1f;
        if ((selector & 0xfffc) == 0) {
            abort(13, 0);
        }
        load_xdt_descriptor(descriptor_table_entry, selector);
        dte_lower_dword = descriptor_table_entry[0];
        dte_upper_dword = descriptor_table_entry[1];
        if (dte_lower_dword == 0 && dte_upper_dword == 0) {
            abort(13, selector & 0xfffc);
        }
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
        if (!(dte_upper_dword & (1 << 10)) && dpl < cpl) { // bit 10 == 0, code (or data) segment descriptor
            int dte_lower_dword, dte_upper_dword;
            load_tss_interlevel(descriptor_table_entry, dpl);
            ss = descriptor_table_entry[0];
            esp = descriptor_table_entry[1];
            if ((ss & 0xfffc) == 0) {
                abort(10, ss & 0xfffc);
            }
            if ((ss & 3) != dpl) {
                abort(10, ss & 0xfffc);
            }
            load_xdt_descriptor(descriptor_table_entry, ss);
            dte_lower_dword = descriptor_table_entry[0];
            dte_upper_dword = descriptor_table_entry[1];
            if (dte_lower_dword == 0 && dte_upper_dword == 0) {
                abort(10, ss & 0xfffc);
            }
            spl = (dte_upper_dword >> 13) & 3;
            if (spl != dpl) {
                abort(10, ss & 0xfffc);
            }
            if (!(dte_upper_dword & (1 << 12)) || (dte_upper_dword & (1 << 11)) || !(dte_upper_dword & (1 << 9))) {
                abort(10, ss & 0xfffc);
            }
            if (!(dte_upper_dword & (1 << 15))) {
                abort(10, ss & 0xfffc);
            }
            // Ue = compile_sizemask(segs[2].flags);
            // Ve = segs[2].base;
            int x = 0;
            SS_mask = compile_sizemask(dte_upper_dword);
            SS_base = compile_dte_base(dte_lower_dword, dte_upper_dword);
            if (is_operand_size32) {
                esp = (esp - 4) & -1;
                mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                st32_mem8_kernel_write(segs[2].selector);
                esp = (esp - 4) & -1;
                mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                st32_mem8_kernel_write(start_esp);
                for (int i = count - 1; i >= 0; i--) {
                    // x = Xe(Ve + ((start_esp + i * 4) & Ue));
                    esp = (esp - 4) & -1;
                    mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                    st32_mem8_kernel_write(x);
                }
            } else {
                esp = (esp - 2) & -1;
                mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                st16_mem8_kernel_write(segs[2].selector);
                esp = (esp - 2) & -1;
                mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                st16_mem8_kernel_write(start_esp);
                for (int i = count - 1; i >= 0; i--) {
                    // x = Ye(Ve + ((start_esp + i * 2) & Ue));
                    esp = (esp - 2) & -1;
                    mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                    st16_mem8_kernel_write(x);
                }
            }
            ss = (ss & ~3) | dpl;
            update_segment_register(2, ss, SS_base, compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
        } else {
            esp = start_esp;
            SS_mask = compile_sizemask(segs[2].flags);
            SS_base = segs[2].base;
        }
        if (is_operand_size32) {
            esp = (esp - 4) & -1;
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            st32_mem8_kernel_write(segs[1].selector);
            esp = (esp - 4) & -1;
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            st32_mem8_kernel_write(return_address);
        } else {
            esp = (esp - 2) & -1;
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            st16_mem8_kernel_write(segs[1].selector);
            esp = (esp - 2) & -1;
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            st16_mem8_kernel_write(return_address);
        }
        selector = (selector & ~3) | dpl;
        update_segment_register(1, selector, compile_dte_base(dte_lower_dword, dte_upper_dword), compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
        set_current_privilege_level(dpl);
        regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
        eip = offset, far = far_start = 0;
    }
}
void x86Internal::do_return_real__v86_mode(bool is_operand_size32, bool is_iret, int return_offset) {
    int cs, esp, stack_eip, stack_eflags, SS_base, SS_mask;
    esp = regs[4];
    SS_base = segs[2].base;
    SS_mask = 0xffff;
    if (is_operand_size32 == 1) {
        mem8_loc = (SS_base + (esp & SS_mask)) & -1;
        stack_eip = ld32_mem8_kernel_read();
        esp = (esp + 4) & -1;
        mem8_loc = (SS_base + (esp & SS_mask)) & -1;
        cs = ld32_mem8_kernel_read();
        esp = (esp + 4) & -1;
        cs &= 0xffff;
        if (is_iret) {
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            stack_eflags = ld32_mem8_kernel_read();
            esp = (esp + 4) & -1;
        }
    } else {
        mem8_loc = (SS_base + (esp & SS_mask)) & -1;
        stack_eip = ld16_mem8_kernel_read();
        esp = (esp + 2) & -1;
        mem8_loc = (SS_base + (esp & SS_mask)) & -1;
        cs = ld16_mem8_kernel_read();
        esp = (esp + 2) & -1;
        if (is_iret) {
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            stack_eflags = ld16_mem8_kernel_read();
            esp = (esp + 2) & -1;
        }
    }
    regs[4] = (regs[4] & ~SS_mask) | ((esp + return_offset) & SS_mask);
    segs[1].selector = cs;
    segs[1].base = (cs << 4);
    eip = stack_eip, far = far_start = 0;
    if (is_iret) {
        int mask;
        if (eflags & 0x00020000) {
            mask = 0x00000100 | 0x00000200 | 0x00004000 | 0x00010000 | 0x00040000 | 0x00200000;
        } else {
            mask = 0x00000100 | 0x00000200 | 0x00003000 | 0x00004000 | 0x00010000 | 0x00040000 | 0x00200000;
        }
        if (is_operand_size32 == 0) {
            mask &= 0xffff;
        }
        set_EFLAGS(stack_eflags, mask);
    }
    update_SSB();
}
void x86Internal::do_return_protected_mode(bool is_operand_size32, bool is_iret, int return_offset) {
    int SS_base, SS_mask, esp, stack_esp, stack_eip, stack_eflags = 0;
    int descriptor_table_entry[2], dte_lower_dword, dte_upper_dword;
    int _cpl = cpl, dpl, rpl, iopl, es, cs, ss, ds, fs, gs;
    esp = regs[4];
    SS_base = segs[2].base;
    SS_mask = compile_sizemask(segs[2].flags);
    if (is_operand_size32 == 1) {
        mem8_loc = (SS_base + (esp & SS_mask)) & -1;
        stack_eip = ld32_mem8_kernel_read();
        esp = (esp + 4) & -1;
        mem8_loc = (SS_base + (esp & SS_mask)) & -1;
        cs = ld32_mem8_kernel_read();
        esp = (esp + 4) & -1;
        cs &= 0xffff;
        if (is_iret) {
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            stack_eflags = ld32_mem8_kernel_read();
            esp = (esp + 4) & -1;
            if (stack_eflags & 0x00020000) {
                mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                stack_esp = ld32_mem8_kernel_read();
                esp = (esp + 4) & -1;
                // pop segment selectors from stack
                mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                ss = ld32_mem8_kernel_read();
                esp = (esp + 4) & -1;
                mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                es = ld32_mem8_kernel_read();
                esp = (esp + 4) & -1;
                mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                ds = ld32_mem8_kernel_read();
                esp = (esp + 4) & -1;
                mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                fs = ld32_mem8_kernel_read();
                esp = (esp + 4) & -1;
                mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                gs = ld32_mem8_kernel_read();
                esp = (esp + 4) & -1;
                // clang-format off
                set_EFLAGS(stack_eflags, 0x00000100 | 0x00000200 |
                                         0x00003000 | 0x00004000 |
                                         0x00020000 | 0x00040000 | 0x00080000 |
                                         0x00100000 | 0x00200000);
                // clang-format on
                set_segment_register_real__v86(0, es & 0xffff);
                set_segment_register_real__v86(1, cs & 0xffff);
                set_segment_register_real__v86(2, ss & 0xffff);
                set_segment_register_real__v86(3, ds & 0xffff);
                set_segment_register_real__v86(4, fs & 0xffff);
                set_segment_register_real__v86(5, gs & 0xffff);
                eip = stack_eip & 0xffff, far = far_start = 0;
                regs[4] = (regs[4] & ~SS_mask) | (stack_esp & SS_mask);
                set_current_privilege_level(3);
                return;
            }
        }
    } else {
        mem8_loc = (SS_base + (esp & SS_mask)) & -1;
        stack_eip = ld16_mem8_kernel_read();
        esp = (esp + 2) & -1;
        mem8_loc = (SS_base + (esp & SS_mask)) & -1;
        cs = ld16_mem8_kernel_read();
        esp = (esp + 2) & -1;
        if (is_iret) {
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            stack_eflags = ld16_mem8_kernel_read();
            esp = (esp + 2) & -1;
        }
    }
    if ((cs & 0xfffc) == 0) {
        abort(13, cs & 0xfffc);
    }
    load_xdt_descriptor(descriptor_table_entry, cs);
    dte_lower_dword = descriptor_table_entry[0];
    dte_upper_dword = descriptor_table_entry[1];
    if (dte_lower_dword == 0 && dte_upper_dword == 0) {
        abort(13, cs & 0xfffc);
    }
    if (!(dte_upper_dword & (1 << 12)) || !(dte_upper_dword & (1 << 11))) {
        abort(13, cs & 0xfffc);
    }
    rpl = cs & 3;
    if (rpl < _cpl) {
        abort(13, cs & 0xfffc);
    }
    dpl = (dte_upper_dword >> 13) & 3;
    if (dte_upper_dword & (1 << 10)) {
        if (dpl > rpl) {
            abort(13, cs & 0xfffc);
        }
    } else {
        if (dpl != rpl) {
            abort(13, cs & 0xfffc);
        }
    }
    if (!(dte_upper_dword & (1 << 15))) {
        abort(11, cs & 0xfffc);
    }
    esp = (esp + return_offset) & -1;
    if (rpl == _cpl) {
        update_segment_register(1, cs, compile_dte_base(dte_lower_dword, dte_upper_dword), compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
    } else {
        if (is_operand_size32 == 1) {
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            stack_esp = ld32_mem8_kernel_read();
            esp = (esp + 4) & -1;
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            ss = ld32_mem8_kernel_read();
            esp = (esp + 4) & -1;
            ss &= 0xffff;
        } else {
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            stack_esp = ld16_mem8_kernel_read();
            esp = (esp + 2) & -1;
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            ss = ld16_mem8_kernel_read();
            esp = (esp + 2) & -1;
        }
        if ((ss & 0xfffc) == 0) {
            abort(13, 0);
        } else {
            int dte_lower_dword, dte_upper_dword;
            if ((ss & 3) != rpl) {
                abort(13, ss & 0xfffc);
            }
            load_xdt_descriptor(descriptor_table_entry, ss);
            dte_lower_dword = descriptor_table_entry[0];
            dte_upper_dword = descriptor_table_entry[1];
            if (dte_lower_dword == 0 && dte_upper_dword == 0) {
                abort(13, ss & 0xfffc);
            }
            if (!(dte_upper_dword & (1 << 12)) || (dte_upper_dword & (1 << 11)) || !(dte_upper_dword & (1 << 9))) {
                abort(13, ss & 0xfffc);
            }
            dpl = (dte_upper_dword >> 13) & 3;
            if (dpl != rpl) {
                abort(13, ss & 0xfffc);
            }
            if (!(dte_upper_dword & (1 << 15))) {
                abort(11, ss & 0xfffc);
            }
            update_segment_register(2, ss, compile_dte_base(dte_lower_dword, dte_upper_dword), compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
        }
        clear_segment_register(0, rpl);
        update_segment_register(1, cs, compile_dte_base(dte_lower_dword, dte_upper_dword), compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
        clear_segment_register(3, rpl);
        clear_segment_register(4, rpl);
        clear_segment_register(5, rpl);
        esp = (stack_esp + return_offset) & -1;
        SS_mask = compile_sizemask(dte_upper_dword);
        set_current_privilege_level(rpl);
    }
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
    eip = stack_eip, far = far_start = 0;
    if (is_iret) {
        int mask = 0x00000100 | 0x00004000 | 0x00010000 | 0x00040000 | 0x00200000;
        if (_cpl == 0) {
            mask |= 0x00003000;
        }
        iopl = (eflags >> 12) & 3;
        if (_cpl <= iopl) {
            mask |= 0x00000200;
        }
        if (is_operand_size32 == 0) {
            mask &= 0xffff;
        }
        set_EFLAGS(stack_eflags, mask);
    }
}
void x86Internal::clear_segment_register(int sreg, int privilege_level) {
    int dpl, dte_upper_dword;
    if ((sreg == 4 || sreg == 5) && (segs[sreg].selector & 0xfffc) == 0) {
        return; // null selector in FS, GS
    }
    dte_upper_dword = segs[sreg].flags;
    dpl = (dte_upper_dword >> 13) & 3;
    if (!(dte_upper_dword & (1 << 11)) || !(dte_upper_dword & (1 << 10))) {
        if (dpl < privilege_level) {
            update_segment_register(sreg, 0, 0, 0, 0);
        }
    }
}
void x86Internal::op_IRET(bool is_operand_size32) {
    int iopl;
    if (!check_protected() || (eflags & 0x00020000)) {
        if (eflags & 0x00020000) {
            iopl = (eflags >> 12) & 3;
            if (iopl != 3) {
                abort(13);
            }
        }
        do_return_real__v86_mode(is_operand_size32, 1, 0);
    } else {
        if (eflags & 0x00004000) {
            throw "fatal: unsupported EFLAGS.NT == 1 in IRET";
        } else {
            do_return_protected_mode(is_operand_size32, 1, 0);
        }
    }
}
void x86Internal::op_RETF(bool is_operand_size32, int return_offset) {
    if (!check_protected() || (eflags & 0x00020000)) {
        do_return_real__v86_mode(is_operand_size32, 0, return_offset);
    } else {
        do_return_protected_mode(is_operand_size32, 0, return_offset);
    }
}
int x86Internal::ld_descriptor_field(int selector, bool is_lsl) {
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
void x86Internal::op_LAR_LSL(bool is_operand_size32, bool is_lsl) {
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
        selector = ld16_mem8_read();
    }
    x = ld_descriptor_field(selector, is_lsl);
    osm_src = compile_flags();
    if (x == -1) {
        osm_src &= ~0x0040;
    } else {
        osm_src |= 0x0040;
        if (is_operand_size32) {
            regs[reg_idx1] = x;
        } else {
            set_lower_word(reg_idx1, x);
        }
    }
    osm_dst = ((osm_src >> 6) & 1) ^ 1;
    osm = 24;
}
void x86Internal::do_interrupt(int interrupt_id, int is_sw, int error_code, int return_address, int is_hw) {
    if (interrupt_id == 0x06) {
        int n, eip_linear;
        std::string str =
            "interrupt_id=" + _1_byte(interrupt_id) +
            " error_code=" + _4_bytes(error_code) +
            " EIP=" + _4_bytes(eip) + " ESP=" + _4_bytes(regs[4]) +
            " EAX=" + _4_bytes(regs[0]) + " EBX=" + _4_bytes(regs[3]) +
            " ECX=" + _4_bytes(regs[1]);
        printf("%s\n", str.c_str());
        eip_linear = eip + CS_base;
        n = 4096 - (eip_linear & 0xfff);
        n = std::min(n, 15);
        str = "[EIP..EIP+" + _1_byte(n) + "]:";
        for (int i = 0; i < n; i++) {
            mem8_loc = (eip_linear + i) & -1;
            str += " " + _1_byte(ld8_mem8_read());
        }
        printf("%s\n", str.c_str());
    }
    if (check_protected()) {
        do_interrupt_protected_mode(interrupt_id, is_sw, error_code, return_address, is_hw);
    } else {
        do_interrupt_real__v86_mode(interrupt_id, is_sw, error_code, return_address, is_hw);
    }
}
void x86Internal::do_interrupt_real__v86_mode(int interrupt_id, int is_sw, int error_code, int return_address, int is_hw) {
    int selector, offset, esp, _return_address;
    if (interrupt_id * 4 + 3 > idt.limit) {
        abort(13, interrupt_id * 8 + 2);
    }
    mem8_loc = idt.base + (interrupt_id << 2);
    offset = ld16_mem8_kernel_read();
    mem8_loc = mem8_loc + 2;
    selector = ld16_mem8_kernel_read();
    esp = regs[4];
    if (is_sw) {
        _return_address = return_address;
    } else {
        _return_address = eip;
    }
    esp = esp - 2;
    mem8_loc = (esp & SS_mask) + SS_base;
    st16_mem8_write(get_EFLAGS());
    esp = esp - 2;
    mem8_loc = (esp & SS_mask) + SS_base;
    st16_mem8_write(segs[1].selector);
    esp = esp - 2;
    mem8_loc = (esp & SS_mask) + SS_base;
    st16_mem8_write(_return_address);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
    eip = offset, far = far_start = 0;
    segs[1].selector = selector;
    segs[1].base = (selector << 4);
    eflags &= ~(0x00000100 | 0x00000200 | 0x00010000 | 0x00040000);
}
void x86Internal::do_interrupt_protected_mode(int interrupt_id, int is_sw, int error_code, int return_address, int is_hw) {
    int selector, offset, st_error_code, is_interlevel, is_386;
    int descriptor_table_entry[2], dte_lower_dword, dte_upper_dword, descriptor_type;
    int _return_address, SS_base, SS_mask, ss, esp, spl, ss_dte_upper_dword, ss_dte_lower_dword;
    st_error_code = 0;
    if (!is_sw && !is_hw) {
        switch (interrupt_id) { // with error codes, Intel IA-32 SDM (latest), Vol. 3A, 7.3
        case 8:  // double exception
        case 10: // invalid task state segment
        case 11: // segment not present
        case 12: // stack fault
        case 13: // general protection
        case 14: // page fault
        case 17: // alignment check (80486)
            st_error_code = 1;
            break;
        }
    }
    if (is_sw) {
        _return_address = return_address;
    } else {
        _return_address = eip;
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
    case 5: // 16/ 32 bit task gate
    case 6: // 16 bit interrupt gate
    case 7: // 16 bit trap gate
        throw "fatal: unsupported gate type";
        break;
    default:
        abort(13, interrupt_id * 8 + 2);
        break;
    }
    dpl = (dte_upper_dword >> 13) & 3;
    if (is_sw && dpl < cpl) {
        abort(13, interrupt_id * 8 + 2);
    }
    if (!(dte_upper_dword & (1 << 15))) {
        abort(11, interrupt_id * 8 + 2);
    }
    selector = dte_lower_dword >> 16;
    offset = (dte_upper_dword & -65536) | (dte_lower_dword & 0x0000ffff);
    if ((selector & 0xfffc) == 0) {
        abort(13, 0);
    }
    load_xdt_descriptor(descriptor_table_entry, selector);
    dte_lower_dword = descriptor_table_entry[0];
    dte_upper_dword = descriptor_table_entry[1];
    if (dte_lower_dword == 0 && dte_upper_dword == 0) {
        abort(13, selector & 0xfffc);
    }
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
    if (!(dte_upper_dword & (1 << 10)) && dpl < cpl) { // bit 10 == 0, code (or data) segment descriptor
        load_tss_interlevel(descriptor_table_entry, dpl);
        ss = descriptor_table_entry[0];
        esp = descriptor_table_entry[1];
        if ((ss & 0xfffc) == 0) {
            abort(10, ss & 0xfffc);
        }
        if ((ss & 3) != dpl) {
            abort(10, ss & 0xfffc);
        }
        load_xdt_descriptor(descriptor_table_entry, ss);
        ss_dte_upper_dword = descriptor_table_entry[0];
        ss_dte_lower_dword = descriptor_table_entry[1];
        if (ss_dte_upper_dword == 0 && ss_dte_lower_dword == 0) {
            abort(10, ss & 0xfffc);
        }
        spl = (ss_dte_lower_dword >> 13) & 3;
        if (spl != dpl) {
            abort(10, ss & 0xfffc);
        }
        if (!(ss_dte_lower_dword & (1 << 12)) || (ss_dte_lower_dword & (1 << 11)) || !(ss_dte_lower_dword & (1 << 9))) {
            abort(10, ss & 0xfffc);
        }
        if (!(ss_dte_lower_dword & (1 << 15))) {
            abort(10, ss & 0xfffc);
        }
        SS_mask = compile_sizemask(ss_dte_lower_dword);
        SS_base = compile_dte_base(ss_dte_upper_dword, ss_dte_lower_dword);
        is_interlevel = 1;
    } else if ((dte_upper_dword & (1 << 10)) || dpl == cpl) {
        if (eflags & 0x00020000) {
            abort(13, selector & 0xfffc);
        }
        dpl = cpl;
        SS_mask = compile_sizemask(segs[2].flags);
        SS_base = segs[2].base;
        esp = regs[4];
        is_interlevel = 0;
    } else {
        abort(13, selector & 0xfffc);
    }
    is_386 = descriptor_type >> 3;
    if (is_386 == 1) {
        if (is_interlevel) {
            if (eflags & 0x00020000) {
                esp = (esp - 4) & -1;
                mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                st32_mem8_kernel_write(segs[5].selector);
                esp = (esp - 4) & -1;
                mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                st32_mem8_kernel_write(segs[4].selector);
                esp = (esp - 4) & -1;
                mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                st32_mem8_kernel_write(segs[3].selector);
                esp = (esp - 4) & -1;
                mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                st32_mem8_kernel_write(segs[0].selector);
            }
            esp = (esp - 4) & -1;
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            st32_mem8_kernel_write(segs[2].selector);
            esp = (esp - 4) & -1;
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            st32_mem8_kernel_write(regs[4]);
        }
        esp = (esp - 4) & -1;
        mem8_loc = (SS_base + (esp & SS_mask)) & -1;
        st32_mem8_kernel_write(get_EFLAGS());
        esp = (esp - 4) & -1;
        mem8_loc = (SS_base + (esp & SS_mask)) & -1;
        st32_mem8_kernel_write(segs[1].selector);
        esp = (esp - 4) & -1;
        mem8_loc = (SS_base + (esp & SS_mask)) & -1;
        st32_mem8_kernel_write(_return_address);
        if (st_error_code) {
            esp = (esp - 4) & -1;
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            st32_mem8_kernel_write(error_code);
        }
    } else {
        if (is_interlevel) {
            if (eflags & 0x00020000) {
                esp = (esp - 2) & -1;
                mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                st16_mem8_kernel_write(segs[5].selector);
                esp = (esp - 2) & -1;
                mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                st16_mem8_kernel_write(segs[4].selector);
                esp = (esp - 2) & -1;
                mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                st16_mem8_kernel_write(segs[3].selector);
                esp = (esp - 2) & -1;
                mem8_loc = (SS_base + (esp & SS_mask)) & -1;
                st16_mem8_kernel_write(segs[0].selector);
            }
            esp = (esp - 2) & -1;
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            st16_mem8_kernel_write(segs[2].selector);
            esp = (esp - 2) & -1;
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            st16_mem8_kernel_write(regs[4]);
        }
        esp = (esp - 2) & -1;
        mem8_loc = (SS_base + (esp & SS_mask)) & -1;
        st16_mem8_kernel_write(get_EFLAGS());
        esp = (esp - 2) & -1;
        mem8_loc = (SS_base + (esp & SS_mask)) & -1;
        st16_mem8_kernel_write(segs[1].selector);
        esp = (esp - 2) & -1;
        mem8_loc = (SS_base + (esp & SS_mask)) & -1;
        st16_mem8_kernel_write(_return_address);
        if (st_error_code) {
            esp = (esp - 2) & -1;
            mem8_loc = (SS_base + (esp & SS_mask)) & -1;
            st16_mem8_kernel_write(error_code);
        }
    }
    if (is_interlevel) {
        if (eflags & 0x00020000) {
            update_segment_register(0, 0, 0, 0, 0);
            update_segment_register(3, 0, 0, 0, 0);
            update_segment_register(4, 0, 0, 0, 0);
            update_segment_register(5, 0, 0, 0, 0);
        }
        ss = (ss & ~3) | dpl;
        update_segment_register(2, ss, SS_base, compile_dte_limit(ss_dte_upper_dword, ss_dte_lower_dword), ss_dte_lower_dword);
    }
    selector = (selector & ~3) | dpl;
    update_segment_register(1, selector, compile_dte_base(dte_lower_dword, dte_upper_dword), compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
    set_current_privilege_level(dpl);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
    eip = offset, far = far_start = 0;
    if ((descriptor_type & 1) == 0) {
        eflags &= ~0x00000200;
    }
    eflags &= ~(0x00000100 | 0x00004000 | 0x00010000 | 0x00020000);
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
        x = ld16_mem8_write();
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
void x86Internal::op_AAM(int radix) {
    int wf, xf;
    if (radix == 0) {
        abort(0);
    }
    wf = regs[0] & 0xff;
    xf = (wf / radix) & -1;
    wf = (wf % radix);
    regs[0] = (regs[0] & ~0xffff) | wf | (xf << 8);
    osm_dst = ((wf << 24) >> 24);
    osm = 12;
}
void x86Internal::op_AAD(int radix) {
    int wf, xf;
    wf = regs[0] & 0xff;
    xf = (regs[0] >> 8) & 0xff;
    wf = (xf * radix + wf) & 0xff;
    regs[0] = (regs[0] & ~0xffff) | wf;
    osm_dst = ((wf << 24) >> 24);
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
void x86Internal::op_BOUND16() {
    int mem8, x, y, z;
    mem8 = phys_mem8[far++];
    if ((mem8 >> 6) == 3) {
        abort(6);
    }
    mem8_loc = segment_translation(mem8);
    x = (ld16_mem8_read() << 16) >> 16;
    mem8_loc = (mem8_loc + 2) & -1;
    y = (ld16_mem8_read() << 16) >> 16;
    reg_idx1 = (mem8 >> 3) & 7;
    z = (regs[reg_idx1] << 16) >> 16;
    if (z < x || z > y) {
        abort(5);
    }
}
void x86Internal::op_BOUND() {
    int mem8, x, y, z;
    mem8 = phys_mem8[far++];
    if ((mem8 >> 6) == 3) {
        abort(6);
    }
    mem8_loc = segment_translation(mem8);
    x = ld32_mem8_read();
    mem8_loc = (mem8_loc + 4) & -1;
    y = ld32_mem8_read();
    reg_idx1 = (mem8 >> 3) & 7;
    z = regs[reg_idx1];
    if (z < x || z > y) {
        abort(5);
    }
}
void x86Internal::op_PUSHA16() {
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
void x86Internal::op_POPA16() {
    int reg_idx1;
    mem8_loc = (regs[4] & SS_mask) + SS_base;
    for (reg_idx1 = 7; reg_idx1 >= 0; reg_idx1--) {
        if (reg_idx1 != 4) {
            set_lower_word(reg_idx1, ld16_mem8_read());
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
            regs[reg_idx1] = ld32_mem8_read();
        }
        mem8_loc = mem8_loc + 4;
    }
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 32) & SS_mask);
}
void x86Internal::op_LEAVE16() {
    int x, y;
    y = regs[5];
    mem8_loc = (y & SS_mask) + SS_base;
    x = ld16_mem8_read();
    set_lower_word(5, x);
    regs[4] = (regs[4] & ~SS_mask) | ((y + 2) & SS_mask);
}
void x86Internal::op_LEAVE() {
    int x, y;
    y = regs[5];
    mem8_loc = (y & SS_mask) + SS_base;
    x = ld32_mem8_read();
    regs[5] = x;
    regs[4] = (regs[4] & ~SS_mask) | ((y + 4) & SS_mask);
}
void x86Internal::op_ENTER16() {
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
            x = ld16_mem8_read();
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
    ld16_mem8_write();
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
            x = ld32_mem8_read();
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
    ld32_mem8_write();
    regs[5] = (regs[5] & ~SS_mask) | (Sf & SS_mask);
    regs[4] = (regs[4] & ~SS_mask) | (le & SS_mask);
}
void x86Internal::ld_full_pointer16(int sreg) {
    int x, y, mem8;
    mem8 = phys_mem8[far++];
    if ((mem8 >> 3) == 3) {
        ; // abort(6);
    }
    mem8_loc = segment_translation(mem8);
    x = ld16_mem8_read();
    mem8_loc += 2;
    y = ld16_mem8_read();
    set_segment_register(sreg, y);
    set_lower_word((mem8 >> 3) & 7, x);
}
void x86Internal::ld_full_pointer32(int sreg) {
    int x, y, mem8;
    mem8 = phys_mem8[far++];
    if ((mem8 >> 3) == 3) {
        ; // abort(6);
    }
    mem8_loc = segment_translation(mem8);
    x = ld32_mem8_read();
    mem8_loc += 4;
    y = ld16_mem8_read();
    set_segment_register(sreg, y);
    regs[(mem8 >> 3) & 7] = x;
}
void x86Internal::op_INS16() {
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
void x86Internal::op_OUTS16() {
    int Xf, cg, sreg, ag, Zf, iopl, x;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl) {
        abort(13);
    }
    if (ipr & 0x0080) {
        Xf = 0xffff;
    } else {
        Xf = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    cg = regs[6];
    Zf = regs[2] & 0xffff;
    if (ipr & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0) {
            return;
        }
        mem8_loc = (cg & Xf) + segs[sreg].base;
        x = ld16_mem8_read();
        st16_port(Zf, x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf) {
            far = far_start;
        }
    } else {
        mem8_loc = (cg & Xf) + segs[sreg].base;
        x = ld16_mem8_read();
        st16_port(Zf, x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
    }
}
void x86Internal::op_MOVS16() {
    int Xf, Yf, cg, ag, sreg, eg;
    if (ipr & 0x0080) {
        Xf = 0xffff;
    } else {
        Xf = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    cg = regs[6];
    Yf = regs[7];
    mem8_loc = (cg & Xf) + segs[sreg].base;
    eg = (Yf & Xf) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0) {
            return;
        }
        x = ld16_mem8_read();
        mem8_loc = eg;
        st16_mem8_write(x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf) {
            far = far_start;
        }
    } else {
        x = ld16_mem8_read();
        mem8_loc = eg;
        st16_mem8_write(x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
    }
}
void x86Internal::op_STOS16() {
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
void x86Internal::op_CMPS16() {
    int Xf, Yf, cg, ag, sreg, eg;
    if (ipr & 0x0080) {
        Xf = 0xffff;
    } else {
        Xf = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    cg = regs[6];
    Yf = regs[7];
    mem8_loc = (cg & Xf) + segs[sreg].base;
    eg = (Yf & Xf) + segs[0].base;
    if (ipr & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0) {
            return;
        }
        x = ld16_mem8_read();
        mem8_loc = eg;
        y = ld16_mem8_read();
        do_arithmetic16(7, x, y);
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
        x = ld16_mem8_read();
        mem8_loc = eg;
        y = ld16_mem8_read();
        do_arithmetic16(7, x, y);
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
    }
}
void x86Internal::op_LODS16() {
    int Xf, cg, sreg, ag, x;
    if (ipr & 0x0080) {
        Xf = 0xffff;
    } else {
        Xf = -1;
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    cg = regs[6];
    mem8_loc = (cg & Xf) + segs[sreg].base;
    if (ipr & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0) {
            return;
        }
        x = ld16_mem8_read();
        regs[0] = (regs[0] & -65536) | x;
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf) {
            far = far_start;
        }
    } else {
        x = ld16_mem8_read();
        regs[0] = (regs[0] & -65536) | x;
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
    }
}
void x86Internal::op_SCAS16() {
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
        x = ld16_mem8_read();
        do_arithmetic16(7, regs[0], x);
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
        x = ld16_mem8_read();
        do_arithmetic16(7, regs[0], x);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
    }
}
