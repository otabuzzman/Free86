#include "free86.h"

Free86::Free86(int memory_size) {
    this->memory_size = memory_size;
    // size plus maximum possible number of bytes per instruction,
    // rounded up to the nearest multiple of 32 bits, as buffer
    // for instructions that span page boundaries.
    memory8 = (uint8_t *) calloc(1, memory_size + ((15 + 3) & ~3));
    memory16 = (uint16_t *) (memory8);
    memory = (uint32_t *) (memory8);
    tlb_readonly_cplX = new int[tlb_size];
    tlb_writable_cplX = new int[tlb_size];
    tlb_readonly_cpl3 = new int[tlb_size];
    tlb_writable_cpl3 = new int[tlb_size];
    for (int i = 0; i < tlb_size; i++) {
        tlb_clear(i);
    }
    reset();
    cycles = 0;
    set_cpl(0); // PM (1986), 10.3
}
Free86::~Free86() {
    free(memory8);
    delete[] tlb_readonly_cplX;
    delete[] tlb_writable_cplX;
    delete[] tlb_readonly_cpl3;
    delete[] tlb_writable_cpl3;
}
void Free86::reset() {
    // chip state variables (Intel 64 IA-32 SDM, Vol. 3A, 11.1.1)
    for (int i = 0 ; i < 8 ; i++) {
        regs[i] = 0;
    }
    eflags = 0x2;
    eip = 0xfff0;
    for (int i = 0 ; i < 7 ; i++) {
        segs[i] = {0, 0, 0, 0};
    }
    segs[1] = {0, 0xffff0000, 0, 0};
    idt = {0, 0, 0x03ff, 0};
    cr0 = 1 << 4; // 80387 present (Vol. 3A, p. 2-16)
    // emulator state variables
    halted = 0;
}
[[noreturn]] void Free86::abort(int id, int error_code) {
    this->cycles += cycles_requested - cycles_remaining;
    interrupt = {id, error_code};
    throw interrupt;
}
void Free86::update_SSB() {
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
void Free86::fetch_opcode() {
    eip = eip + far - far_start;
    eip_linear = is_real__v86() ? (eip + CS_base) & 0xfffff : eip + CS_base;
    int64_t eip_tlb_hash = tlb_readonly[eip_linear >> 12];
    // `eip_tlb_hash' equals -1 or instruction with maximum bytes (15)
    // would extend across the page boundary.
    // combined check ok because bits 0-11 in `eip_tlb_hash' always 0.
    if (((eip_tlb_hash | eip_linear) & 0xfff) > (4096 - 15)) {
        if (eip_tlb_hash == -1) {
            page_translation(0, cpl == 3, eip_linear);
        }
        eip_tlb_hash = tlb_readonly[eip_linear >> 12];
        far = far_start = eip_linear ^ eip_tlb_hash;
        opcode = ld8_direct();
        int page_offset = eip_linear & 0xfff;
        if (page_offset > (4096 - 15)) {
            x = instruction_length(opcode);
            if ((page_offset + x) > 4096) { // instruction extends page boundary
                far = far_start = memory_size;
                for (y = 0; y < x; y++) { // copy instruction to dedicated buffer on top of memory
                    lax = eip_linear + y;
                    memory8[far + y] = ld8_readonly_cpl3();
                }
                far++;
            }
        }
    } else {
        far = far_start = eip_linear ^ eip_tlb_hash;
        opcode = ld8_direct();
    }
}
int Free86::instruction_length(int opcode) {
    int ipr, operation, stride;
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
            lax = eip_linear + (n++);
            opcode = ld8_readonly_cpl3();
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
            lax = eip_linear + (n++);
            opcode = ld8_readonly_cpl3();
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
            lax = eip_linear + (n++);
            modRM = ld8_readonly_cpl3();
            if (ipr & 0x0080) {
                switch (modRM >> 6) {
                case 0:
                    if ((modRM & 7) == 6) {
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
                switch ((modRM & 7) | ((modRM >> 3) & 0x18)) {
                case 0x04:
                    if ((n + 1) > 15) {
                        abort(13);
                    }
                    lax = eip_linear + (n++);
                    modRM = ld8_readonly_cpl3();
                    if ((modRM & 7) == 5) {
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
            lax = eip_linear + (n++);
            modRM = ld8_readonly_cpl3();
            if (ipr & 0x0080) {
                switch (modRM >> 6) {
                case 0:
                    if ((modRM & 7) == 6) {
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
                switch ((modRM & 7) | ((modRM >> 3) & 0x18)) {
                case 0x04:
                    if ((n + 1) > 15) {
                        abort(13);
                    }
                    lax = eip_linear + (n++);
                    modRM = ld8_readonly_cpl3();
                    if ((modRM & 7) == 5) {
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
            lax = eip_linear + (n++);
            modRM = ld8_readonly_cpl3();
            if (ipr & 0x0080) {
                switch (modRM >> 6) {
                case 0:
                    if ((modRM & 7) == 6) {
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
                switch ((modRM & 7) | ((modRM >> 3) & 0x18)) {
                case 0x04:
                    if ((n + 1) > 15) {
                        abort(13);
                    }
                    lax = eip_linear + (n++);
                    modRM = ld8_readonly_cpl3();
                    if ((modRM & 7) == 5) {
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
            lax = eip_linear + (n++);
            modRM = ld8_readonly_cpl3();
            if (ipr & 0x0080) {
                switch (modRM >> 6) {
                case 0:
                    if ((modRM & 7) == 6) {
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
                switch ((modRM & 7) | ((modRM >> 3) & 0x18)) {
                case 0x04:
                    if ((n + 1) > 15) {
                        abort(13);
                    }
                    lax = eip_linear + (n++);
                    modRM = ld8_readonly_cpl3();
                    if ((modRM & 7) == 5) {
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
            operation = (modRM >> 3) & 7;
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
            lax = eip_linear + (n++);
            modRM = ld8_readonly_cpl3();
            if (ipr & 0x0080) {
                switch (modRM >> 6) {
                case 0:
                    if ((modRM & 7) == 6) {
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
                switch ((modRM & 7) | ((modRM >> 3) & 0x18)) {
                case 0x04:
                    if ((n + 1) > 15) {
                        abort(13);
                    }
                    lax = eip_linear + (n++);
                    modRM = ld8_readonly_cpl3();
                    if ((modRM & 7) == 5) {
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
            operation = (modRM >> 3) & 7;
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
            lax = eip_linear + (n++);
            opcode = ld8_readonly_cpl3();
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
                lax = eip_linear + (n++);
                modRM = ld8_readonly_cpl3();
                if (ipr & 0x0080) {
                    switch (modRM >> 6) {
                    case 0:
                        if ((modRM & 7) == 6) {
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
                    switch ((modRM & 7) | ((modRM >> 3) & 0x18)) {
                    case 0x04:
                        if ((n + 1) > 15) {
                            abort(13);
                        }
                        lax = eip_linear + (n++);
                        modRM = ld8_readonly_cpl3();
                        if ((modRM & 7) == 5) {
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
                lax = eip_linear + (n++);
                modRM = ld8_readonly_cpl3();
                if (ipr & 0x0080) {
                    switch (modRM >> 6) {
                    case 0:
                        if ((modRM & 7) == 6) {
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
                    switch ((modRM & 7) | ((modRM >> 3) & 0x18)) {
                    case 0x04:
                        if ((n + 1) > 15) {
                            abort(13);
                        }
                        lax = eip_linear + (n++);
                        modRM = ld8_readonly_cpl3();
                        if ((modRM & 7) == 5) {
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
        }
    }
EXEC_LOOP:;
    return n;
}
void Free86::set_CR0(int bits) {
    // if changing flags 31, 16, or 0, must flush tlb
    if ((bits & ((1 << 31) | (1 << 16) | (1 << 0))) != (cr0 & ((1 << 31) | (1 << 16) | (1 << 0)))) {
        tlb_flush_all();
    }
    cr0 = bits | (1 << 4); // keep bit 4 set to 1 (80387 present)
}
void Free86::set_CR3(int bits) {
    cr3 = bits;
    if (cr0 & (1 << 31)) { // if in paging mode must reset tables
        tlb_flush_all();
    }
}
void Free86::set_CR4(int bits) {
    cr4 = bits;
}
bool Free86::is_real__v86() {
    return !is_protected();
}
bool Free86::is_protected() {
    return cr0 & (1 << 0);
}
bool Free86::is_paging() {
    return (cr0 & (1 << 31)) && is_protected();
}
void Free86::set_cpl(int level) {
    cpl = level;
    if (cpl == 3) {
        tlb_readonly = tlb_readonly_cpl3;
        tlb_writable = tlb_writable_cpl3;
    } else {
        tlb_readonly = tlb_readonly_cplX;
        tlb_writable = tlb_writable_cplX;
    }
}
int Free86::ld8_io(int port) {
    return io_read(port);
}
int Free86::ld16_io(int port) {
    return io_read(port);
}
int Free86::ld_io(int port) {
    return io_read(port);
}
void Free86::st8_io(int port, int byte) {
    io_write(port, byte);
}
void Free86::st16_io(int port, int word) {
    io_write(port, word);
}
void Free86::st_io(int port, int dword) {
    io_write(port, dword);
}
void Free86::set_lower_byte(int reg, int byte) {
    if (reg & 4) { // ESP, EBP, ESI, EDI: set AH, CH, DH, BH
        regs[reg & 3] = (regs[reg & 3] & -65281) | ((byte & 0xff) << 8);
    } else { // set AL, CL, DL, BL
        regs[reg & 3] = (regs[reg & 3] & -256) | (byte & 0xff);
    }
}
void Free86::set_lower_word(int reg, int word) {
    regs[reg] = (regs[reg] & -65536) | (word & 0xffff);
}
void Free86::page_translation(int writable, bool user) {
    page_translation(writable, user, lax);
}
void Free86::page_translation(int writable, bool user, int address) {
    int pde_address, pde, pte_address, pte, pxe, pte_RW = 0, pte_US = 1, ok, error_code;
    if (!is_paging()) {
        pte_RW = 1; // writable
        pte_US = 0; // supervisor
        tlb_update(address & -4096, address & -4096, pte_RW, pte_US);
    } else { // paging enabled
        pde_address = (cr3 & -4096) + ((address >> 20) & 0xffc);
        pde = ld_direct(pde_address);
        if (!(pde & 0x00000001)) { // page not present
            error_code = 0;
        } else {
            pte_address = (pde & -4096) + ((address >> 10) & 0xffc);
            pte = ld_direct(pte_address);
            if (!(pte & 0x00000001)) { // page not present
                error_code = 0;
            } else {
                pxe = pde & pte;
                if (user && !(pxe & 0x00000004)) { // user request and page supervisor
                    error_code = 0x01;
                // user request or WP set and W request for RO page
                } else if ((user || (cr0 & (1 << 16))) && writable && !(pxe & 0x00000002)) {
                    error_code = 0x01;
                } else {
                    if (!(pde & 0x00000020)) { // page not accessed
                        pde |= 0x00000020;
                        st_direct(pde_address, pde);
                    }
                    ok = writable && !(pte & 0x00000040); // WR request and page not dirty
                    if (ok || !(pte & 0x00000020)) { // previous or page not yet accessed
                        pte |= 0x00000020;     // set page accessed
                        if (ok) {
                            pte |= 0x00000040; // set page dirty
                        }
                        st_direct(pte_address, pte);
                    }
                    // page dirty and supervisor request and page not RO
                    if ((pte & 0x00000040) && (!user || (pxe & 0x00000002))) {
                        pte_RW = 1;
                    }
                    // page not supervisor
                    if (pxe & 0x00000004) {
                        pte_US = 1;
                    }
                    tlb_update(address & -4096, pte & -4096, pte_RW, pte_US);
                    return;
                }
            }
        }
        error_code |= writable << 1;
        if (user) {
            error_code |= 0x04;
        }
        cr2 = address;
        abort(14, error_code);
    }
}
void Free86::segment_translation() {
    int sreg, sreg_default; // no DS override prefix
    if (x86_64_long_mode && (ipr & (0x000f | 0x0080)) == 0) {
        switch ((modRM & 7) | ((modRM >> 3) & 0x18)) {
        case 0x04:
            sib = ld8_direct();
            base = sib & 7;
            if (base == 5) {
                lax = ld_direct();
            } else {
                lax = regs[base];
            }
            index = (sib >> 3) & 7;
            if (index != 4) {
                lax = lax + (regs[index] << (sib >> 6));
            }
            break;
        case 0x0c:
            sib = ld8_direct();
            lax = (ld8_direct() << 24) >> 24;
            base = sib & 7;
            lax = lax + regs[base];
            index = (sib >> 3) & 7;
            if (index != 4) {
                lax = lax + (regs[index] << (sib >> 6));
            }
            break;
        case 0x14:
            sib = ld8_direct();
            lax = ld_direct();
            base = sib & 7;
            lax = lax + regs[base];
            index = (sib >> 3) & 7;
            if (index != 4) {
                lax = lax + (regs[index] << (sib >> 6));
            }
            break;
        case 0x05:
            lax = ld_direct();
            break;
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x06:
        case 0x07:
            base = modRM & 7;
            lax = regs[base];
            break;
        case 0x08:
        case 0x09:
        case 0x0a:
        case 0x0b:
        case 0x0d:
        case 0x0e:
        case 0x0f:
            lax = (ld8_direct() << 24) >> 24;
            base = modRM & 7;
            lax = lax + regs[base];
            break;
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x15:
        case 0x16:
        case 0x17:
        default:
            lax = ld_direct();
            base = modRM & 7;
            lax = lax + regs[base];
            break;
        }
        return;
    } else if (ipr & 0x0080) {
        if ((modRM & 0xc7) == 0x06) {
            lax = ld16_direct();
            sreg_default = 3;
        } else {
            switch (modRM >> 6) {
            case 0:
                lax = 0;
                break;
            case 1:
                lax = (ld8_direct() << 24) >> 24;
                break;
            default:
                lax = ld16_direct();
                break;
            }
            switch (modRM & 7) {
            case 0:
                lax = (lax + regs[3] + regs[6]) & 0xffff;
                sreg_default = 3;
                break;
            case 1:
                lax = (lax + regs[3] + regs[7]) & 0xffff;
                sreg_default = 3;
                break;
            case 2:
                lax = (lax + regs[5] + regs[6]) & 0xffff;
                sreg_default = 2;
                break;
            case 3:
                lax = (lax + regs[5] + regs[7]) & 0xffff;
                sreg_default = 2;
                break;
            case 4:
                lax = (lax + regs[6]) & 0xffff;
                sreg_default = 3;
                break;
            case 5:
                lax = (lax + regs[7]) & 0xffff;
                sreg_default = 3;
                break;
            case 6:
                lax = (lax + regs[5]) & 0xffff;
                sreg_default = 2;
                break;
            case 7:
            default:
                lax = (lax + regs[3]) & 0xffff;
                sreg_default = 3;
                break;
            }
        }
        sreg = ipr & 0x000f;
        if (sreg == 0) {
            sreg = sreg_default;
        } else {
            sreg--;
        }
        lax = lax + segs[sreg].base;
        return;
    }
    switch ((modRM & 7) | ((modRM >> 3) & 0x18)) {
        case 0x04:
            sib = ld8_direct();
            base = sib & 7;
            if (base == 5) {
                lax = ld_direct();
                base = 0;
            } else {
                lax = regs[base];
            }
            index = (sib >> 3) & 7;
            if (index != 4) {
                lax = lax + (regs[index] << (sib >> 6));
            }
            break;
        case 0x0c:
            sib = ld8_direct();
            lax = (ld8_direct() << 24) >> 24;
            base = sib & 7;
            lax = lax + regs[base];
            index = (sib >> 3) & 7;
            if (index != 4) {
                lax = lax + (regs[index] << (sib >> 6));
            }
            break;
        case 0x14:
            sib = ld8_direct();
            lax = ld_direct();
            base = sib & 7;
            lax = lax + regs[base];
            index = (sib >> 3) & 7;
            if (index != 4) {
                lax = lax + (regs[index] << (sib >> 6));
            }
            break;
        case 0x05:
            lax = ld_direct();
            base = 0;
            break;
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x06:
        case 0x07:
            base = modRM & 7;
            lax = regs[base];
            break;
        case 0x08:
        case 0x09:
        case 0x0a:
        case 0x0b:
        case 0x0d:
        case 0x0e:
        case 0x0f: // 2-byte instruction escape
            lax = (ld8_direct() << 24) >> 24;
            base = modRM & 7;
            lax = lax + regs[base];
            break;
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x15:
        case 0x16:
        case 0x17:
        default:
            lax = ld_direct();
            base = modRM & 7;
            lax = lax + regs[base];
            break;
        }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
            if (base == 4 || base == 5) {
                sreg = 2;
            } else {
                sreg = 3;
            }
    } else {
            sreg--;
        }
    lax = lax + segs[sreg].base;
    return;
}
void Free86::offset_to_linear(bool writable) {
    uint64_t la;
    int sreg, stride, type_notok, limit_notok;
    if (ipr & 0x0080) {
        la = ld16_direct() & 0xffff;
        stride = 2; // 16 bit mode
    } else {
        la = ld_direct() & 0xffffffff;
        stride = 4; // 32 bit mode
    }
    if (!(opcode & 0x01)) {
        stride = 1; // byte mode, opcodes A0, A2
    }
    sreg = ipr & 0x000f;
    if (sreg == 0) {
        sreg = 3;
    } else {
        sreg--;
    }
    // type checking
    if (sreg == 1) { // CS
        type_notok = writable || !(segs[sreg].flags & (1 << 9));
    } else { // data segment
        type_notok = writable && !(segs[sreg].flags & (1 << 9));
    }
    if (type_notok) {
        abort(13, 0);
    }
    la = segs[sreg].base + la;
    // limit checking
    if (segs[sreg].flags & (1 << 10)) { // expand-down segment
        limit_notok = la < (uint64_t) segs[sreg].base + segs[sreg].limit + 1;
    } else {
        limit_notok = la > (uint64_t) segs[sreg].base + segs[sreg].limit - (stride - 1);
    }
    if (limit_notok) {
        if (sreg == 2) {
            abort(12, 0); // #SS(0)
        } else {
            abort(13, 0); // #GP(0)
        }
    }
    lax = la;
}
void Free86::set_segment_register(int sreg, int selector, uint32_t base, uint32_t limit, int flags) {
    segs[sreg] = {selector, base, limit, flags}; // set register
    update_SSB(); // emulator state variables
}
void Free86::set_segment_register(int sreg, int selector) {
    int s;
    s = selector & 0xffff;
    if (is_protected()) {
        set_segment_register_protected(sreg, s);
    } else { // real or v86 mode
        set_segment_register_real__v86(sreg, s);
    }
}
void Free86::set_segment_register_real__v86(int sreg, int selector) {
    if (eflags & 0x00020000) { // v86 mode
        set_segment_register(sreg, selector, (selector << 4), 0xffff, (1 << 15) | (3 << 13) | (1 << 12) | (1 << 9) | (1 << 8));
    } else { // real mode
        segs[sreg].selector = selector;
        segs[sreg].base = selector << 4;
        segs[sreg].limit = 0xffff;
    }
}
void Free86::set_segment_register_protected(int sreg, int selector) {
    SegmentRegister xdt;
    int dte_lower_dword, dte_upper_dword, selector_index;
    if ((selector & 0xfffc) == 0) { // null selector
        if (sreg == 2) {
            abort(13, 0);
        }
        set_segment_register(sreg, selector, 0, 0, 0);
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
        lax = xdt.base + selector_index;
        dte_lower_dword = ld_readonly_cplX();
        lax += 4;
        dte_upper_dword = ld_readonly_cplX();
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
            dte_upper_dword |= 1 << 8;
            st_writable_cplX(dte_upper_dword);
        }
        set_segment_register(sreg, selector, compile_dte_base(dte_lower_dword, dte_upper_dword), compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
    }
}
int Free86::is_segment_accessible(int selector, bool writable) {
    int descriptor_table_entry[2], dte_lower_dword, dte_upper_dword;
    if ((selector & 0xfffc) == 0) {
        return 1;
    }
    fill_xdt_descriptor(descriptor_table_entry, selector);
    dte_lower_dword = descriptor_table_entry[0];
    dte_upper_dword = descriptor_table_entry[1];
    if (dte_lower_dword == 0 && dte_upper_dword == 0) {
        return 1;
    }
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
void Free86::fill_xdt_descriptor(int *descriptor_table_entry, int selector) {
    SegmentRegister xdt;
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
    lax = xdt.base + index;
    dte_lower_dword = ld_readonly_cplX();
    lax += 4;
    dte_upper_dword = ld_readonly_cplX();
    descriptor_table_entry[0] = dte_lower_dword;
    descriptor_table_entry[1] = dte_upper_dword;
}
void Free86::fill_tss_interlevel(int *descriptor_table_entry, int privilege_level) {
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
    lax = tr.base + offset;
    if (is_386 == 0) {
        dte_upper_dword = ld16_readonly_cplX(); // privileged SP
        lax += 2;
    } else {
        dte_upper_dword = ld_readonly_cplX(); // privileged ESP
        lax += 4;
    }
    dte_lower_dword = ld16_readonly_cplX(); // privileged SS
    descriptor_table_entry[0] = dte_lower_dword;
    descriptor_table_entry[1] = dte_upper_dword;
}
int Free86::compile_dte_base(int dte_lower_dword, int dte_upper_dword) {
    return (((dte_lower_dword >> 16) & 0xffff) | ((dte_upper_dword & 0xff) << 16) | (dte_upper_dword & 0xff000000));
}
int Free86::compile_dte_limit(int dte_lower_dword, int dte_upper_dword) {
    int limit = (dte_lower_dword & 0xffff) | (dte_upper_dword & 0x000f0000);
    if (dte_upper_dword & (1 << 23)) {
        limit = (limit << 12) | 0xfff;
    }
    return limit;
}
void Free86::fill_segment_register(SegmentRegister *segment_register, int dte_lower_dword, int dte_upper_dword) {
    segment_register->base = compile_dte_base(dte_lower_dword, dte_upper_dword);
    segment_register->limit = compile_dte_limit(dte_lower_dword, dte_upper_dword);
    segment_register->flags = dte_upper_dword;
}
int Free86::get_addressmask(int dte_upper_dword) {
    if (dte_upper_dword & (1 << 22)) {
        return -1;
    } else {
        return 0xffff;
    }
}
int Free86::aux_INC8(int data) {
    if (osm < 25) {
        ocm_preserved = osm;
        ocm_dst_preserved = osm_dst;
    }
    osm_dst = ((data + 1) << 24) >> 24;
    osm = 25;
    return osm_dst;
}
int Free86::aux_INC16(int data) {
    if (osm < 25) {
        ocm_preserved = osm;
        ocm_dst_preserved = osm_dst;
    }
    osm_dst = ((data + 1) << 16) >> 16;
    osm = 26;
    return osm_dst;
}
int Free86::aux_DEC8(int data) {
    if (osm < 25) {
        ocm_preserved = osm;
        ocm_dst_preserved = osm_dst;
    }
    osm_dst = ((data - 1) << 24) >> 24;
    osm = 28;
    return osm_dst;
}
int Free86::aux_DEC16(int data) {
    if (osm < 25) {
        ocm_preserved = osm;
        ocm_dst_preserved = osm_dst;
    }
    osm_dst = ((data - 1) << 16) >> 16;
    osm = 29;
    return osm_dst;
}
int Free86::aux_SHRD16_SHLD16(int dst, int src, int count) {
    int c, s, r;
    r = dst;
    c = count & 0x1f;
    if (c) {
        if (operation == 0) { // SHLD
            s = src & 0xffff;
            x = s | (r << 16);
            osm_src = x >> (32 - c);
            x = x << c;
            if (c > 16) {
                x |= s << (c - 16);
            }
            osm_dst = r = x >> 16;
            osm = 19;
        } else { // SHRD
            x = (r & 0xffff) | (src << 16);
            osm_src = x >> (c - 1);
            x = x >> c;
            if (c > 16) {
                x |= src << (32 - c);
            }
            osm_dst = r = (x << 16) >> 16;
            osm = 19;
        }
    }
    return r;
}
int Free86::aux_SHRD(int dst, int src, int count) {
    int r, c;
    r = dst;
    c = count & 0x1f;
    if (c) {
        osm_src = r >> (c - 1);
        uint32_t lval = (uint32_t) r >> c;
        uint32_t rval = (uint32_t) src << (32 - c);
        osm_dst = r = lval | rval;
        osm = 20;
    }
    return r;
}
int Free86::aux_SHLD(int dst, int src, int count) {
    int c, r;
    r = dst;
    c = count & 0x1f;
    if (c) {
        osm_src = r << (c - 1);
        uint32_t lval = r << c;
        uint32_t rval = (uint32_t) src >> (32 - c);
        osm_dst = r = lval | rval;
        osm = 17;
    }
    return r;
}
void Free86::aux_BT16(int base, int offset) {
    osm_src = base >> (offset & 0xf);
    osm = 19;
}
void Free86::aux_BT(int base, int offset) {
    osm_src = base >> (offset & 0x1f);
    osm = 20;
}
int Free86::aux_BTS16_BTR16_BTC16(int base, int offset) {
    int o, r;
    o = offset & 0xf;
    osm_src = base >> o;
    x = 1 << o;
    switch (operation) {
    case 1: // BTS
        r = base | x;
        break;
    case 2: // BTR
        r = base & ~x;
        break;
    case 3: // BTC
    default:
        r = base ^ x;
        break;
    }
    osm = 19;
    return r;
}
int Free86::aux_BTS_BTR_BTC(int base, int offset) {
    int o, r;
    o = offset & 0x1f;
    osm_src = base >> o;
    x = 1 << o;
    switch (operation) {
    case 1: // BTS
        r = base | x;
        break;
    case 2: // BTR
        r = base & ~x;
        break;
    case 3: // BTC
    default:
        r = base ^ x;
        break;
    }
    osm = 20;
    return r;
}
int Free86::aux_BSF16(int dst, int src) {
    int s, r;
    r = dst;
    s = src & 0xffff;
    if (s) {
        r = 0;
        while ((s & 1) == 0) {
            r++;
            s >>= 1;
        }
        osm_dst = 1;
    } else {
        osm_dst = 0;
    }
    osm = 14;
    return r;
}
int Free86::aux_BSF(int dst, int src) {
    int s, r;
    r = dst;
    s = src;
    if (s) {
        r = 0;
        while ((s & 1) == 0) {
            r++;
            s >>= 1;
        }
        osm_dst = 1;
    } else {
        osm_dst = 0;
    }
    osm = 14;
    return r;
}
int Free86::aux_BSR16(int dst, int src) {
    int s, r;
    r = dst;
    s = src & 0xffff;
    if (s) {
        r = 15;
        while ((s & 0x8000) == 0) {
            r--;
            s <<= 1;
        }
        osm_dst = 1;
    } else {
        osm_dst = 0;
    }
    osm = 14;
    return r;
}
int Free86::aux_BSR(int dst, int src) {
    int s, r;
    r = dst;
    s = src;
    if (s) {
        r = 31;
        while (s >= 0) {
            r--;
            s <<= 1;
        }
        osm_dst = 1;
    } else {
        osm_dst = 0;
    }
    osm = 14;
    return r;
}
void Free86::aux_DIV8(int divisor) {
    int d, a, q, r;
    d = divisor & 0xff;
    a = regs[0] & 0xffff;
    if ((a >> 8) >= d) {
        abort(0);
    }
    q = a / d;
    r = a % d;
    set_lower_word(0, (q & 0xff) | (r << 8));
}
void Free86::aux_DIV16(int divisor) {
    int d, a, q, r;
    d = divisor & 0xffff;
    a = (regs[2] << 16) | (regs[0] & 0xffff);
    uint32_t au = a;
    if ((au >> 16) >= d) {
        abort(0);
    }
    q = au / d;
    r = au % d;
    set_lower_word(0, q);
    set_lower_word(2, r);
}
void Free86::aux_DIV(uint32_t dividend_upper, uint32_t dividend_lower, uint32_t divisor) {
    uint64_t a;
    uint32_t dd_upper, dd_lower;
    int dd_sign;
    dd_upper = dividend_upper;
    dd_lower = dividend_lower;
    if (dd_upper >= divisor) {
        abort(0);
    }
    if (dd_upper <= 0x200000) {
        a = dd_upper * 4294967296 + dd_lower;
        y = a % divisor;
        x = a / divisor;
    } else {
        for (int i = 0; i < 32; i++) {
            dd_sign = dd_upper >> 31;
            dd_upper = (dd_upper << 1) | (dd_lower >> 31);
            if (dd_sign || (dd_upper >= divisor)) {
                dd_upper = dd_upper - divisor;
                dd_lower = (dd_lower << 1) | 1;
            } else {
                dd_lower = dd_lower << 1;
            }
        }
        y = dd_upper;
        x = dd_lower;
    }
}
void Free86::aux_IDIV8(int divisor) {
    int d, a, q, r;
    d = (divisor << 24) >> 24;
    a = (regs[0] << 16) >> 16;
    if (d == 0) {
        abort(0);
    }
    q = a / d;
    if (((q << 24) >> 24) != q) {
        abort(0);
    }
    r = a % d;
    set_lower_word(0, (q & 0xff) | (r << 8));
}
void Free86::aux_IDIV16(int divisor) {
    int d, a, q, r;
    d = (divisor << 16) >> 16;
    a = (regs[2] << 16) | (regs[0] & 0xffff);
    if (d == 0) {
        abort(0);
    }
    q = a / d;
    if (((q << 16) >> 16) != q) {
        abort(0);
    }
    r = a % d;
    set_lower_word(0, q);
    set_lower_word(2, r);
}
void Free86::aux_IDIV(int dividend_upper, int dividend_lower, int divisor) {
    int dd_upper, dd_lower, dd_sign, dr_sign;
    dd_upper = dividend_upper;
    dd_lower = dividend_lower;
    if (dd_upper < 0) {
        dd_sign = 1;
        dd_upper = ~dd_upper;
        dd_lower = -dd_lower;
        if (dd_lower == 0) {
            dd_upper = dd_upper + 1;
        }
    } else {
        dd_sign = 0;
    }
    if (divisor < 0) {
        divisor = -divisor;
        dr_sign = 1;
    } else {
        dr_sign = 0;
    }
    aux_DIV(dd_upper, dd_lower, divisor);
    dr_sign ^= dd_sign;
    if (dr_sign) {
        if (x > 0x80000000) {
            abort(0);
        }
        x = -x;
    } else {
        if (x >= 0x80000000) {
            abort(0);
        }
    }
    if (dd_sign) {
        y = -y;
    }
}
void Free86::aux_MUL8(int multiplicand, int multiplier) {
    int md, mr;
    md = multiplicand & 0xff;
    mr = multiplier & 0xff;
    x = (md & 0xff) * (mr & 0xff);
    osm_src = x >> 8;
    osm_dst = (x << 24) >> 24;
    osm = 21;
}
void Free86::aux_MUL16(int multiplicand, int multiplier) {
    x = (multiplicand & 0xffff) * (multiplier & 0xffff);
    osm_src = x >> 16;
    osm_dst = (x << 16) >> 16;
    osm = 22;
}
void Free86::aux_MUL(int multiplicand, int multiplier) {
    multiply(multiplicand, multiplier);
    osm_dst = x;
    osm_src = y;
    osm = 23;
}
void Free86::aux_IMUL8(int multiplicand, int multiplier) {
    int md, mr;
    md = (multiplicand << 24) >> 24;
    mr = (multiplier << 24) >> 24;
    x = md * mr;
    osm_dst = (x << 24) >> 24;
    osm_src = x != osm_dst;
    osm = 21;
}
void Free86::aux_IMUL16(int multiplicand, int multiplier) {
    int md, mr;
    md = (multiplicand << 16) >> 16;
    mr = (multiplier << 16) >> 16;
    x = md * mr;
    osm_dst = (x << 16) >> 16;
    osm_src = x != osm_dst;
    osm = 22;
}
void Free86::aux_IMUL(int multiplicand, int multiplier) {
    int md, mr, s;
    md = multiplicand;
    mr = multiplier;
    s = 0;
    if (md < 0) {
        md = -md;
        s = 1;
    }
    if (mr < 0) {
        mr = -mr;
        s ^= 1;
    }
    multiply(md, mr);
    if (s) {
        y = ~y;
        x = -x;
        if (x == 0) {
            y = y + 1;
        }
    }
    osm_dst = x;
    osm_src = y - (x >> 31);
    osm = 23;
}
void Free86::multiply(int multiplicand, int multiplier) {
    uint32_t md_lower, md_upper, mr_lower, mr_upper, z;
    uint64_t x = (uint64_t) multiplicand * (uint32_t) multiplier;
    if (x <= 0xffffffff) {
        y = 0;
    } else {
        md_lower = multiplicand & 0xffff;
        md_upper = (uint32_t) multiplicand >> 16;
        mr_lower = multiplier & 0xffff;
        mr_upper = (uint32_t) multiplier >> 16;
        x = md_lower * mr_lower;
        y = md_upper * mr_upper;
        z = md_lower * mr_upper;
        x += (z & 0xffff) << 16;
        y += z >> 16;
        if (x >= 4294967296) {
            x -= 4294967296;
            y++;
        }
        z = md_upper * mr_lower;
        x += (z & 0xffff) << 16;
        y += z >> 16;
        if (x >= 4294967296) {
            x -= 4294967296;
            y++;
        }
    }
    this->x = x;
}
int Free86::calculate8(int dst, int src) {
    int r, cf;
    r = dst;
    switch (operation & 7) {
    case 0:
        osm_src = src;
        r = ((r + src) << 24) >> 24;
        osm_dst = r;
        osm = 0;
        break;
    case 1:
        r = (((r | src) << 24) >> 24);
        osm_dst = r;
        osm = 12;
        break;
    case 2:
        cf = is_CF();
        osm_src = src;
        r = ((r + src + cf) << 24) >> 24;
        osm_dst = r;
        osm = cf ? 3 : 0;
        break;
    case 3:
        cf = is_CF();
        osm_src = src;
        r = ((r - src - cf) << 24) >> 24;
        osm_dst = r;
        osm = cf ? 9 : 6;
        break;
    case 4:
        r = ((r & src) << 24) >> 24;
        osm_dst = r;
        osm = 12;
        break;
    case 5:
        osm_src = src;
        r = ((r - src) << 24) >> 24;
        osm_dst = r;
        osm = 6;
        break;
    case 6:
        r = ((r ^ src) << 24) >> 24;
        osm_dst = r;
        osm = 12;
        break;
    case 7:
        osm_src = src;
        osm_dst = ((dst - src) << 24) >> 24;
        osm = 6;
        break;
    }
    return r;
}
int Free86::calculate16(int dst, int src) {
    int r, cf;
    r = dst;
    switch (operation & 7) {
    case 0:
        osm_src = src;
        r = ((r + src) << 16) >> 16;
        osm_dst = r;
        osm = 1;
        break;
    case 1:
        r = (((r | src) << 16) >> 16);
        osm_dst = r;
        osm = 13;
        break;
    case 2:
        cf = is_CF();
        osm_src = src;
        r = ((r + src + cf) << 16) >> 16;
        osm_dst = r;
        osm = cf ? 4 : 1;
        break;
    case 3:
        cf = is_CF();
        osm_src = src;
        r = ((r - src - cf) << 16) >> 16;
        osm_dst = r;
        osm = cf ? 10 : 7;
        break;
    case 4:
        r = ((r & src) << 16) >> 16;
        osm_dst = r;
        osm = 13;
        break;
    case 5:
        osm_src = src;
        r = ((r - src) << 16) >> 16;
        osm_dst = r;
        osm = 7;
        break;
    case 6:
        r = ((r ^ src) << 16) >> 16;
        osm_dst = r;
        osm = 13;
        break;
    case 7:
        osm_src = src;
        osm_dst = ((r - src) << 16) >> 16;
        osm = 7;
        break;
    }
    return r;
}
int Free86::calculate(int dst, int src) {
    int r, cf;
    r = dst;
    switch (operation & 7) {
    case 0:
        osm_src = src;
        r = r + src;
        osm_dst = r;
        osm = 2;
        break;
    case 1:
        r = r | src;
        osm_dst = r;
        osm = 14;
        break;
    case 2:
        cf = is_CF();
        osm_src = src;
        r = r + src + cf;
        osm_dst = r;
        osm = cf ? 5 : 2;
        break;
    case 3:
        cf = is_CF();
        osm_src = src;
        r = r - src - cf;
        osm_dst = r;
        osm = cf ? 11 : 8;
        break;
    case 4:
        r = r & src;
        osm_dst = r;
        osm = 14;
        break;
    case 5:
        osm_src = src;
        r = r - src;
        osm_dst = r;
        osm = 8;
        break;
    case 6:
        r = r ^ src;
        osm_dst = r;
        osm = 14;
        break;
    case 7:
        osm_src = src;
        osm_dst = r - src;
        osm = 8;
        break;
    }
    return r;
}
int Free86::shift8(int src, int count) {
    int c, cf, s, r;
    s = r = src & 0xff;
    switch (operation & 7) {
    case 0:
        if (count & 0x1f) {
            c = count & 0x7;
            r = (r << c) | (r >> (8 - c));
            osm_src = compile_eflags(true);
            osm_src |= r & 0x0001;
            if (c == 1) {
                osm_src |= ((s ^ r) << 4) & 0x0800;
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 1:
        if (count & 0x1f) {
            c = count & 0x7;
            r = (r >> c) | (r << (8 - c));
            osm_src = compile_eflags(true);
            osm_src |= (r >> 7) & 0x0001;
            if (c == 1) {
                osm_src |= ((s ^ r) << 4) & 0x0800;
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 2:
        c = shift8_LUT[count & 0x1f];
        if (c) {
            cf = is_CF();
            r = (r << c) | (cf << (c - 1));
            if (c > 1) {
                r |= s >> (9 - c);
            }
            osm_src = compile_eflags(true);
            osm_src |= (s >> (8 - c)) & 0x0001;
            if (c == 1) {
                osm_src |= ((s ^ r) << 4) & 0x0800;
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 3:
        c = shift8_LUT[count & 0x1f];
        if (c) {
            cf = is_CF();
            r = (r >> c) | (cf << (8 - c));
            if (c > 1) {
                r |= s << (9 - c);
            }
            osm_src = compile_eflags(true);
            osm_src |= (s >> (c - 1)) & 0x0001;
            if (c == 1) {
                osm_src |= ((s ^ r) << 4) & 0x0800;
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 4:
    case 6:
        c = count & 0x1f;
        if (c) {
            osm_src = r << (c - 1);
            osm_dst = r = ((r << c) << 24) >> 24;
            osm = 15;
        }
        break;
    case 5:
        c = count & 0x1f;
        if (c) {
            osm_src = r >> (c - 1);
            osm_dst = r = ((r >> c) << 24) >> 24;
            osm = 18;
        }
        break;
    case 7:
        c = count & 0x1f;
        if (c) {
            r = (src << 24) >> 24;
            osm_src = r >> (c - 1);
            osm_dst = r = ((r >> c) << 24) >> 24;
            osm = 18;
        }
        break;
    }
    return r;
}
int Free86::shift16(int src, int count) {
    int c, cf, s, r;
    s = r = src & 0xffff;
    switch (operation & 7) {
    case 0:
        if (count & 0x1f) {
            c = count & 0xf;
            r = (r << c) | (r >> (16 - c));
            osm_src = compile_eflags(true);
            osm_src |= r & 0x0001;
            if (c == 1) {
                osm_src |= ((s ^ r) >> 4) & 0x0800;
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 1:
        if (count & 0x1f) {
            c = count & 0xf;
            r = (r >> c) | (r << (16 - c));
            osm_src = compile_eflags(true);
            osm_src |= (r >> 15) & 0x0001;
            if (c == 1) {
                osm_src |= ((s ^ r) >> 4) & 0x0800;
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 2:
        c = shift16_LUT[count & 0x1f];
        if (c) {
            cf = is_CF();
            r = (r << c) | (cf << (c - 1));
            if (c > 1) {
                r |= s >> (17 - c);
            }
            osm_src = compile_eflags(true);
            osm_src |= (s >> (16 - c)) & 0x0001;
            if (c == 1) {
                osm_src |= ((s ^ r) >> 4) & 0x0800;
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 3:
        c = shift16_LUT[count & 0x1f];
        if (c) {
            cf = is_CF();
            r = (r >> c) | (cf << (16 - c));
            if (c > 1) {
                r |= s << (17 - c);
            }
            osm_src = compile_eflags(true);
            osm_src |= (s >> (c - 1)) & 0x0001;
            if (c == 1) {
                osm_src |= ((s ^ r) >> 4) & 0x0800;
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 4:
    case 6:
        c = count & 0x1f;
        if (c) {
            osm_src = r << (c - 1);
            osm_dst = r = ((r << c) << 16) >> 16;
            osm = 16;
        }
        break;
    case 5:
        c = count & 0x1f;
        if (c) {
            osm_src = r >> (c - 1);
            osm_dst = r = ((r >> c) << 16) >> 16;
            osm = 19;
        }
        break;
    case 7:
        c = count & 0x1f;
        if (c) {
            r = (src << 16) >> 16;
            osm_src = r >> (c - 1);
            osm_dst = r = ((r >> c) << 16) >> 16;
            osm = 19;
        }
        break;
    }
    return r;
}
int Free86::shift(uint32_t src, int count) {
    uint32_t s, r;
    int c, cf;
    s = r = src;
    switch (operation & 7) {
    case 0:
        c = count & 0x1f;
        if (c) {
            r = (r << c) | (r >> (32 - c));
            osm_src = compile_eflags(true);
            osm_src |= r & 0x0001;
            if (c == 1) {
                osm_src |= ((s ^ r) >> 20) & 0x0800;
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 1:
        c = count & 0x1f;
        if (c) {
            r = (r >> c) | (r << (32 - c));
            osm_src = compile_eflags(true);
            osm_src |= (r >> 31) & 0x0001;
            if (c == 1) {
                osm_src |= ((s ^ r) >> 20) & 0x0800;
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 2:
        c = count & 0x1f;
        if (c) {
            cf = is_CF();
            r = (r << c) | (cf << (c - 1));
            if (c > 1) {
                r |= s >> (33 - c);
            }
            osm_src = compile_eflags(true);
            osm_src |= (s >> (32 - c)) & 0x0001;
            if (c == 1) {
                osm_src |= ((s ^ r) >> 20) & 0x0800;
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 3:
        c = count & 0x1f;
        if (c) {
            cf = is_CF();
            r = (r >> c) | (cf << (32 - c));
            if (c > 1) {
                r |= s << (33 - c);
            }
            osm_src = compile_eflags(true);
            osm_src |= (s >> (c - 1)) & 0x0001;
            if (c == 1) {
                osm_src |= ((s ^ r) >> 20) & 0x0800;
            }
            osm_dst = ((osm_src >> 6) & 1) ^ 1;
            osm = 24;
        }
        break;
    case 4:
    case 6:
        c = count & 0x1f;
        if (c) {
            osm_src = r << (c - 1);
            osm_dst = r = r << c;
            osm = 17;
        }
        break;
    case 5:
        c = count & 0x1f;
        if (c) {
            osm_src = r >> (c - 1);
            osm_dst = r = r >> c;
            osm = 20;
        }
        break;
    case 7:
        c = count & 0x1f;
        if (c) {
            osm_src = (int) r >> (c - 1);
            osm_dst = r = (int) r >> c;
            osm = 20;
        }
        break;
    }
    return r;
}
void Free86::aux_LDTR(int selector) {
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
        lax = gdt.base + index;
        dte_lower_dword = ld_readonly_cplX();
        lax += 4;
        dte_upper_dword = ld_readonly_cplX();
        if ((dte_upper_dword & (1 << 12)) || ((dte_upper_dword >> 8) & 0xf) != 2) {
            abort(13, selector & 0xfffc);
        }
        if (!(dte_upper_dword & (1 << 15))) {
            abort(11, selector & 0xfffc);
        }
        fill_segment_register(&ldt, dte_lower_dword, dte_upper_dword);
    }
    ldt.selector = selector;
}
void Free86::aux_LTR(int selector) {
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
        lax = gdt.base + index;
        dte_lower_dword = ld_readonly_cplX();
        lax += 4;
        dte_upper_dword = ld_readonly_cplX();
        descriptor_type = (dte_upper_dword >> 8) & 0xf;
        if ((dte_upper_dword & (1 << 12)) || (descriptor_type != 1 && descriptor_type != 9)) {
            abort(13, selector & 0xfffc);
        }
        if (!(dte_upper_dword & (1 << 15))) {
            abort(11, selector & 0xfffc);
        }
        fill_segment_register(&tr, dte_lower_dword, dte_upper_dword);
        dte_upper_dword |= 1 << 9;
        st_writable_cplX(dte_upper_dword);
    }
    tr.selector = selector;
}
void Free86::aux_JMPF(int selector, int address) {
    if (!is_protected() || (eflags & 0x00020000)) {
        aux_JMPF_real__v86_mode(selector, address);
    } else {
        aux_JMPF_protected_mode(selector, address);
    }
}
void Free86::aux_JMPF_real__v86_mode(int selector, int address) {
    eip = address, far = far_start = 0;
    segs[1].selector = selector;
    segs[1].base = selector << 4;
    update_SSB();
}
void Free86::aux_JMPF_protected_mode(int selector, int address) {
    int descriptor_table_entry[2], dte_lower_dword, dte_upper_dword;
    uint32_t limit;
    if ((selector & 0xfffc) == 0) {
        abort(13, 0);
    }
    fill_xdt_descriptor(descriptor_table_entry, selector);
    dte_lower_dword = descriptor_table_entry[0];
    dte_upper_dword = descriptor_table_entry[1];
    if (dte_lower_dword == 0 && dte_upper_dword == 0) {
        abort(13, selector & 0xfffc);
    }
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
        set_segment_register(1, (selector & 0xfffc) | cpl, compile_dte_base(dte_lower_dword, dte_upper_dword), limit,  dte_upper_dword);
        eip = address, far = far_start = 0;
    } else {
        throw "fatal: unsupported TSS or task gate in JMP";
    }
}
void Free86::aux_CALLF(bool is_operand_size32, int selector, int address, int return_address) {
    if (!is_protected() || (eflags & 0x00020000)) {
        aux_CALLF_real__v86_mode(is_operand_size32, selector, address, return_address);
    } else {
        aux_CALLF_protected_mode(is_operand_size32, selector, address, return_address);
    }
}
void Free86::aux_CALLF_real__v86_mode(bool is_operand_size32, int selector, int address, int return_address) {
    int esp = regs[4];
    if (is_operand_size32) {
        esp = esp - 4;
        lax = (esp & SS_mask) + SS_base;
        st_writable_cpl3(segs[1].selector);
        esp = esp - 4;
        lax = (esp & SS_mask) + SS_base;
        st_writable_cpl3(return_address);
    } else {
        esp = esp - 2;
        lax = (esp & SS_mask) + SS_base;
        st16_writable_cpl3(segs[1].selector);
        esp = esp - 2;
        lax = (esp & SS_mask) + SS_base;
        st16_writable_cpl3(return_address);
    }
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
    eip = address, far = far_start = 0;
    segs[1].selector = selector;
    segs[1].base = selector << 4;
    update_SSB();
}
void Free86::aux_CALLF_protected_mode(bool is_operand_size32, int selector, int address, int return_address) {
    int descriptor_table_entry[2], dte_lower_dword, dte_upper_dword, descriptor_type;
    int offset, count;
    int ss, esp, start_esp, spl;
    // int Ue, Ve;
    if ((selector & 0xfffc) == 0) {
        abort(13, 0);
    }
    fill_xdt_descriptor(descriptor_table_entry, selector);
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
        SS_mask = get_addressmask(segs[2].flags);
        SS_base = segs[2].base;
        if (is_operand_size32) {
            esp = esp - 4;
            lax = SS_base + (esp & SS_mask);
            st_writable_cplX(segs[1].selector);
            esp = esp - 4;
            lax = SS_base + (esp & SS_mask);
            st_writable_cplX(return_address);
        } else {
            esp = esp - 2;
            lax = SS_base + (esp & SS_mask);
            st16_writable_cplX(segs[1].selector);
            esp = esp - 2;
            lax = SS_base + (esp & SS_mask);
            st16_writable_cplX(return_address);
        }
        int limit = compile_dte_limit(dte_lower_dword, dte_upper_dword);
        if (address > limit) {
            abort(13, selector & 0xfffc);
        }
        regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
        set_segment_register(1, (selector & 0xfffc) | cpl, compile_dte_base(dte_lower_dword, dte_upper_dword), limit, dte_upper_dword);
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
        default:
            abort(13, selector & 0xfffc);
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
        fill_xdt_descriptor(descriptor_table_entry, selector);
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
            fill_tss_interlevel(descriptor_table_entry, dpl);
            ss = descriptor_table_entry[0];
            esp = descriptor_table_entry[1];
            if ((ss & 0xfffc) == 0) {
                abort(10, ss & 0xfffc);
            }
            if ((ss & 3) != dpl) {
                abort(10, ss & 0xfffc);
            }
            fill_xdt_descriptor(descriptor_table_entry, ss);
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
            // Ue = get_addressmask(segs[2].flags);
            // Ve = segs[2].base;
            SS_mask = get_addressmask(dte_upper_dword);
            SS_base = compile_dte_base(dte_lower_dword, dte_upper_dword);
            if (is_operand_size32) {
                esp = esp - 4;
                lax = SS_base + (esp & SS_mask);
                st_writable_cplX(segs[2].selector);
                esp = esp - 4;
                lax = SS_base + (esp & SS_mask);
                st_writable_cplX(start_esp);
                for (int i = count - 1; i >= 0; i--) {
                    // x = Xe(Ve + ((start_esp + i * 4) & Ue));
                    esp = esp - 4;
                    lax = SS_base + (esp & SS_mask);
                    st_writable_cplX(0);
                    // st_writable_cplX(x);
                }
            } else {
                esp = esp - 2;
                lax = SS_base + (esp & SS_mask);
                st16_writable_cplX(segs[2].selector);
                esp = esp - 2;
                lax = SS_base + (esp & SS_mask);
                st16_writable_cplX(start_esp);
                for (int i = count - 1; i >= 0; i--) {
                    // x = Ye(Ve + ((start_esp + i * 2) & Ue));
                    esp = esp - 2;
                    lax = SS_base + (esp & SS_mask);
                    st16_writable_cplX(0);
                    // st16_writable_cplX(x);
                }
            }
            ss = (ss & ~3) | dpl;
            set_segment_register(2, ss, SS_base, compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
        } else {
            esp = start_esp;
            SS_mask = get_addressmask(segs[2].flags);
            SS_base = segs[2].base;
        }
        if (is_operand_size32) {
            esp = esp - 4;
            lax = SS_base + (esp & SS_mask);
            st_writable_cplX(segs[1].selector);
            esp = esp - 4;
            lax = SS_base + (esp & SS_mask);
            st_writable_cplX(return_address);
        } else {
            esp = esp - 2;
            lax = SS_base + (esp & SS_mask);
            st16_writable_cplX(segs[1].selector);
            esp = esp - 2;
            lax = SS_base + (esp & SS_mask);
            st16_writable_cplX(return_address);
        }
        selector = (selector & ~3) | dpl;
        set_segment_register(1, selector, compile_dte_base(dte_lower_dword, dte_upper_dword), compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
        set_cpl(dpl);
        regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
        eip = offset, far = far_start = 0;
    }
}
void Free86::return_real__v86_mode(bool is_operand_size32, bool is_iret, int return_offset) {
    int cs, esp, stack_eip, stack_eflags;
    esp = regs[4];
    SS_base = segs[2].base;
    SS_mask = 0xffff;
    if (is_operand_size32 == 1) {
        lax = SS_base + (esp & SS_mask);
        stack_eip = ld_readonly_cplX();
        esp = esp + 4;
        lax = SS_base + (esp & SS_mask);
        cs = ld_readonly_cplX();
        esp = esp + 4;
        cs &= 0xffff;
        if (is_iret) {
            lax = SS_base + (esp & SS_mask);
            stack_eflags = ld_readonly_cplX();
            esp = esp + 4;
        }
    } else {
        lax = SS_base + (esp & SS_mask);
        stack_eip = ld16_readonly_cplX();
        esp = esp + 2;
        lax = SS_base + (esp & SS_mask);
        cs = ld16_readonly_cplX();
        esp = esp + 2;
        if (is_iret) {
            lax = SS_base + (esp & SS_mask);
            stack_eflags = ld16_readonly_cplX();
            esp = esp + 2;
        }
    }
    regs[4] = (regs[4] & ~SS_mask) | ((esp + return_offset) & SS_mask);
    segs[1].selector = cs;
    segs[1].base = cs << 4;
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
void Free86::return_protected_mode(bool is_operand_size32, bool is_iret, int return_offset) {
    int esp, stack_esp, stack_eip, stack_eflags = 0;
    int descriptor_table_entry[2], dte_lower_dword, dte_upper_dword;
    int cpl = this->cpl, es, cs, ss, ds, fs, gs;
    esp = regs[4];
    SS_base = segs[2].base;
    SS_mask = get_addressmask(segs[2].flags);
    if (is_operand_size32 == 1) {
        lax = SS_base + (esp & SS_mask);
        stack_eip = ld_readonly_cplX();
        esp = esp + 4;
        lax = SS_base + (esp & SS_mask);
        cs = ld_readonly_cplX();
        esp = esp + 4;
        cs &= 0xffff;
        if (is_iret) {
            lax = SS_base + (esp & SS_mask);
            stack_eflags = ld_readonly_cplX();
            esp = esp + 4;
            if (stack_eflags & 0x00020000) {
                lax = SS_base + (esp & SS_mask);
                stack_esp = ld_readonly_cplX();
                esp = esp + 4;
                // pop segment selectors from stack
                lax = SS_base + (esp & SS_mask);
                ss = ld_readonly_cplX();
                esp = esp + 4;
                lax = SS_base + (esp & SS_mask);
                es = ld_readonly_cplX();
                esp = esp + 4;
                lax = SS_base + (esp & SS_mask);
                ds = ld_readonly_cplX();
                esp = esp + 4;
                lax = SS_base + (esp & SS_mask);
                fs = ld_readonly_cplX();
                esp = esp + 4;
                lax = SS_base + (esp & SS_mask);
                gs = ld_readonly_cplX();
                esp = esp + 4;
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
                set_cpl(3);
                return;
            }
        }
    } else {
        lax = SS_base + (esp & SS_mask);
        stack_eip = ld16_readonly_cplX();
        esp = esp + 2;
        lax = SS_base + (esp & SS_mask);
        cs = ld16_readonly_cplX();
        esp = esp + 2;
        if (is_iret) {
            lax = SS_base + (esp & SS_mask);
            stack_eflags = ld16_readonly_cplX();
            esp = esp + 2;
        }
    }
    if ((cs & 0xfffc) == 0) {
        abort(13, cs & 0xfffc);
    }
    fill_xdt_descriptor(descriptor_table_entry, cs);
    dte_lower_dword = descriptor_table_entry[0];
    dte_upper_dword = descriptor_table_entry[1];
    if (dte_lower_dword == 0 && dte_upper_dword == 0) {
        abort(13, cs & 0xfffc);
    }
    if (!(dte_upper_dword & (1 << 12)) || !(dte_upper_dword & (1 << 11))) {
        abort(13, cs & 0xfffc);
    }
    rpl = cs & 3;
    if (rpl < cpl) {
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
    esp = esp + return_offset;
    if (rpl == cpl) {
        set_segment_register(1, cs, compile_dte_base(dte_lower_dword, dte_upper_dword), compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
    } else {
        if (is_operand_size32 == 1) {
            lax = SS_base + (esp & SS_mask);
            stack_esp = ld_readonly_cplX();
            esp = esp + 4;
            lax = SS_base + (esp & SS_mask);
            ss = ld_readonly_cplX();
            esp = esp + 4;
            ss &= 0xffff;
        } else {
            lax = SS_base + (esp & SS_mask);
            stack_esp = ld16_readonly_cplX();
            esp = esp + 2;
            lax = SS_base + (esp & SS_mask);
            ss = ld16_readonly_cplX();
            esp = esp + 2;
        }
        if ((ss & 0xfffc) == 0) {
            abort(13, 0);
        } else {
            int dte_lower_dword, dte_upper_dword;
            if ((ss & 3) != rpl) {
                abort(13, ss & 0xfffc);
            }
            fill_xdt_descriptor(descriptor_table_entry, ss);
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
            set_segment_register(2, ss, compile_dte_base(dte_lower_dword, dte_upper_dword), compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
        }
        zero_segment_register(0, rpl);
        set_segment_register(1, cs, compile_dte_base(dte_lower_dword, dte_upper_dword), compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
        zero_segment_register(3, rpl);
        zero_segment_register(4, rpl);
        zero_segment_register(5, rpl);
        esp = stack_esp + return_offset;
        SS_mask = get_addressmask(dte_upper_dword);
        set_cpl(rpl);
    }
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
    eip = stack_eip, far = far_start = 0;
    if (is_iret) {
        int mask = 0x00000100 | 0x00004000 | 0x00010000 | 0x00040000 | 0x00200000;
        if (cpl == 0) {
            mask |= 0x00003000;
        }
        iopl = (eflags >> 12) & 3;
        if (cpl <= iopl) {
            mask |= 0x00000200;
        }
        if (is_operand_size32 == 0) {
            mask &= 0xffff;
        }
        set_EFLAGS(stack_eflags, mask);
    }
}
void Free86::zero_segment_register(int sreg, int privilege_level) {
    int dte_upper_dword;
    if ((sreg == 4 || sreg == 5) && (segs[sreg].selector & 0xfffc) == 0) {
        return; // null selector in FS, GS
    }
    dte_upper_dword = segs[sreg].flags;
    dpl = (dte_upper_dword >> 13) & 3;
    if (!(dte_upper_dword & (1 << 11)) || !(dte_upper_dword & (1 << 10))) {
        if (dpl < privilege_level) {
            set_segment_register(sreg, 0, 0, 0, 0);
        }
    }
}
void Free86::aux_RETF(bool is_operand_size32, int return_offset) {
    if (!is_protected() || (eflags & 0x00020000)) {
        return_real__v86_mode(is_operand_size32, 0, return_offset);
    } else {
        return_protected_mode(is_operand_size32, 0, return_offset);
    }
}
void Free86::raise_interrupt(int id, int error_code, int is_hw, int is_sw, int return_address) {
    if (is_protected()) {
        raise_interrupt_protected_mode(id, error_code, is_hw, is_sw, return_address);
    } else {
        raise_interrupt_real__v86_mode(id, is_sw, return_address);
    }
}
void Free86::raise_interrupt_real__v86_mode(int id, int is_sw, int return_address) {
    int selector, offset, esp;
    if (id * 4 + 3 > idt.limit) {
        abort(13, id * 8 + 2);
    }
    lax = idt.base + (id << 2);
    offset = ld16_readonly_cplX();
    lax = lax + 2;
    selector = ld16_readonly_cplX();
    esp = regs[4];
    esp = esp - 2;
    lax = (esp & SS_mask) + SS_base;
    st16_writable_cpl3(get_EFLAGS());
    esp = esp - 2;
    lax = (esp & SS_mask) + SS_base;
    st16_writable_cpl3(segs[1].selector);
    esp = esp - 2;
    lax = (esp & SS_mask) + SS_base;
    st16_writable_cpl3(is_sw ? return_address : eip);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
    eip = offset, far = far_start = 0;
    segs[1].selector = selector;
    segs[1].base = selector << 4;
    eflags &= ~(0x00000100 | 0x00000200 | 0x00010000 | 0x00040000);
}
void Free86::raise_interrupt_protected_mode(int id, int error_code, int is_hw, int is_sw, int return_address) {
    int selector, offset, st_error_code, is_interlevel, is_386;
    int descriptor_table_entry[2], dte_lower_dword, dte_upper_dword, descriptor_type;
    int ss, esp, spl, ss_dte_upper_dword, ss_dte_lower_dword;
    st_error_code = 0;
    if (!is_sw && !is_hw) {
        switch (id) { // with error codes, Intel IA-32 SDM (latest), Vol. 3A, 7.3
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
    if (id * 8 + 7 > idt.limit) {
        abort(13, id * 8 + 2);
    }
    lax = idt.base + id * 8;
    dte_lower_dword = ld_readonly_cplX();
    lax += 4;
    dte_upper_dword = ld_readonly_cplX();
    descriptor_type = (dte_upper_dword >> 8) & 0x1f;
    switch (descriptor_type) {
    case 14: // 32 bit interrupt gate
    case 15: // 32 bit trap gate
        break;
    case 5: // 16/ 32 bit task gate
    case 6: // 16 bit interrupt gate
    case 7: // 16 bit trap gate
        throw "fatal: unsupported gate type";
    default:
        abort(13, id * 8 + 2);
    }
    dpl = (dte_upper_dword >> 13) & 3;
    if (is_sw && dpl < cpl) {
        abort(13, id * 8 + 2);
    }
    if (!(dte_upper_dword & (1 << 15))) {
        abort(11, id * 8 + 2);
    }
    selector = dte_lower_dword >> 16;
    offset = (dte_upper_dword & -65536) | (dte_lower_dword & 0x0000ffff);
    if ((selector & 0xfffc) == 0) {
        abort(13, 0);
    }
    fill_xdt_descriptor(descriptor_table_entry, selector);
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
        fill_tss_interlevel(descriptor_table_entry, dpl);
        ss = descriptor_table_entry[0];
        esp = descriptor_table_entry[1];
        if ((ss & 0xfffc) == 0) {
            abort(10, ss & 0xfffc);
        }
        if ((ss & 3) != dpl) {
            abort(10, ss & 0xfffc);
        }
        fill_xdt_descriptor(descriptor_table_entry, ss);
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
        SS_mask = get_addressmask(ss_dte_lower_dword);
        SS_base = compile_dte_base(ss_dte_upper_dword, ss_dte_lower_dword);
        is_interlevel = 1;
    } else if ((dte_upper_dword & (1 << 10)) || dpl == cpl) {
        if (eflags & 0x00020000) {
            abort(13, selector & 0xfffc);
        }
        dpl = cpl;
        SS_mask = get_addressmask(segs[2].flags);
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
                esp = esp - 4;
                lax = SS_base + (esp & SS_mask);
                st_writable_cplX(segs[5].selector);
                esp = esp - 4;
                lax = SS_base + (esp & SS_mask);
                st_writable_cplX(segs[4].selector);
                esp = esp - 4;
                lax = SS_base + (esp & SS_mask);
                st_writable_cplX(segs[3].selector);
                esp = esp - 4;
                lax = SS_base + (esp & SS_mask);
                st_writable_cplX(segs[0].selector);
            }
            esp = esp - 4;
            lax = SS_base + (esp & SS_mask);
            st_writable_cplX(segs[2].selector);
            esp = esp - 4;
            lax = SS_base + (esp & SS_mask);
            st_writable_cplX(regs[4]);
        }
        esp = esp - 4;
        lax = SS_base + (esp & SS_mask);
        st_writable_cplX(get_EFLAGS());
        esp = esp - 4;
        lax = SS_base + (esp & SS_mask);
        st_writable_cplX(segs[1].selector);
        esp = esp - 4;
        lax = SS_base + (esp & SS_mask);
        st_writable_cplX(is_sw ? return_address : eip);
        if (st_error_code) {
            esp = esp - 4;
            lax = SS_base + (esp & SS_mask);
            st_writable_cplX(error_code);
        }
    } else {
        if (is_interlevel) {
            if (eflags & 0x00020000) {
                esp = esp - 2;
                lax = SS_base + (esp & SS_mask);
                st16_writable_cplX(segs[5].selector);
                esp = esp - 2;
                lax = SS_base + (esp & SS_mask);
                st16_writable_cplX(segs[4].selector);
                esp = esp - 2;
                lax = SS_base + (esp & SS_mask);
                st16_writable_cplX(segs[3].selector);
                esp = esp - 2;
                lax = SS_base + (esp & SS_mask);
                st16_writable_cplX(segs[0].selector);
            }
            esp = esp - 2;
            lax = SS_base + (esp & SS_mask);
            st16_writable_cplX(segs[2].selector);
            esp = esp - 2;
            lax = SS_base + (esp & SS_mask);
            st16_writable_cplX(regs[4]);
        }
        esp = esp - 2;
        lax = SS_base + (esp & SS_mask);
        st16_writable_cplX(get_EFLAGS());
        esp = esp - 2;
        lax = SS_base + (esp & SS_mask);
        st16_writable_cplX(segs[1].selector);
        esp = esp - 2;
        lax = SS_base + (esp & SS_mask);
        st16_writable_cplX(is_sw ? return_address : eip);
        if (st_error_code) {
            esp = esp - 2;
            lax = SS_base + (esp & SS_mask);
            st16_writable_cplX(error_code);
        }
    }
    if (is_interlevel) {
        if (eflags & 0x00020000) {
            set_segment_register(0, 0, 0, 0, 0);
            set_segment_register(3, 0, 0, 0, 0);
            set_segment_register(4, 0, 0, 0, 0);
            set_segment_register(5, 0, 0, 0, 0);
        }
        ss = (ss & ~3) | dpl;
        set_segment_register(2, ss, SS_base, compile_dte_limit(ss_dte_upper_dword, ss_dte_lower_dword), ss_dte_lower_dword);
    }
    selector = (selector & ~3) | dpl;
    set_segment_register(1, selector, compile_dte_base(dte_lower_dword, dte_upper_dword), compile_dte_limit(dte_lower_dword, dte_upper_dword), dte_upper_dword);
    set_cpl(dpl);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
    eip = offset, far = far_start = 0;
    if ((descriptor_type & 1) == 0) {
        eflags &= ~0x00000200;
    }
    eflags &= ~(0x00000100 | 0x00004000 | 0x00010000 | 0x00020000);
}
void Free86::aux_IRET(bool is_operand_size32) {
    if (!is_protected() || (eflags & 0x00020000)) {
        if (eflags & 0x00020000) {
            iopl = (eflags >> 12) & 3;
            if (iopl != 3) {
                abort(13);
            }
        }
        return_real__v86_mode(is_operand_size32, 1, 0);
    } else {
        if (eflags & 0x00004000) {
            throw "fatal: unsupported EFLAGS.NT == 1 in IRET";
        } else {
            return_protected_mode(is_operand_size32, 1, 0);
        }
    }
}
void Free86::aux_LAR_LSL(bool is_operand_size32, bool is_lsl) {
    int selector;
    if (!is_protected() || (eflags & 0x00020000)) {
        abort(6);
    }
    modRM = ld8_direct();
    reg = (modRM >> 3) & 7;
    if ((modRM >> 6) == 3) {
        selector = regs[modRM & 7] & 0xffff;
    } else {
        segment_translation();
        selector = ld16_readonly_cpl3();
    }
    x = ld_descriptor_flags(selector, is_lsl);
    osm_src = compile_eflags();
    if (x == -1) {
        osm_src &= ~0x0040;
    } else {
        osm_src |= 0x0040;
        if (is_operand_size32) {
            regs[reg] = x;
        } else {
            set_lower_word(reg, x);
        }
    }
    osm_dst = ((osm_src >> 6) & 1) ^ 1;
    osm = 24;
}
int Free86::ld_descriptor_flags(int selector, bool is_lsl) {
    int descriptor_table_entry[2], dte_lower_dword, dte_upper_dword, descriptor_type;
    if ((selector & 0xfffc) == 0) {
        return -1;
    }
    fill_xdt_descriptor(descriptor_table_entry, selector);
    dte_lower_dword = descriptor_table_entry[0];
    dte_upper_dword = descriptor_table_entry[1];
    if (dte_lower_dword == 0 && dte_upper_dword == 0) {
        return -1;
    }
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
        case 1: // data, RO, accessed
        case 2: // data, RW
        case 3: // data, RW, accessed
        case 9: // code, X, accessed
        case 11: // code, RX, accessed
            break;
        case 4: // data, RO, expand-down
        case 5: // data, RO, expand-down, accessed
        case 12: // code, X, conforming
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
void Free86::aux_VERR_VERW(int selector, bool writable) {
    int ok;
    ok = is_segment_accessible(selector, writable);
    osm_src = compile_eflags();
    if (!ok) {
        osm_src |= 0x0040;
    } else {
        osm_src &= ~0x0040;
    }
    osm_dst = ((osm_src >> 6) & 1) ^ 1;
    osm = 24;
}
void Free86::aux_ARPL() {
    if (!is_protected() || (eflags & 0x00020000)) {
        abort(6);
    }
    modRM = ld8_direct();
    if ((modRM >> 6) == 3) {
        rM = modRM & 7;
        rm = regs[rM] & 0xffff;
    } else {
        segment_translation();
        rm = ld16_writable_cpl3();
    }
    r = regs[(modRM >> 3) & 7];
    osm_src = compile_eflags();
    if ((rm & 3) < (r & 3)) {
        x = (rm & ~3) | (r & 3);
        if ((modRM >> 6) == 3) {
            set_lower_word(rM, x);
        } else {
            st16_writable_cpl3(x);
        }
        osm_src |= 0x0040;
    } else {
        osm_src &= ~0x0040;
    }
    osm_dst = ((osm_src >> 6) & 1) ^ 1;
    osm = 24;
}
void Free86::aux_CPUID() {
    switch (regs[0]) {
    case 0: // vendor ID
        regs[0] = 1;
        regs[3] = 0x756e6547; // "uneG"
        regs[2] = 0x49656e69; // "Ieni"
        regs[1] = 0x6c65746e; // "letn"
        break;
    case 1:  // processor info and feature flags
    default: // https://datasheets.chipdb.org/Intel/x86/CPUID/24161821.pdf
        regs[0] = (5 << 8) | (4 << 4) | 3; // type | family | model | stepping
        regs[3] = 8 << 8;                  //   00     0101    0100       0011
        regs[1] = 0;
        regs[2] = 1 << 4;
        break;
    }
}
void Free86::aux_AAM(int radix) {
    int al, ah;
    if (radix == 0) {
        abort(0);
    }
    al = regs[0] & 0xff;
    ah = al / radix;
    al = al % radix;
    regs[0] = (regs[0] & ~0xffff) | al | (ah << 8);
    osm_dst = (al << 24) >> 24;
    osm = 12;
}
void Free86::aux_AAD(int radix) {
    int al, ah;
    al = regs[0] & 0xff;
    ah = (regs[0] >> 8) & 0xff;
    al = (ah * radix + al) & 0xff;
    regs[0] = (regs[0] & ~0xffff) | al;
    osm_dst = (al << 24) >> 24;
    osm = 12;
}
void Free86::aux_AAA() {
    int of, al, ah, f4, flags;
    flags = compile_eflags();
    f4 = flags & 0x0010;
    al = regs[0] & 0xff;
    ah = (regs[0] >> 8) & 0xff;
    of = al > 0xf9;
    if (((al & 0x0f) > 9) || f4) {
        al = (al + 6) & 0x0f;
        ah = (ah + 1 + of) & 0xff;
        flags |= 0x0001 | 0x0010;
    } else {
        flags &= ~(0x0001 | 0x0010);
        al &= 0x0f;
    }
    regs[0] = (regs[0] & ~0xffff) | al | (ah << 8);
    osm_src = flags;
    osm_dst = ((osm_src >> 6) & 1) ^ 1;
    osm = 24;
}
void Free86::aux_AAS() {
    int of, al, ah, f4, flags;
    flags = compile_eflags();
    f4 = flags & 0x0010; // AF
    al = regs[0] & 0xff;
    ah = (regs[0] >> 8) & 0xff;
    of = al < 6;
    if (((al & 0x0f) > 9) || f4) {
        al = (al - 6) & 0x0f;
        ah = (ah - 1 - of) & 0xff;
        flags |= 0x0001 | 0x0010;
    } else {
        flags &= ~(0x0001 | 0x0010);
        al &= 0x0f;
    }
    regs[0] = (regs[0] & ~0xffff) | al | (ah << 8);
    osm_src = flags;
    osm_dst = ((osm_src >> 6) & 1) ^ 1;
    osm = 24;
}
void Free86::aux_DAA() {
    int al, f0, f4, flags;
    flags = compile_eflags();
    f0 = flags & 0x0001;
    f4 = flags & 0x0010;
    al = regs[0] & 0xff;
    flags = 0;
    if (((al & 0x0f) > 9) || f4) {
        al = (al + 6) & 0xff;
        flags |= 0x0010;
    }
    if ((al > 0x9f) || f0) {
        al = (al + 0x60) & 0xff;
        flags |= 0x0001;
    }
    regs[0] = (regs[0] & ~0xff) | al;
    flags |= (al == 0) << 6;
    flags |= parity_LUT[al] << 2;
    flags |= al & 0x80;
    osm_src = flags;
    osm_dst = ((osm_src >> 6) & 1) ^ 1;
    osm = 24;
}
void Free86::aux_DAS() {
    int al, of, f0, f4, flags;
    flags = compile_eflags();
    f0 = flags & 0x0001;
    f4 = flags & 0x0010;
    al = regs[0] & 0xff;
    flags = 0;
    of = al;
    if (((al & 0x0f) > 9) || f4) {
        flags |= 0x0010;
        if (al < 6 || f0) {
            flags |= 0x0001;
        }
        al = (al - 6) & 0xff;
    }
    if ((of > 0x99) || f0) {
        al = (al - 0x60) & 0xff;
        flags |= 0x0001;
    }
    regs[0] = (regs[0] & ~0xff) | al;
    flags |= (al == 0) << 6;
    flags |= parity_LUT[al] << 2;
    flags |= al & 0x80;
    osm_src = flags;
    osm_dst = ((osm_src >> 6) & 1) ^ 1;
    osm = 24;
}
void Free86::aux_BOUND16() {
    modRM = ld8_direct();
    if ((modRM >> 6) == 3) {
        abort(6);
    }
    segment_translation();
    imm1st = (ld16_readonly_cpl3() << 16) >> 16;
    lax = lax + 2;
    imm2nd = (ld16_readonly_cpl3() << 16) >> 16;
    reg = (modRM >> 3) & 7;
    r = (regs[reg] << 16) >> 16;
    if (r < imm1st || r > imm2nd) {
        abort(5);
    }
}
void Free86::aux_BOUND() {
    modRM = ld8_direct();
    if ((modRM >> 6) == 3) {
        abort(6);
    }
    segment_translation();
    imm1st = ld_readonly_cpl3();
    lax = lax + 4;
    imm2nd = ld_readonly_cpl3();
    reg = (modRM >> 3) & 7;
    r = regs[reg];
    if (r < imm1st || r > imm2nd) {
        abort(5);
    }
}
void Free86::aux_PUSHA16() {
    lax = ((regs[4] - 16) & SS_mask) + SS_base;
    for (int reg = 7; reg >= 0; reg--) {
        r = regs[reg];
        st16_writable_cpl3(r);
        lax = lax + 2;
    }
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] - 16) & SS_mask);
}
void Free86::aux_PUSHA() {
    lax = ((regs[4] - 32) & SS_mask) + SS_base;
    for (int reg = 7; reg >= 0; reg--) {
        r = regs[reg];
        st_writable_cpl3(r);
        lax = lax + 4;
    }
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] - 32) & SS_mask);
}
void Free86::aux_POPA16() {
    lax = (regs[4] & SS_mask) + SS_base;
    for (int reg = 7; reg >= 0; reg--) {
        if (reg != 4) {
            set_lower_word(reg, ld16_readonly_cpl3());
        }
        lax = lax + 2;
    }
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 16) & SS_mask);
}
void Free86::aux_POPA() {
    lax = (regs[4] & SS_mask) + SS_base;
    for (int reg = 7; reg >= 0; reg--) {
        if (reg != 4) {
            regs[reg] = ld_readonly_cpl3();
        }
        lax = lax + 4;
    }
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 32) & SS_mask);
}
void Free86::aux_LEAVE16() {
    int ebp;
    ebp = regs[5];
    lax = (ebp & SS_mask) + SS_base;
    set_lower_word(5, ld16_readonly_cpl3());
    regs[4] = (regs[4] & ~SS_mask) | ((ebp + 2) & SS_mask);
}
void Free86::aux_LEAVE() {
    int ebp;
    ebp = regs[5];
    lax = (ebp & SS_mask) + SS_base;
    regs[5] = ld_readonly_cpl3();
    regs[4] = (regs[4] & ~SS_mask) | ((ebp + 4) & SS_mask);
}
void Free86::aux_ENTER16() {
    int c, l, esp, ebp, exp;
    c = ld16_direct();
    l = ld8_direct();
    l &= 0x1f;
    esp = regs[4];
    ebp = regs[5];
    esp = esp - 2;
    lax = (esp & SS_mask) + SS_base;
    st16_writable_cpl3(ebp);
    exp = esp;
    if (l != 0) {
        while (l > 1) {
            ebp = ebp - 2;
            lax = (ebp & SS_mask) + SS_base;
            m16 = ld16_readonly_cpl3();
            esp = esp - 2;
            lax = (esp & SS_mask) + SS_base;
            st16_writable_cpl3(m16);
            l--;
        }
        esp = esp - 2;
        lax = (esp & SS_mask) + SS_base;
        st16_writable_cpl3(exp);
    }
    esp = esp - c;
    lax = (esp & SS_mask) + SS_base;
    ld16_writable_cpl3();
    regs[5] = (regs[5] & ~SS_mask) | (exp & SS_mask);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
}
void Free86::aux_ENTER() {
    int c, l, esp, ebp, exp;
    c = ld16_direct();
    l = ld8_direct();
    l &= 0x1f;
    esp = regs[4];
    ebp = regs[5];
    esp = esp - 4;
    lax = (esp & SS_mask) + SS_base;
    st_writable_cpl3(ebp);
    exp = (ebp & ~SS_mask) | (esp & SS_mask);
    if (l != 0) {
        while (l > 1) {
            ebp = ebp - 4;
            lax = (ebp & SS_mask) + SS_base;
            m = ld_readonly_cpl3();
            esp = esp - 4;
            lax = (esp & SS_mask) + SS_base;
            st_writable_cpl3(m);
            l--;
        }
        esp = esp - 4;
        lax = (esp & SS_mask) + SS_base;
        st_writable_cpl3(exp);
    }
    esp = esp - c;
    lax = (esp & SS_mask) + SS_base;
    ld_writable_cpl3();
    regs[5] = (regs[5] & ~SS_mask) | (exp & SS_mask);
    regs[4] = (regs[4] & ~SS_mask) | (esp & SS_mask);
}
void Free86::ld_far_pointer16(int sreg) {
    modRM = ld8_direct();
    if ((modRM >> 3) == 3) {
        ; // abort(6);
    }
    segment_translation();
    imm = ld16_readonly_cpl3();
    lax += 2;
    imm16 = ld16_readonly_cpl3();
    set_segment_register(sreg, imm16);
    set_lower_word((modRM >> 3) & 7, imm);
}
void Free86::ld_far_pointer(int sreg) {
    modRM = ld8_direct();
    if ((modRM >> 3) == 3) {
        ; // abort(6);
    }
    segment_translation();
    imm = ld_readonly_cpl3();
    lax += 4;
    imm16 = ld16_readonly_cpl3();
    set_segment_register(sreg, imm16);
    regs[(modRM >> 3) & 7] = imm;
}
