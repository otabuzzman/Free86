#include "free86.h"

void Free86::fetch_decode_execute(uint64_t cycles, Interrupt& interrupt) {
    uint32_t sreg, hL; // H (0x80) or L (0x00) byte selector
    if (halted) {
        if (get_irq() != 0 && (eflags & 0x00000200)) {
            halted = false;
        } else {
            return;
        }
    }
    cycles_requested = cycles;
    cycles_remaining = cycles;
    far = far_start = 0;
    update_SSB(); // init segments state block
    if (interrupt.id >= 0) {
        raise_interrupt(interrupt.id, interrupt.error_code, 0, 0, 0);
        interrupt = {-1, 0};
    }
    if (get_irq() != 0 && (eflags & 0x00000200)) {
        raise_interrupt(get_iid(), 0, 1, 0, 0);
    }
    do { // cycles (actually instructions)
        fetch_opcode();
        ipr = ipr_default;
        opcode |= ipr & 0x0100;
        while (true) { // loop over instruction bytes (fetch)
            if ((ipr & 0x0040) && !(ipr & 0x0100)) {
                switch (opcode) {
                    case 0x00: // ADD
                    case 0x01: // ADD
                    case 0x08: // OR
                    case 0x09: // OR
                    case 0x10: // ADC
                    case 0x11: // ADC
                    case 0x18: // SBB
                    case 0x19: // SBB
                    case 0x20: // AND
                    case 0x21: // AND
                    case 0x28: // SUB
                    case 0x29: // SUB
                    case 0x30: // XOR
                    case 0x31: // XOR
                    case 0x80: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, -)
                    case 0x81: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, -)
                    case 0x82: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, -)
                    case 0x83: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, -)
                    case 0x86: // XCHG
                    case 0x87: // XCHG
                    case 0xf6: // G3 (-, -, NOT, NEG, -, -, -, -)
                    case 0xf7: // G3 (-, -, NOT, NEG, -, -, -, -)
                    case 0xfe: // G4 (INC, DEC, -, -, -, -, -, -)
                    case 0xff: // G5 (INC, DEC, -, -, -, -, -, -)
                    case 0x0f: // 2-byte instruction escape
                        break;
                    default:
                        abort(6);
                }
            }
            switch (opcode) {
            case 0x26: // ES segment override prefix
            case 0x2e: // CS segment override prefix
            case 0x36: // SS segment override prefix
            case 0x3e: // DS segment override prefix
                if (ipr == ipr_default) {
                    instruction_length(opcode);
                }
                ipr = (ipr & ~0x000fu) | (((opcode >> 3) & 3) + 1);
                opcode = fetch8_data();
                opcode |= ipr & 0x0100;
                break;
            case 0x64: // FS segment override prefix
            case 0x65: // GS segment override prefix
                if (ipr == ipr_default) {
                    instruction_length(opcode);
                }
                ipr = (ipr & ~0x000fu) | ((opcode & 7) + 1);
                opcode = fetch8_data();
                opcode |= ipr & 0x0100;
                break;
            case 0xf0: // LOCK prefix
                if (ipr == ipr_default) {
                    instruction_length(opcode);
                }
                ipr |= 0x0040;
                opcode = fetch8_data();
                opcode |= ipr & 0x0100;
                break;
            case 0xf2: // REPN[EZ] repeat string operation prefix
                if (ipr == ipr_default) {
                    instruction_length(opcode);
                }
                ipr |= 0x0020;
                opcode = fetch8_data();
                opcode |= ipr & 0x0100;
                break;
            case 0xf3: // REP[EZ] repeat string operation prefix
                if (ipr == ipr_default) {
                    instruction_length(opcode);
                }
                ipr |= 0x0010;
                opcode = fetch8_data();
                opcode |= ipr & 0x0100;
                break;
            case 0x66: // operand-size override prefix
                if (ipr == ipr_default) {
                    instruction_length(opcode);
                }
                if (ipr_default & 0x0100) {
                    ipr &= ~0x0100u;
                } else {
                    ipr |= 0x0100;
                }
                opcode = fetch8_data();
                opcode |= ipr & 0x0100;
                break;
            case 0x67: // address-size override prefix
                if (ipr == ipr_default) {
                    instruction_length(opcode);
                }
                if (ipr_default & 0x0080) {
                    ipr &= ~0x0080u;
                } else {
                    ipr |= 0x0080;
                }
                opcode = fetch8_data();
                opcode |= ipr & 0x0100;
                break;
            case 0xb0: // MOV AL
            case 0xb1: // MOV CL
            case 0xb2: // MOV DL
            case 0xb3: // MOV BL
            case 0xb4: // MOV AH
            case 0xb5: // MOV CH
            case 0xb6: // MOV DH
            case 0xb7: // MOV BH
                imm = fetch8_data();
                opcode &= 7;
                hL = (opcode & 4) << 1;
                regs[opcode & 3] = (regs[opcode & 3] & ~(0xffu << hL)) | ((imm & 0xffu) << hL);
                goto FETCH_LOOP;
            case 0xb8: // MOV A
            case 0xb9: // MOV C
            case 0xba: // MOV D
            case 0xbb: // MOV B
            case 0xbc: // MOV SP
            case 0xbd: // MOV BP
            case 0xbe: // MOV SI
            case 0xbf: // MOV DI
                imm = fetch_data();
                regs[opcode & 7] = imm;
                goto FETCH_LOOP;
            case 0x88: // MOV
                modRM = fetch8_data();
                reg = (modRM >> 3) & 7;
                r = regs[reg & 3] >> ((reg & 4) << 1);
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    hL = (rM & 4) << 1;
                    regs[rM & 3] = (regs[rM & 3] & ~(0xffu << hL)) | ((r & 0xffu) << hL);
                } else {
                    segment_translation();
                    st8_writable_cpl3(r);
                }
                goto FETCH_LOOP;
            case 0x89: // MOV
                modRM = fetch8_data();
                r = regs[(modRM >> 3) & 7];
                if ((modRM >> 6) == 3) {
                    regs[modRM & 7] = r;
                } else {
                    segment_translation();
                    st_writable_cpl3(r);
                }
                goto FETCH_LOOP;
            case 0x8a: // MOV
                modRM = fetch8_data();
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    rm = regs[rM & 3] >> ((rM & 4) << 1);
                } else {
                    segment_translation();
                    rm = ld8_readonly_cpl3();
                }
                reg = (modRM >> 3) & 7;
                hL = (reg & 4) << 1;
                regs[reg & 3] = (regs[reg & 3] & ~(0xffu << hL)) | ((rm & 0xffu) << hL);
                goto FETCH_LOOP;
            case 0x8b: // MOV
                modRM = fetch8_data();
                if ((modRM >> 6) == 3) {
                    rm = regs[modRM & 7];
                } else {
                    segment_translation();
                    rm = ld_readonly_cpl3();
                }
                regs[(modRM >> 3) & 7] = rm;
                goto FETCH_LOOP;
            case 0xa0: // MOV AL,
                ld_memory_offset(false);
                moffs = ld8_readonly_cpl3();
                regs[0] = (regs[0] & 0xffffff00) | moffs;
                goto FETCH_LOOP;
            case 0xa1: // MOV AX,
                ld_memory_offset(false);
                moffs = ld_readonly_cpl3();
                regs[0] = moffs;
                goto FETCH_LOOP;
            case 0xa2: // MOV ,AL
                ld_memory_offset(true);
                st8_writable_cpl3(regs[0]);
                goto FETCH_LOOP;
            case 0xa3: // MOV ,AX
                ld_memory_offset(true);
                st_writable_cpl3(regs[0]);
                goto FETCH_LOOP;
            case 0xc6: // MOV
                modRM = fetch8_data();
                if ((modRM >> 6) == 3) {
                    imm = fetch8_data();
                    set_lower_byte(modRM & 7, imm);
                } else {
                    segment_translation();
                    imm = fetch8_data();
                    st8_writable_cpl3(imm);
                }
                goto FETCH_LOOP;
            case 0xc7: // MOV
                modRM = fetch8_data();
                if ((modRM >> 6) == 3) {
                    imm = fetch_data();
                    regs[modRM & 7] = imm;
                } else {
                    segment_translation();
                    imm = fetch_data();
                    st_writable_cpl3(imm);
                }
                goto FETCH_LOOP;
            case 0x8e: // MOV
                modRM = fetch8_data();
                reg = (modRM >> 3) & 7;
                if (reg >= 6 || reg == 1) {
                    abort(6);
                }
                if ((modRM >> 6) == 3) {
                    rm = regs[modRM & 7] & 0xffff;
                } else {
                    segment_translation();
                    rm = ld16_readonly_cpl3();
                }
                set_segment_register(reg, rm);
                goto FETCH_LOOP;
            case 0x8c: // MOV
                modRM = fetch8_data();
                reg = (modRM >> 3) & 7;
                if (reg >= 6) {
                    abort(6);
                }
                u = segs[reg].selector;
                if ((modRM >> 6) == 3) {
                    if (((ipr >> 8) & 1) ^ 1) {
                        regs[modRM & 7] = u;
                    } else {
                        set_lower_word(modRM & 7, u);
                    }
                } else {
                    segment_translation();
                    st16_writable_cpl3(u);
                }
                goto FETCH_LOOP;
            case 0x86: // XCHG
                modRM = fetch8_data();
                reg = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    // LOCK prefix not allowed
                    rM = modRM & 7;
                    rm = regs[rM & 3] >> ((rM & 4) << 1);
                    set_lower_byte(rM, (regs[reg & 3] >> ((reg & 4) << 1)));
                } else {
                    segment_translation();
                    rm = ld8_writable_cpl3();
                    st8_writable_cpl3((regs[reg & 3] >> ((reg & 4) << 1)));
                }
                set_lower_byte(reg, rm);
                goto FETCH_LOOP;
            case 0x87: // XCHG
                modRM = fetch8_data();
                reg = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    // LOCK prefix not allowed
                    rM = modRM & 7;
                    rm = regs[rM];
                    regs[rM] = regs[reg];
                } else {
                    segment_translation();
                    rm = ld_writable_cpl3();
                    st_writable_cpl3(regs[reg]);
                }
                regs[reg] = rm;
                goto FETCH_LOOP;
            case 0x91: // XCHG C
            case 0x92: // XCHG D
            case 0x93: // XCHG B
            case 0x94: // XCHG SP
            case 0x95: // XCHG BP
            case 0x96: // XCHG SI
            case 0x97: // XCHG DI
                reg = opcode & 7;
                u = regs[0];
                regs[0] = regs[reg];
                regs[reg] = u;
                goto FETCH_LOOP;
            case 0xd7: // XLAT
                lax = regs[3] + (regs[0] & 0xff);
                if (ipr & 0x0080) {
                    lax &= 0xffff;
                }
                sreg = ipr & 0x000f;
                if (sreg == 0) {
                    sreg = 3;
                } else {
                    sreg--;
                }
                lax = segs[sreg].shadow.base + lax;
                m = ld8_readonly_cpl3();
                set_lower_byte(0, m);
                goto FETCH_LOOP;
            case 0xc4: // LES
                ld_far_pointer(0);
                goto FETCH_LOOP;
            case 0xc5: // LDS
                ld_far_pointer(3);
                goto FETCH_LOOP;
            case 0x00: // ADD
            case 0x08: // OR
            case 0x10: // ADC
            case 0x18: // SBB
            case 0x20: // AND
            case 0x28: // SUB
            case 0x30: // XOR
            case 0x38: // CMP
                modRM = fetch8_data();
                operation = opcode >> 3;
                reg = (modRM >> 3) & 7;
                r = regs[reg & 3] >> ((reg & 4) << 1);
                if ((modRM >> 6) == 3) {
                    // LOCK prefix not allowed
                    rM = modRM & 7;
                    set_lower_byte(rM, calculate8((regs[rM & 3] >> ((rM & 4) << 1)), r));
                } else {
                    segment_translation();
                    if (operation != 7) {
                        rm = ld8_writable_cpl3();
                        u = calculate8(rm, r);
                        st8_writable_cpl3(u);
                    } else {
                        // LOCK prefix not allowed
                        rm = ld8_readonly_cpl3();
                        calculate8(rm, r);
                    }
                }
                goto FETCH_LOOP;
            case 0x01: // ADD
                modRM = fetch8_data();
                r = regs[(modRM >> 3) & 7];
                if ((modRM >> 6) == 3) {
                    // LOCK prefix not allowed
                    rM = modRM & 7;
                    osm_src = r;
                    osm_dst = regs[rM] = regs[rM] + r;
                    osm = 2;
                } else {
                    segment_translation();
                    rm = ld_writable_cpl3();
                    osm_src = r;
                    osm_dst = rm = rm + r;
                    osm = 2;
                    st_writable_cpl3(rm);
                }
                goto FETCH_LOOP;
            case 0x09: // OR
            case 0x11: // ADC
            case 0x19: // SBB
            case 0x21: // AND
            case 0x29: // SUB
            case 0x31: // XOR
                modRM = fetch8_data();
                operation = opcode >> 3;
                r = regs[(modRM >> 3) & 7];
                if ((modRM >> 6) == 3) {
                    // LOCK prefix not allowed
                    rM = modRM & 7;
                    regs[rM] = calculate(regs[rM], r);
                } else {
                    segment_translation();
                    rm = ld_writable_cpl3();
                    u = calculate(rm, r);
                    st_writable_cpl3(u);
                }
                goto FETCH_LOOP;
            case 0x39: // CMP
                modRM = fetch8_data();
                operation = opcode >> 3;
                r = regs[(modRM >> 3) & 7];
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    osm_src = r;
                    osm_dst = regs[rM] - r;
                    osm = 8;
                } else {
                    segment_translation();
                    rm = ld_readonly_cpl3();
                    osm_src = r;
                    osm_dst = rm - r;
                    osm = 8;
                }
                goto FETCH_LOOP;
            case 0x02: // ADD
            case 0x0a: // OR
            case 0x12: // ADC
            case 0x1a: // SBB
            case 0x22: // AND
            case 0x2a: // SUB
            case 0x32: // XOR
            case 0x3a: // CMP
                modRM = fetch8_data();
                operation = opcode >> 3;
                reg = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    rm = regs[rM & 3] >> ((rM & 4) << 1);
                } else {
                    segment_translation();
                    rm = ld8_readonly_cpl3();
                }
                set_lower_byte(reg, calculate8((regs[reg & 3] >> ((reg & 4) << 1)), rm));
                goto FETCH_LOOP;
            case 0x03: // ADD
                modRM = fetch8_data();
                reg = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rm = regs[modRM & 7];
                } else {
                    segment_translation();
                    rm = ld_readonly_cpl3();
                }
                osm_src = rm;
                osm_dst = regs[reg] = regs[reg] + rm;
                osm = 2;
                goto FETCH_LOOP;
            case 0x0b: // OR
            case 0x13: // ADC
            case 0x1b: // SBB
            case 0x23: // AND
            case 0x2b: // SUB
            case 0x33: // XOR
                modRM = fetch8_data();
                operation = opcode >> 3;
                reg = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rm = regs[modRM & 7];
                } else {
                    segment_translation();
                    rm = ld_readonly_cpl3();
                }
                regs[reg] = calculate(regs[reg], rm);
                goto FETCH_LOOP;
            case 0x3b: // CMP
                modRM = fetch8_data();
                operation = opcode >> 3;
                reg = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rm = regs[modRM & 7];
                } else {
                    segment_translation();
                    rm = ld_readonly_cpl3();
                }
                osm_src = rm;
                osm_dst = regs[reg] - rm;
                osm = 8;
                goto FETCH_LOOP;
            case 0x04: // ADD
            case 0x0c: // OR
            case 0x14: // ADC
            case 0x1c: // SBB
            case 0x24: // AND
            case 0x2c: // SUB
            case 0x34: // XOR
            case 0x3c: // CMP
                imm = fetch8_data();
                operation = opcode >> 3;
                set_lower_byte(0, calculate8(regs[0] & 0xff, imm));
                goto FETCH_LOOP;
            case 0x05: // ADD
                imm = fetch_data();
                osm_src = imm;
                osm_dst = regs[0] = regs[0] + imm;
                osm = 2;
                goto FETCH_LOOP;
            case 0x0d: // OR
            case 0x15: // ADC
            case 0x1d: // SBB
            case 0x25: // AND
            case 0x2d: // SUB
                imm = fetch_data();
                operation = opcode >> 3;
                regs[0] = calculate(regs[0], imm);
                goto FETCH_LOOP;
            case 0x35: // XOR
                imm = fetch_data();
                osm_dst = regs[0] = regs[0] ^ imm;
                osm = 14;
                goto FETCH_LOOP;
            case 0x3d: // CMP
                imm = fetch_data();
                osm_src = imm;
                osm_dst = regs[0] - imm;
                osm = 8;
                goto FETCH_LOOP;
            case 0x80: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
            case 0x82: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                modRM = fetch8_data();
                operation = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    // LOCK prefix not allowed
                    rM = modRM & 7;
                    imm = fetch8_data();
                    set_lower_byte(rM, calculate8((regs[rM & 3] >> ((rM & 4) << 1)), imm));
                } else {
                    segment_translation();
                    imm = fetch8_data();
                    if (operation != 7) {
                        rm = ld8_writable_cpl3();
                        u = calculate8(rm, imm);
                        st8_writable_cpl3(u);
                    } else {
                        // LOCK prefix not allowed
                        rm = ld8_readonly_cpl3();
                        calculate8(rm, imm);
                    }
                }
                goto FETCH_LOOP;
            case 0x81: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                modRM = fetch8_data();
                operation = (modRM >> 3) & 7;
                if (operation == 7) {
                    // LOCK prefix not allowed
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    imm = fetch_data();
                    osm_src = imm;
                    osm_dst = rm - imm;
                    osm = 8;
                } else {
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        imm = fetch_data();
                        regs[rM] = calculate(regs[rM], imm);
                    } else {
                        segment_translation();
                        imm = fetch_data();
                        rm = ld_writable_cpl3();
                        u = calculate(rm, imm);
                        st_writable_cpl3(u);
                    }
                }
                goto FETCH_LOOP;
            case 0x83: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                modRM = fetch8_data();
                operation = (modRM >> 3) & 7;
                if (operation == 7) {
                    // LOCK prefix not allowed
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    u = sign_extend_byte(fetch8_data());
                    osm_src = u;
                    osm_dst = rm - u;
                    osm = 8;
                } else {
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        u = sign_extend_byte(fetch8_data());
                        regs[rM] = calculate(regs[rM], u);
                    } else {
                        segment_translation();
                        u = sign_extend_byte(fetch8_data());
                        rm = ld_writable_cpl3();
                        v = calculate(rm, u);
                        st_writable_cpl3(v);
                    }
                }
                goto FETCH_LOOP;
            case 0x40: // INC A
            case 0x41: // INC C
            case 0x42: // INC D
            case 0x43: // INC B
            case 0x44: // INC SP
            case 0x45: // INC BP
            case 0x46: // INC SI
            case 0x47: // INC DI
                reg = opcode & 7;
                if (osm < 25) {
                    osm_preserved = osm;
                    osm_dst_preserved = osm_dst;
                }
                regs[reg] = osm_dst = regs[reg] + 1;
                osm = 27;
                goto FETCH_LOOP;
            case 0x48: // DEC A
            case 0x49: // DEC C
            case 0x4a: // DEC D
            case 0x4b: // DEC B
            case 0x4c: // DEC SP
            case 0x4d: // DEC BP
            case 0x4e: // DEC SI
            case 0x4f: // DEC DI
                reg = opcode & 7;
                if (osm < 25) {
                    osm_preserved = osm;
                    osm_dst_preserved = osm_dst;
                }
                regs[reg] = osm_dst = regs[reg] - 1;
                osm = 30;
                goto FETCH_LOOP;
            case 0x69: // IMUL
                modRM = fetch8_data();
                reg = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rm = regs[modRM & 7];
                } else {
                    segment_translation();
                    rm = ld_readonly_cpl3();
                }
                imm = fetch_data();
                aux_IMUL(rm, imm);
                regs[reg] = u;
                goto FETCH_LOOP;
            case 0x6b: // IMUL
                modRM = fetch8_data();
                reg = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rm = regs[modRM & 7];
                } else {
                    segment_translation();
                    rm = ld_readonly_cpl3();
                }
                v = sign_extend_byte(fetch8_data());
                aux_IMUL(rm, v);
                regs[reg] = u;
                goto FETCH_LOOP;
            case 0x84: // TEST
                modRM = fetch8_data();
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    rm = regs[rM & 3] >> ((rM & 4) << 1);
                } else {
                    segment_translation();
                    rm = ld8_readonly_cpl3();
                }
                reg = (modRM >> 3) & 7;
                r = regs[reg & 3] >> ((reg & 4) << 1);
                osm_dst = sign_extend_byte(rm & r);
                osm = 12;
                goto FETCH_LOOP;
            case 0x85: // TEST
                modRM = fetch8_data();
                if ((modRM >> 6) == 3) {
                    rm = regs[modRM & 7];
                } else {
                    segment_translation();
                    rm = ld_readonly_cpl3();
                }
                r = regs[(modRM >> 3) & 7];
                osm_dst = rm & r;
                osm = 14;
                goto FETCH_LOOP;
            case 0xa8: // TEST
                imm = fetch8_data();
                osm_dst = sign_extend_byte(regs[0] & imm);
                osm = 12;
                goto FETCH_LOOP;
            case 0xa9: // TEST
                imm = fetch_data();
                osm_dst = regs[0] & imm;
                osm = 14;
                goto FETCH_LOOP;
            case 0xf6: // G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
                modRM = fetch8_data();
                operation = (modRM >> 3) & 7;
                // LOCK prefix not allowed if operation != 2 && operation != 3
                switch (operation) {
                case 0: // TEST
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        rm = regs[rM & 3] >> ((rM & 4) << 1);
                    } else {
                        segment_translation();
                        rm = ld8_readonly_cpl3();
                    }
                    imm = fetch8_data();
                    osm_dst = sign_extend_byte(rm & imm);
                    osm = 12;
                    break;
                case 2: // NOT
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        set_lower_byte(rM, ~(regs[rM & 3] >> ((rM & 4) << 1)));
                    } else {
                        segment_translation();
                        rm = ld8_writable_cpl3();
                        st8_writable_cpl3(~rm);
                    }
                    break;
                case 3: // NEG
                    operation = 5;
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        set_lower_byte(rM, calculate8(0, (regs[rM & 3] >> ((rM & 4) << 1))));
                    } else {
                        segment_translation();
                        rm = ld8_writable_cpl3();
                        u = calculate8(0, rm);
                        st8_writable_cpl3(u);
                    }
                    break;
                case 4: // MUL AL/X
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        rm = regs[rM & 3] >> ((rM & 4) << 1);
                    } else {
                        segment_translation();
                        rm = ld8_readonly_cpl3();
                    }
                    aux8_MUL(regs[0], rm);
                    set_lower_word(0, u);
                    break;
                case 5: // IMUL AL/X
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        rm = regs[rM & 3] >> ((rM & 4) << 1);
                    } else {
                        segment_translation();
                        rm = ld8_readonly_cpl3();
                    }
                    aux8_IMUL(regs[0], rm);
                    set_lower_word(0, u);
                    break;
                case 6: // DIV AL/X
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        rm = regs[rM & 3] >> ((rM & 4) << 1);
                    } else {
                        segment_translation();
                        rm = ld8_readonly_cpl3();
                    }
                    aux8_DIV(rm);
                    break;
                case 7: // IDIV AL/X
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        rm = regs[rM & 3] >> ((rM & 4) << 1);
                    } else {
                        segment_translation();
                        rm = ld8_readonly_cpl3();
                    }
                    aux8_IDIV(rm);
                    break;
                default:
                    abort(6);
                }
                goto FETCH_LOOP;
            case 0xf7: // G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
                modRM = fetch8_data();
                operation = (modRM >> 3) & 7;
                // LOCK prefix not allowed if operation != 2 && operation != 3
                switch (operation) {
                case 0: // TEST
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    imm = fetch_data();
                    osm_dst = rm & imm;
                    osm = 14;
                    break;
                case 2: // NOT
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        regs[rM] = ~regs[rM];
                    } else {
                        segment_translation();
                        rm = ld_writable_cpl3();
                        st_writable_cpl3(~rm);
                    }
                    break;
                case 3: // NEG
                    operation = 5;
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        regs[rM] = calculate(0, regs[rM]);
                    } else {
                        segment_translation();
                        rm = ld_writable_cpl3();
                        u = calculate(0, rm);
                        st_writable_cpl3(u);
                    }
                    break;
                case 4: // MUL AL/X
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    aux_MUL(regs[0], rm);
                    regs[0] = u;
                    regs[2] = v;
                    break;
                case 5: // IMUL AL/X
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    aux_IMUL(regs[0], rm);
                    regs[0] = u;
                    regs[2] = v;
                    break;
                case 6: // DIV AL/X
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    aux_DIV((regs[0] & 0xffffffff) | static_cast<uint64_t>(regs[2]) << 32, rm);
                    regs[0] = u;
                    regs[2] = v;
                    break;
                case 7: // IDIV AL/X
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    aux_IDIV((regs[0] & 0xffffffff) | static_cast<uint64_t>(regs[2]) << 32, rm);
                    regs[0] = u;
                    regs[2] = v;
                    break;
                default:
                    abort(6);
                }
                goto FETCH_LOOP;
            case 0xc0: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
                modRM = fetch8_data();
                operation = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    imm = fetch8_data();
                    rM = modRM & 7;
                    set_lower_byte(rM, shift8((regs[rM & 3] >> ((rM & 4) << 1)), imm));
                } else {
                    segment_translation();
                    imm = fetch8_data();
                    rm = ld8_writable_cpl3();
                    u = shift8(rm, imm);
                    st8_writable_cpl3(u);
                }
                goto FETCH_LOOP;
            case 0xc1: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
                modRM = fetch8_data();
                operation = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    imm = fetch8_data();
                    rM = modRM & 7;
                    regs[rM] = shift(regs[rM], imm);
                } else {
                    segment_translation();
                    imm = fetch8_data();
                    rm = ld_writable_cpl3();
                    u = shift(rm, imm);
                    st_writable_cpl3(u);
                }
                goto FETCH_LOOP;
            case 0xd0: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
                modRM = fetch8_data();
                operation = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    set_lower_byte(rM, shift8((regs[rM & 3] >> ((rM & 4) << 1)), 1));
                } else {
                    segment_translation();
                    rm = ld8_writable_cpl3();
                    u = shift8(rm, 1);
                    st8_writable_cpl3(u);
                }
                goto FETCH_LOOP;
            case 0xd1: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
                modRM = fetch8_data();
                operation = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    regs[rM] = shift(regs[rM], 1);
                } else {
                    segment_translation();
                    rm = ld_writable_cpl3();
                    u = shift(rm, 1);
                    st_writable_cpl3(u);
                }
                goto FETCH_LOOP;
            case 0xd2: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
                modRM = fetch8_data();
                operation = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    set_lower_byte(rM, shift8((regs[rM & 3] >> ((rM & 4) << 1)), regs[1] & 0xff));
                } else {
                    segment_translation();
                    rm = ld8_writable_cpl3();
                    u = shift8(rm, regs[1] & 0xff);
                    st8_writable_cpl3(u);
                }
                goto FETCH_LOOP;
            case 0xd3: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
                modRM = fetch8_data();
                operation = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    regs[rM] = shift(regs[rM], regs[1] & 0xff);
                } else {
                    segment_translation();
                    rm = ld_writable_cpl3();
                    u = shift(rm, regs[1] & 0xff);
                    st_writable_cpl3(u);
                }
                goto FETCH_LOOP;
            case 0x98: // CBW
                regs[0] = sign_extend_word(regs[0]);
                goto FETCH_LOOP;
            case 0x99: // CWD
                regs[2] = sign_shift_right(regs[0], 31);
                goto FETCH_LOOP;
            case 0x50: // PUSH A
            case 0x51: // PUSH C
            case 0x52: // PUSH D
            case 0x53: // PUSH B
            case 0x54: // PUSH SP
            case 0x55: // PUSH BP
            case 0x56: // PUSH SI
            case 0x57: // PUSH DI
                r = regs[opcode & 7];
                if (x86_64_long_mode) {
                    lax = regs[4] - 4;
                    st_writable_cpl3(r);
                    regs[4] = lax;
                } else {
                    push(r);
                }
                goto FETCH_LOOP;
            case 0x58: // POP A
            case 0x59: // POP C
            case 0x5a: // POP D
            case 0x5b: // POP B
            case 0x5c: // POP SP
            case 0x5d: // POP BP
            case 0x5e: // POP SI
            case 0x5f: // POP DI
                if (x86_64_long_mode) {
                    lax = regs[4];
                    m = ld_readonly_cpl3();
                    regs[4] = lax + 4;
                } else {
                    m = pop();
                }
                regs[opcode & 7] = m;
                goto FETCH_LOOP;
            case 0x60: // PUSHA
                aux_PUSHA();
                goto FETCH_LOOP;
            case 0x61: // POPA
                aux_POPA();
                goto FETCH_LOOP;
            case 0x8f: // POP
                modRM = fetch8_data();
                if ((modRM >> 6) == 3) {
                    m = pop();
                    regs[modRM & 7] = m;
                } else {
                    u = regs[4];
                    m = pop();
                    v = regs[4];
                    segment_translation();
                    regs[4] = u;
                    st_writable_cpl3(m);
                    regs[4] = v;
                }
                goto FETCH_LOOP;
            case 0x68: // PUSH
                imm = fetch_data();
                if (x86_64_long_mode) {
                    lax = regs[4] - 4;
                    st_writable_cpl3(imm);
                    regs[4] = lax;
                } else {
                    push(imm);
                }
                goto FETCH_LOOP;
            case 0x6a: // PUSH
                u = sign_extend_byte(fetch8_data());
                if (x86_64_long_mode) {
                    lax = regs[4] - 4;
                    st_writable_cpl3(u);
                    regs[4] = lax;
                } else {
                    push(u);
                }
                goto FETCH_LOOP;
            case 0xc8: // ENTER
                aux_ENTER();
                goto FETCH_LOOP;
            case 0xc9: // LEAVE
                if (x86_64_long_mode) {
                    lax = regs[5];
                    regs[5] = ld_readonly_cpl3();
                    regs[4] = lax + 4;
                } else {
                    aux_LEAVE();
                }
                goto FETCH_LOOP;
            case 0x9c: // PUSHF
                iopl = (eflags >> 12) & 3;
                if ((eflags & 0x00020000) && iopl != 3) {
                    abort(13);
                }
                u = get_EFLAGS() & ~(0x00010000u | 0x00020000u);
                if (((ipr >> 8) & 1) ^ 1) {
                    push(u);
                } else {
                    push16(u);
                }
                goto FETCH_LOOP;
            case 0x9d: // POPF
                iopl = (eflags >> 12) & 3;
                if ((eflags & 0x00020000) && iopl != 3) {
                    abort(13);
                }
                if (((ipr >> 8) & 1) ^ 1) {
                    m = pop();
                    u = 0xffffffff;
                } else {
                    m = pop16();
                    u = 0xffff;
                }
                v = 0x00000100 | 0x00004000 | 0x00040000 | 0x00200000;
                if (cpl == 0) {
                    v |= 0x00000200 | 0x00003000;
                } else {
                    if (cpl <= iopl) {
                        v |= 0x00000200;
                    }
                }
                set_EFLAGS(m, v & u);
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto FETCH_LOOP;
            case 0x06: // PUSH
            case 0x0e: // PUSH
            case 0x16: // PUSH
            case 0x1e: // PUSH
                push(segs[opcode >> 3].selector);
                goto FETCH_LOOP;
            case 0x07: // POP
            case 0x17: // POP
            case 0x1f: // POP
                m = pop() & 0xffff;
                set_segment_register(opcode >> 3, m);
                goto FETCH_LOOP;
            case 0x8d: // LEA
                modRM = fetch8_data();
                if ((modRM >> 6) == 3) {
                    abort(6);
                }
                ipr = (ipr & ~0x000fu) | (6 + 1);
                segment_translation();
                regs[(modRM >> 3) & 7] = lax;
                goto FETCH_LOOP;
            case 0xfe: // G4 (INC, DEC, -, -, -, -, -, -)
                modRM = fetch8_data();
                operation = (modRM >> 3) & 7;
                switch (operation) {
                case 0: // INC
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        set_lower_byte(rM, aux8_INC((regs[rM & 3] >> ((rM & 4) << 1))));
                    } else {
                        segment_translation();
                        rm = ld8_writable_cpl3();
                        u = aux8_INC(rm);
                        st8_writable_cpl3(u);
                    }
                    break;
                case 1: // DEC
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        set_lower_byte(rM, aux8_DEC((regs[rM & 3] >> ((rM & 4) << 1))));
                    } else {
                        segment_translation();
                        rm = ld8_writable_cpl3();
                        u = aux8_DEC(rm);
                        st8_writable_cpl3(u);
                    }
                    break;
                default:
                    abort(6);
                }
                goto FETCH_LOOP;
            case 0xff: // G5 (INC, DEC, CALL, CALL, JMP, JMP, PUSH, -)
                modRM = fetch8_data();
                operation = (modRM >> 3) & 7;
                // LOCK prefix not allowed if operation != 0 && operation != 1
                switch (operation) {
                case 0: // INC
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        if (osm < 25) {
                            osm_preserved = osm;
                            osm_dst_preserved = osm_dst;
                        }
                        regs[rM] = osm_dst = regs[rM] + 1;
                        osm = 27;
                    } else {
                        segment_translation();
                        rm = ld_writable_cpl3();
                        if (osm < 25) {
                            osm_preserved = osm;
                            osm_dst_preserved = osm_dst;
                        }
                        rm = osm_dst = rm + 1;
                        osm = 27;
                        st_writable_cpl3(rm);
                    }
                    break;
                case 1: // DEC
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        if (osm < 25) {
                            osm_preserved = osm;
                            osm_dst_preserved = osm_dst;
                        }
                        regs[rM] = osm_dst = regs[rM] - 1;
                        osm = 30;
                    } else {
                        segment_translation();
                        rm = ld_writable_cpl3();
                        if (osm < 25) {
                            osm_preserved = osm;
                            osm_dst_preserved = osm_dst;
                        }
                        rm = osm_dst = rm - 1;
                        osm = 30;
                        st_writable_cpl3(rm);
                    }
                    break;
                case 2: // CALL
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    u = eip + far - far_start;
                    if (x86_64_long_mode) {
                        lax = regs[4] - 4;
                        st_writable_cpl3(u);
                        regs[4] = lax;
                    } else {
                        push(u);
                    }
                    eip = rm, far = far_start = 0;
                    break;
                case 3: // CALLF
                case 5: // JMPF
                    if ((modRM >> 6) == 3) {
                        abort(6);
                    }
                    segment_translation();
                    m = ld_readonly_cpl3();
                    lax = lax + 4;
                    m16 = ld16_readonly_cpl3();
                    if (operation == 3) {
                        aux_CALLF(1, m16, m, (eip + far - far_start));
                    } else {
                        aux_JMPF(m16, m);
                    }
                    break;
                case 4: // JMP
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    eip = rm, far = far_start = 0;
                    break;
                case 6: // PUSH
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    if (x86_64_long_mode) {
                        lax = regs[4] - 4;
                        st_writable_cpl3(rm);
                        regs[4] = lax;
                    } else {
                        push(rm);
                    }
                    break;
                default:
                    abort(6);
                }
                goto FETCH_LOOP;
            case 0xeb: // JMP
                u = sign_extend_byte(fetch8_data());
                far = far + u;
                goto FETCH_LOOP;
            case 0xe9: // JMP
                imm = fetch_data();
                far = far + imm;
                goto FETCH_LOOP;
            case 0xea: // JMPF
                if (((ipr >> 8) & 1) ^ 1) {
                    imm = fetch_data();
                } else {
                    imm = fetch16_data();
                }
                imm16 = fetch16_data();
                aux_JMPF(imm16, imm);
                goto FETCH_LOOP;
            case 0x70: // JO
                if (is_OF()) {
                    u = sign_extend_byte(fetch8_data());
                    far = far + u;
                } else {
                    far = far + 1;
                }
                goto FETCH_LOOP;
            case 0x71: // JNO
                if (!is_OF()) {
                    u = sign_extend_byte(fetch8_data());
                    far = far + u;
                } else {
                    far = far + 1;
                }
                goto FETCH_LOOP;
            case 0x72: // JB
                if (is_CF()) {
                    u = sign_extend_byte(fetch8_data());
                    far = far + u;
                } else {
                    far = far + 1;
                }
                goto FETCH_LOOP;
            case 0x73: // JNB
                if (!is_CF()) {
                    u = sign_extend_byte(fetch8_data());
                    far = far + u;
                } else {
                    far = far + 1;
                }
                goto FETCH_LOOP;
            case 0x74: // JZ
                if (osm_dst == 0) {
                    u = sign_extend_byte(fetch8_data());
                    far = far + u;
                } else {
                    far = far + 1;
                }
                goto FETCH_LOOP;
            case 0x75: // JNZ
                if (!(osm_dst == 0)) {
                    u = sign_extend_byte(fetch8_data());
                    far = far + u;
                } else {
                    far = far + 1;
                }
                goto FETCH_LOOP;
            case 0x76: // JBE
                if (is_BE()) {
                    u = sign_extend_byte(fetch8_data());
                    far = far + u;
                } else {
                    far = far + 1;
                }
                goto FETCH_LOOP;
            case 0x77: // JNBE
                if (!is_BE()) {
                    u = sign_extend_byte(fetch8_data());
                    far = far + u;
                } else {
                    far = far + 1;
                }
                goto FETCH_LOOP;
            case 0x78: // JS
                if (osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst & 0x80000000 ? 1 : 0)) {
                    u = sign_extend_byte(fetch8_data());
                    far = far + u;
                } else {
                    far = far + 1;
                }
                goto FETCH_LOOP;
            case 0x79: // JNS
                if (!(osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst & 0x80000000 ? 1 : 0))) {
                    u = sign_extend_byte(fetch8_data());
                    far = far + u;
                } else {
                    far = far + 1;
                }
                goto FETCH_LOOP;
            case 0x7a: // JP
                if (is_PF()) {
                    u = sign_extend_byte(fetch8_data());
                    far = far + u;
                } else {
                    far = far + 1;
                }
                goto FETCH_LOOP;
            case 0x7b: // JNP
                if (!is_PF()) {
                    u = sign_extend_byte(fetch8_data());
                    far = far + u;
                } else {
                    far = far + 1;
                }
                goto FETCH_LOOP;
            case 0x7c: // JL
                if (is_LT()) {
                    u = sign_extend_byte(fetch8_data());
                    far = far + u;
                } else {
                    far = far + 1;
                }
                goto FETCH_LOOP;
            case 0x7d: // JNL
                if (!is_LT()) {
                    u = sign_extend_byte(fetch8_data());
                    far = far + u;
                } else {
                    far = far + 1;
                }
                goto FETCH_LOOP;
            case 0x7e: // JLE
                if (is_LE()) {
                    u = sign_extend_byte(fetch8_data());
                    far = far + u;
                } else {
                    far = far + 1;
                }
                goto FETCH_LOOP;
            case 0x7f: // JNLE
                if (!is_LE()) {
                    u = sign_extend_byte(fetch8_data());
                    far = far + u;
                } else {
                    far = far + 1;
                }
                goto FETCH_LOOP;
            case 0xe0: // LOOPNE
            case 0xe1: // LOOPE
            case 0xe2: // LOOP
                w = sign_extend_byte(fetch8_data());
                ipr_os_mask = (ipr & 0x0080) ? 0xffff : 0xffffffff;
                u = (regs[1] - 1) & ipr_os_mask;
                regs[1] = (regs[1] & ~ipr_os_mask) | u;
                opcode &= 3;
                if (opcode == 0) {
                    v = osm_dst != 0;
                } else if (opcode == 1) {
                    v = osm_dst == 0;
                } else {
                    v = 1;
                }
                if (u && v) {
                    if (ipr & 0x0100) {
                        eip = (eip + far - far_start + w) & 0xffff;
                        far = far_start = 0;
                    } else {
                        far = far + w;
                    }
                }
                goto FETCH_LOOP;
            case 0xe3: // JCXZ
                u = sign_extend_byte(fetch8_data());
                ipr_os_mask = (ipr & 0x0080) ? 0xffff : 0xffffffff;
                if ((regs[1] & ipr_os_mask) == 0) {
                    if (ipr & 0x0100) {
                        eip = (eip + far - far_start + u) & 0xffff;
                        far = far_start = 0;
                    } else {
                        far = far + u;
                    }
                }
                goto FETCH_LOOP;
            case 0xc2: // RET
                u = sign_extend_word(fetch16_data());
                m = ld_stack();
                regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 4 + u) & SS_mask);
                eip = m, far = far_start = 0;
                goto FETCH_LOOP;
            case 0xc3: // RET
                if (x86_64_long_mode) {
                    lax = regs[4];
                    m = ld_readonly_cpl3();
                    regs[4] = regs[4] + 4;
                } else {
                    m = pop();
                }
                eip = m, far = far_start = 0;
                goto FETCH_LOOP;
            case 0xe8: // CALL
                imm = fetch_data();
                u = eip + far - far_start;
                if (x86_64_long_mode) {
                    lax = regs[4] - 4;
                    st_writable_cpl3(u);
                    regs[4] = lax;
                } else {
                    push(u);
                }
                far = far + imm;
                goto FETCH_LOOP;
            case 0x9a: // CALLF
                u = ((ipr >> 8) & 1) ^ 1;
                if (u) {
                    imm = fetch_data();
                } else {
                    imm = fetch16_data();
                }
                imm16 = fetch16_data();
                aux_CALLF(u, imm16, imm, (eip + far - far_start));
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto FETCH_LOOP;
            case 0xca: // RET
                u = sign_extend_word(fetch16_data());
                aux_RETF((((ipr >> 8) & 1) ^ 1), u);
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto FETCH_LOOP;
            case 0xcb: // RET
                aux_RETF((((ipr >> 8) & 1) ^ 1), 0);
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto FETCH_LOOP;
            case 0xcf: // IRET
                aux_IRET((((ipr >> 8) & 1) ^ 1));
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto FETCH_LOOP;
            case 0x90: // NOP
                goto FETCH_LOOP;
            case 0xcc: // INT
                u = eip + far - far_start;
                raise_interrupt(3, 0, 0, 1, u);
                goto FETCH_LOOP;
            case 0xcd: // INT
                imm = fetch8_data();
                if ((eflags & 0x00020000) && ((eflags >> 12) & 3) != 3) {
                    abort(13);
                }
                u = eip + far - far_start;
                raise_interrupt(imm, 0, 0, 1, u);
                goto FETCH_LOOP;
            case 0xce: // INTO
                if (is_OF()) {
                    u = eip + far - far_start;
                    raise_interrupt(4, 0, 0, 1, u);
                }
                goto FETCH_LOOP;
            case 0x62: // BOUND
                aux_BOUND();
                goto FETCH_LOOP;
            case 0xf5: // CMC
                osm_src = compile_EFLAGS() ^ 0x0001;
                osm_dst = ((osm_src >> 6) & 1) ^ 1;
                osm = 24;
                goto FETCH_LOOP;
            case 0xf8: // CLC
                osm_src = compile_EFLAGS() & ~0x0001u;
                osm_dst = ((osm_src >> 6) & 1) ^ 1;
                osm = 24;
                goto FETCH_LOOP;
            case 0xf9: // STC
                osm_src = compile_EFLAGS() | 0x0001;
                osm_dst = ((osm_src >> 6) & 1) ^ 1;
                osm = 24;
                goto FETCH_LOOP;
            case 0xfc: // CLD
                df = 1;
                goto FETCH_LOOP;
            case 0xfd: // STD
                df = -1;
                goto FETCH_LOOP;
            case 0xfa: // CLI
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                eflags &= ~0x00000200u;
                goto FETCH_LOOP;
            case 0xfb: // STI
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                eflags |= 0x00000200;
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto FETCH_LOOP;
            case 0x9e: // SAHF
                osm_src = ((regs[0] >> 8) & (0x0080 | 0x0040 | 0x0010 | 0x0004 | 0x0001)) | (is_OF() << 11);
                osm_dst = ((osm_src >> 6) & 1) ^ 1;
                osm = 24;
                goto FETCH_LOOP;
            case 0x9f: // LAHF
                u = get_EFLAGS();
                set_lower_byte(4, u);
                goto FETCH_LOOP;
            case 0xf4: // HLT
                if (cpl != 0) {
                    abort(13);
                }
                halted = true;
                goto OUTER_LOOP;
            case 0xa4: // MOVSB
                aux_MOVSB();
                goto FETCH_LOOP;
            case 0xa5: // MOVSW/D
                ipr & 0x0100 ? aux_MOVSW() : aux_MOVSD();
                goto FETCH_LOOP;
            case 0xaa: // STOSB
                aux_STOSB();
                goto FETCH_LOOP;
            case 0xab: // STOSW/D
                ipr & 0x0100 ? aux_STOSW() : aux_STOSD();
                goto FETCH_LOOP;
            case 0xa6: // CMPSB
                aux_CMPSB();
                goto FETCH_LOOP;
            case 0xa7: // CMPSW/D
                ipr & 0x0100 ? aux_CMPSW() : aux_CMPSD();
                goto FETCH_LOOP;
            case 0xac: // LOSB
                aux_LODSB();
                goto FETCH_LOOP;
            case 0xad: // LOSW/D
                ipr & 0x0100 ? aux_LODSW() : aux_LODSD();
                goto FETCH_LOOP;
            case 0xae: // SCASB
                aux_SCASB();
                goto FETCH_LOOP;
            case 0xaf: // SCASW/D
                ipr & 0x0100 ? aux_SCASW() : aux_SCASD();
                goto FETCH_LOOP;
            case 0x6c: // INSB
                aux_INSB();
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto FETCH_LOOP;
            case 0x6d: // INSW/D
                ipr & 0x0100 ? aux_INSW() : aux_INSD();
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto FETCH_LOOP;
            case 0x6e: // OUTSB
                aux_OUTSB();
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto FETCH_LOOP;
            case 0x6f: // OUTSW/D
                ipr & 0x0100 ? aux_OUTSW() : aux_OUTSD();
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto FETCH_LOOP;
            case 0xd8: // ESC (80387) 11011XXX
            case 0xd9: // ESC (80387)
            case 0xda: // ESC (80387)
            case 0xdb: // ESC (80387)
            case 0xdc: // ESC (80387)
            case 0xdd: // ESC (80387)
            case 0xde: // ESC (80387)
            case 0xdf: // ESC (80387)
                if (cr0 & ((1 << 2) | (1 << 3))) {
                    abort(7);
                }
                modRM = fetch8_data();
                reg = (modRM >> 3) & 7;
                rM = modRM & 7;
                operation = ((opcode & 7) << 3) | ((modRM >> 3) & 7);
                set_lower_word(0, 0xffff);
                if ((modRM >> 6) == 3) {
                } else {
                    segment_translation();
                }
                goto FETCH_LOOP;
            case 0x9b: // FWAIT/WAIT
                goto FETCH_LOOP;
            case 0xe4: // IN AL,
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                imm = fetch8_data();
                set_lower_byte(0, io_read(imm));
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto FETCH_LOOP;
            case 0xe5: // IN AX,
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                imm = fetch8_data();
                regs[0] = io_read(imm);
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto FETCH_LOOP;
            case 0xe6: // OUT ,AL
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                imm = fetch8_data();
                io_write(imm, regs[0] & 0xff);
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto FETCH_LOOP;
            case 0xe7: // OUT ,AX
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                imm = fetch8_data();
                io_write(imm, regs[0]);
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto FETCH_LOOP;
            case 0xec: // IN AL,DX
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                set_lower_byte(0, io_read(regs[2] & 0xffff));
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto FETCH_LOOP;
            case 0xed: // IN AX,DX
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                regs[0] = io_read(regs[2] & 0xffff);
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto FETCH_LOOP;
            case 0xee: // OUT DX,AL
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                io_write(regs[2] & 0xffff, regs[0] & 0xff);
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto FETCH_LOOP;
            case 0xef: // OUT DX,AX
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                io_write(regs[2] & 0xffff, regs[0]);
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto FETCH_LOOP;
            case 0x27: // DAA
                aux_DAA();
                goto FETCH_LOOP;
            case 0x2f: // DAS
                aux_DAS();
                goto FETCH_LOOP;
            case 0x37: // AAA
                aux_AAA();
                goto FETCH_LOOP;
            case 0x3f: // AAS
                aux_AAS();
                goto FETCH_LOOP;
            case 0xd4: // AAM
                imm = fetch8_data();
                aux_AAM(imm);
                goto FETCH_LOOP;
            case 0xd5: // AAD
                imm = fetch8_data();
                aux_AAD(imm);
                goto FETCH_LOOP;
            case 0x63: // ARPL
                aux_ARPL();
                goto FETCH_LOOP;
            case 0xd6: // -
            case 0xf1: // -
                abort(6);
            case 0x0f: // 2-byte instruction escape
                opcode = fetch8_data();
                if (ipr & 0x0040) {
                    switch (opcode) {
                        case 0xa3: // BT
                        case 0xab: // BTS
                        case 0xb0: // CMPXCHG
                        case 0xb1: // CMPXCHG
                        case 0xb3: // BTR
                        case 0xba: // G8 (-, -, -, -, BT, BTS, BTR, BTC)
                        case 0xbb: // BTC
                        case 0xc0: // XADD
                        case 0xc1: // XADD
                            break;
                        default:
                            abort(6);
                    }
                }
                switch (opcode) {
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
                    imm = fetch_data();
                    if (can_jmp(opcode & 0xf)) {
                        far = far + imm;
                    }
                    goto FETCH_LOOP;
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
                    modRM = fetch8_data();
                    u = can_jmp(opcode & 0xf);
                    if ((modRM >> 6) == 3) {
                        set_lower_byte(modRM & 7, u);
                    } else {
                        segment_translation();
                        st8_writable_cpl3(u);
                    }
                    goto FETCH_LOOP;
                case 0x40: // CMOVx conditional move (80486) - overflow (OF == 1)
                case 0x41: // CMOVx   - not overflow (OF == 0)
                case 0x42: // CMOVx   - below/not above or equal/carry (CF == 1)
                case 0x43: // CMOVx   - not below/above or equal/not carry (CF == 0)
                case 0x44: // CMOVx   - zero/equal (ZF == 1)
                case 0x45: // CMOVx   - not zero/not equal (ZF == 0)
                case 0x46: // CMOVx   - below or equal/not above (CF == 1 OR ZF == 1)
                case 0x47: // CMOVx   - not below or equal/above (CF == 0 AND ZF == 0)
                case 0x48: // CMOVx   - sign (SF == 1)
                case 0x49: // CMOVx   - not sign (SF == 0)
                case 0x4a: // CMOVx   - parity/parity even (PF == 1)
                case 0x4b: // CMOVx   - not parity/parity odd (PF == 0)
                case 0x4c: // CMOVx   - less/not greater (SF != OF)
                case 0x4d: // CMOVx   - not less/greater or equal (SF == OF)
                case 0x4e: // CMOVx   - less or equal/not greater ((ZF == 1) OR (SF != OF))
                case 0x4f: // CMOVx   - not less nor equal/greater ((ZF == 0) AND (SF == OF))
                    modRM = fetch8_data();
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    if (can_jmp(opcode & 0xf)) {
                        regs[(modRM >> 3) & 7] = rm;
                    }
                    goto FETCH_LOOP;
                case 0xb6: // MOVZX
                    modRM = fetch8_data();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        rm = (regs[rM & 3] >> ((rM & 4) << 1)) & 0xff;
                    } else {
                        segment_translation();
                        rm = ld8_readonly_cpl3();
                    }
                    regs[reg] = rm;
                    goto FETCH_LOOP;
                case 0xb7: // MOVZX
                    modRM = fetch8_data();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7] & 0xffff;
                    } else {
                        segment_translation();
                        rm = ld16_readonly_cpl3();
                    }
                    regs[reg] = rm;
                    goto FETCH_LOOP;
                case 0xbe: // MOVSX
                    modRM = fetch8_data();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        rm = regs[rM & 3] >> ((rM & 4) << 1);
                    } else {
                        segment_translation();
                        rm = ld8_readonly_cpl3();
                    }
                    regs[reg] = sign_extend_byte(rm);
                    goto FETCH_LOOP;
                case 0xbf: // MOVSX
                    modRM = fetch8_data();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld16_readonly_cpl3();
                    }
                    regs[reg] = sign_extend_word(rm);
                    goto FETCH_LOOP;
                case 0x00: // G6 (SLDT, STR, LLDT, LTR, VERR, VERW, -)
                    if (!is_protected() || (eflags & 0x00020000)) {
                        abort(6);
                    }
                    modRM = fetch8_data();
                    operation = (modRM >> 3) & 7;
                    switch (operation) {
                    case 0: // SLDT
                    case 1: // STR
                        if (operation == 0) {
                            u = ldt.selector;
                        } else {
                            u = tr.selector;
                        }
                        if ((modRM >> 6) == 3) {
                            set_lower_word(modRM & 7, u);
                        } else {
                            segment_translation();
                            st16_writable_cpl3(u);
                        }
                        break;
                    case 2: // LDTR
                    case 3: // LTR
                        if (cpl != 0) {
                            abort(13);
                        }
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7] & 0xffff;
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        if (operation == 2) {
                            aux_LDTR(rm);
                        } else {
                            aux_LTR(rm);
                        }
                        break;
                    case 4: // VERR
                    case 5: // VERW
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7] & 0xffff;
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        aux_VERR_VERW(rm, operation & 1);
                        break;
                    default:
                        abort(6);
                    }
                    goto FETCH_LOOP;
                case 0x01: // G7 (SGDT, SIDT, LGDT, LIDT, SMSW, -, LMSW, -)
                    modRM = fetch8_data();
                    operation = (modRM >> 3) & 7;
                    switch (operation) {
                    case 2: // LGDT
                    case 3: // LIDT
                        if ((modRM >> 6) == 3) {
                            abort(6);
                        }
                        if (cpl != 0) {
                            abort(13);
                        }
                        segment_translation();
                        m16 = ld16_readonly_cpl3();
                        lax += 2;
                        m = ld_readonly_cpl3();
                        if (operation == 2) {
                            gdt.shadow.base = m;
                            gdt.shadow.limit = m16;
                        } else {
                            idt.shadow.base = m;
                            idt.shadow.limit = m16;
                        }
                        break;
                    case 7: // INVLPG (80486)
                        if (cpl != 0) {
                            abort(13);
                        }
                        if ((modRM >> 6) == 3) {
                            abort(6);
                        }
                        segment_translation();
                        tlb_flush_page(lax & 0xfffff000);
                        break;
                    default:
                        abort(6);
                    }
                    goto FETCH_LOOP;
                case 0x02: // LAR
                case 0x03: // LSL
                    aux_LAR_LSL((((ipr >> 8) & 1) ^ 1), opcode & 1);
                    goto FETCH_LOOP;
                case 0x20: // MOV
                    if (cpl != 0) {
                        abort(13);
                    }
                    modRM = fetch8_data();
                    if ((modRM >> 6) != 3) {
                        abort(6);
                    }
                    reg = (modRM >> 3) & 7;
                    switch (reg) {
                    case 0:
                        u = cr0;
                        break;
                    case 2:
                        u = cr2;
                        break;
                    case 3:
                        u = cr3;
                        break;
                    case 4:
                        u = cr4;
                        break;
                    default:
                        abort(6);
                    }
                    regs[modRM & 7] = u;
                    goto FETCH_LOOP;
                case 0x22: // MOV
                    if (cpl != 0) {
                        abort(13);
                    }
                    modRM = fetch8_data();
                    if ((modRM >> 6) != 3) {
                        abort(6);
                    }
                    reg = (modRM >> 3) & 7;
                    rm = regs[modRM & 7];
                    switch (reg) {
                    case 0:
                        set_CR0(rm);
                        break;
                    case 2:
                        cr2 = rm;
                        break;
                    case 3:
                        set_CR3(rm);
                        break;
                    case 4:
                        cr4 = rm;
                        break;
                    default:
                        abort(6);
                    }
                    goto FETCH_LOOP;
                case 0x06: // CLTS
                    if (cpl != 0) {
                        abort(13);
                    }
                    set_CR0(cr0 & ~(1u << 3));
                    goto FETCH_LOOP;
                case 0x23: // MOV
                    if (cpl != 0) {
                        abort(13);
                    }
                    modRM = fetch8_data();
                    if ((modRM >> 6) != 3) {
                        abort(6);
                    }
                    reg = (modRM >> 3) & 7;
                    rm = regs[modRM & 7];
                    if (reg == 4 || reg == 5) {
                        abort(6);
                    }
                    goto FETCH_LOOP;
                case 0xb2: // LSS
                case 0xb4: // LFS
                case 0xb5: // LGS
                    ld_far_pointer(opcode & 7);
                    goto FETCH_LOOP;
                case 0xa2: // CPUID (80486)
                    aux_CPUID();
                    goto FETCH_LOOP;
                case 0xa4: // SHLD
                    modRM = fetch8_data();
                    r = regs[(modRM >> 3) & 7];
                    if ((modRM >> 6) == 3) {
                        imm = fetch8_data();
                        rM = modRM & 7;
                        regs[rM] = aux_SHLD(regs[rM], r, imm);
                    } else {
                        segment_translation();
                        imm = fetch8_data();
                        rm = ld_writable_cpl3();
                        u = aux_SHLD(rm, r, imm);
                        st_writable_cpl3(u);
                    }
                    goto FETCH_LOOP;
                case 0xa5: // SHLD
                    modRM = fetch8_data();
                    r = regs[(modRM >> 3) & 7];
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        regs[rM] = aux_SHLD(regs[rM], r, regs[1]);
                    } else {
                        segment_translation();
                        rm = ld_writable_cpl3();
                        u = aux_SHLD(rm, r, regs[1]);
                        st_writable_cpl3(u);
                    }
                    goto FETCH_LOOP;
                case 0xac: // SHRD
                    modRM = fetch8_data();
                    r = regs[(modRM >> 3) & 7];
                    if ((modRM >> 6) == 3) {
                        imm = fetch8_data();
                        rM = modRM & 7;
                        regs[rM] = aux_SHRD(regs[rM], r, imm);
                    } else {
                        segment_translation();
                        imm = fetch8_data();
                        rm = ld_writable_cpl3();
                        u = aux_SHRD(rm, r, imm);
                        st_writable_cpl3(u);
                    }
                    goto FETCH_LOOP;
                case 0xad: // SHRD
                    modRM = fetch8_data();
                    r = regs[(modRM >> 3) & 7];
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        regs[rM] = aux_SHRD(regs[rM], r, regs[1]);
                    } else {
                        segment_translation();
                        rm = ld_writable_cpl3();
                        u = aux_SHRD(rm, r, regs[1]);
                        st_writable_cpl3(u);
                    }
                    goto FETCH_LOOP;
                case 0xba: // G8 (-, -, -, -, BT, BTS, BTR, BTC)
                    modRM = fetch8_data();
                    operation = (modRM >> 3) & 7;
                    switch (operation) {
                    case 4: // BT
                        if ((modRM >> 6) == 3) {
                            // LOCK prefix not allowed
                            rm = regs[modRM & 7];
                            imm = fetch8_data();
                        } else {
                            segment_translation();
                            imm = fetch8_data();
                            rm = ld_readonly_cpl3();
                        }
                        aux_BT(rm, imm);
                        break;
                    case 5: // BTS
                    case 6: // BTR
                    case 7: // BTC
                        operation &= 3;
                        if ((modRM >> 6) == 3) {
                            // LOCK prefix not allowed
                            rM = modRM & 7;
                            imm = fetch8_data();
                            regs[rM] = aux_BTS_BTR_BTC(regs[rM], imm);
                        } else {
                            segment_translation();
                            imm = fetch8_data();
                            rm = ld_writable_cpl3();
                            u = aux_BTS_BTR_BTC(rm, imm);
                            st_writable_cpl3(u);
                        }
                        break;
                    default:
                        abort(6);
                    }
                    goto FETCH_LOOP;
                case 0xa3: // BT
                    modRM = fetch8_data();
                    r = regs[(modRM >> 3) & 7];
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        lax = lax + ((r >> 5) << 2);
                        rm = ld_readonly_cpl3();
                    }
                    aux_BT(rm, r);
                    goto FETCH_LOOP;
                case 0xab: // BTS
                case 0xb3: // BTR
                case 0xbb: // BTC
                    modRM = fetch8_data();
                    r = regs[(modRM >> 3) & 7];
                    operation = (opcode >> 3) & 3;
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        regs[rM] = aux_BTS_BTR_BTC(regs[rM], r);
                    } else {
                        segment_translation();
                        lax = lax + ((r >> 5) << 2);
                        rm = ld_writable_cpl3();
                        u = aux_BTS_BTR_BTC(rm, r);
                        st_writable_cpl3(u);
                    }
                    goto FETCH_LOOP;
                case 0xbc: // BSF
                case 0xbd: // BSR
                    modRM = fetch8_data();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    if (opcode & 1) {
                        regs[reg] = aux_BSR(regs[reg], rm);
                    } else {
                        regs[reg] = aux_BSF(regs[reg], rm);
                    }
                    goto FETCH_LOOP;
                case 0xaf: // IMUL
                    modRM = fetch8_data();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    aux_IMUL(regs[reg], rm);
                    regs[reg] = u;
                    goto FETCH_LOOP;
                case 0x31: // RDTSC (80486)
                    if ((cr4 & (1 << 2)) && cpl != 0) {
                        abort(13);
                    }
                    {
                        uint64_t t = this->cycles + (cycles_requested - cycles_remaining);
                        regs[0] = static_cast<uint32_t>(t & 0xffffffff);
                        regs[2] = t >> 32;
                    }
                    goto FETCH_LOOP;
                case 0xc0: // XADD (80486)
                    operation = 0;
                    modRM = fetch8_data();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        r = regs[rM & 3] >> ((rM & 4) << 1);
                        u = calculate8(r, (regs[reg & 3] >> ((reg & 4) << 1)));
                        set_lower_byte(reg, r);
                        set_lower_byte(rM, u);
                    } else {
                        segment_translation();
                        rm = ld8_writable_cpl3();
                        u = calculate8(rm, (regs[reg & 3] >> ((reg & 4) << 1)));
                        st8_writable_cpl3(u);
                        set_lower_byte(reg, rm);
                    }
                    goto FETCH_LOOP;
                case 0xc1: // XADD (80486)
                    operation = 0;
                    modRM = fetch8_data();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        r = regs[rM];
                        u = calculate(r, regs[reg]);
                        regs[reg] = r;
                        regs[rM] = u;
                    } else {
                        segment_translation();
                        rm = ld_writable_cpl3();
                        u = calculate(rm, regs[reg]);
                        st_writable_cpl3(u);
                        regs[reg] = rm;
                    }
                    goto FETCH_LOOP;
                case 0xb0: // CMPXCHG (80486)
                    operation = 5;
                    modRM = fetch8_data();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        r = regs[rM & 3] >> ((rM & 4) << 1);
                        u = calculate8(regs[0], r);
                        if (u == 0) {
                            set_lower_byte(rM, (regs[reg & 3] >> ((reg & 4) << 1)));
                        } else {
                            set_lower_byte(0, r);
                        }
                    } else {
                        segment_translation();
                        rm = ld8_writable_cpl3();
                        u = calculate8(regs[0], rm);
                        if (u == 0) {
                            st8_writable_cpl3((regs[reg & 3] >> ((reg & 4) << 1)));
                        } else {
                            set_lower_byte(0, rm);
                        }
                    }
                    goto FETCH_LOOP;
                case 0xb1: // CMPXCHG (80486)
                    operation = 5;
                    modRM = fetch8_data();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        r = regs[rM];
                        u = calculate(regs[0], r);
                        if (u == 0) {
                            regs[rM] = regs[reg];
                        } else {
                            regs[0] = r;
                        }
                    } else {
                        segment_translation();
                        rm = ld_writable_cpl3();
                        u = calculate(regs[0], rm);
                        if (u == 0) {
                            st_writable_cpl3(regs[reg]);
                        } else {
                            regs[0] = rm;
                        }
                    }
                    goto FETCH_LOOP;
                case 0xa0: // PUSH FS
                case 0xa8: // PUSH GS
                    push(segs[(opcode >> 3) & 7].selector);
                    goto FETCH_LOOP;
                case 0xa1: // POP FS
                case 0xa9: // POP GS
                    m = pop() & 0xffff;
                    set_segment_register((opcode >> 3) & 7, m);
                    goto FETCH_LOOP;
                case 0xc8: // -
                case 0xc9: // -
                case 0xca: // -
                case 0xcb: // -
                case 0xcc: // -
                case 0xcd: // -
                case 0xce: // -
                case 0xcf: // BSWAP (80486)
                    reg = opcode & 7;
                    r = regs[reg];
                    regs[reg] = ((r >> 24) & 0xff) | ((r >> 8) & 0xff00) | ((r << 8) & 0xff0000) | (r << 24);
                    goto FETCH_LOOP;
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
                case 0xd0: // -
                case 0xd1: // -
                case 0xd2: // -
                case 0xd3: // -
                case 0xd4: // -
                case 0xd5: // -
                case 0xd6: // -
                case 0xd7: // -
                case 0xd8: // -
                case 0xd9: // -
                case 0xda: // -
                case 0xdb: // -
                case 0xdc: // -
                case 0xdd: // -
                case 0xde: // -
                case 0xdf: // -
                case 0xe0: // -
                case 0xe1: // -
                case 0xe2: // -
                case 0xe3: // -
                case 0xe4: // -
                case 0xe5: // -
                case 0xe6: // -
                case 0xe7: // -
                case 0xe8: // -
                case 0xe9: // -
                case 0xea: // -
                case 0xeb: // -
                case 0xec: // -
                case 0xed: // -
                case 0xee: // -
                case 0xef: // -
                case 0xf0: // -
                case 0xf1: // -
                case 0xf2: // -
                case 0xf3: // -
                case 0xf4: // -
                case 0xf5: // -
                case 0xf6: // -
                case 0xf7: // -
                case 0xf8: // -
                case 0xf9: // -
                case 0xfa: // -
                case 0xfb: // -
                case 0xfc: // -
                case 0xfd: // -
                case 0xfe: // -
                case 0xff: // -
                default:
                    abort(6);
                }
            default:
                if (ipr & 0x0040) {
                    switch (opcode) {
                        case 0x101: // ADD
                        case 0x109: // OR
                        case 0x111: // ADC
                        case 0x119: // SBB
                        case 0x121: // AND
                        case 0x129: // SUB
                        case 0x131: // XOR
                        case 0x181: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, -)
                        case 0x183: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, -)
                        case 0x187: // XCHG
                        case 0x1f7: // G3 (-, -, NOT, NEG, -, -, -, -)
                        case 0x1ff: // G5 (INC, DEC, -, -, -, -, -, -)
                        case 0x10f: // 2-byte instruction escape
                            break;
                        default:
                            abort(6);
                    }
                }
                switch (opcode) {
                case 0x189: // MOV
                    modRM = fetch8_data();
                    r = regs[(modRM >> 3) & 7];
                    if ((modRM >> 6) == 3) {
                        set_lower_word(modRM & 7, r);
                    } else {
                        segment_translation();
                        st16_writable_cpl3(r);
                    }
                    goto FETCH_LOOP;
                case 0x18b: // MOV
                    modRM = fetch8_data();
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld16_readonly_cpl3();
                    }
                    set_lower_word((modRM >> 3) & 7, rm);
                    goto FETCH_LOOP;
                case 0x1b8: // MOV A
                case 0x1b9: // MOV C
                case 0x1ba: // MOV D
                case 0x1bb: // MOV B
                case 0x1bc: // MOV SP
                case 0x1bd: // MOV BP
                case 0x1be: // MOV SI
                case 0x1bf: // MOV DI
                    set_lower_word(opcode & 7, fetch16_data());
                    goto FETCH_LOOP;
                case 0x1a1: // MOV AX,
                    ld_memory_offset(false);
                    moffs = ld16_readonly_cpl3();
                    set_lower_word(0, moffs);
                    goto FETCH_LOOP;
                case 0x1a3: // MOV ,AX
                    ld_memory_offset(true);
                    st16_writable_cpl3(regs[0]);
                    goto FETCH_LOOP;
                case 0x1c7: // MOV
                    modRM = fetch8_data();
                    if ((modRM >> 6) == 3) {
                        imm = fetch16_data();
                        set_lower_word(modRM & 7, imm);
                    } else {
                        segment_translation();
                        imm = fetch16_data();
                        st16_writable_cpl3(imm);
                    }
                    goto FETCH_LOOP;
                case 0x191: // XCHG C
                case 0x192: // XCHG D
                case 0x193: // XCHG B
                case 0x194: // XCHG SP
                case 0x195: // XCHG BP
                case 0x196: // XCHG SI
                case 0x197: // XCHG DI
                    reg = opcode & 7;
                    u = regs[0];
                    set_lower_word(0, regs[reg]);
                    set_lower_word(reg, u);
                    goto FETCH_LOOP;
                case 0x187: // XCHG
                    modRM = fetch8_data();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        rm = regs[rM];
                        set_lower_word(rM, regs[reg]);
                    } else {
                        segment_translation();
                        rm = ld16_writable_cpl3();
                        st16_writable_cpl3(regs[reg]);
                    }
                    set_lower_word(reg, rm);
                    goto FETCH_LOOP;
                case 0x1c4: // LES
                    ld_far_pointer16(0);
                    goto FETCH_LOOP;
                case 0x1c5: // LDS
                    ld_far_pointer16(3);
                    goto FETCH_LOOP;
                case 0x101: // ADD
                case 0x109: // OR
                case 0x111: // ADC
                case 0x119: // SBB
                case 0x121: // AND
                case 0x129: // SUB
                case 0x131: // XOR
                case 0x139: // CMP
                    modRM = fetch8_data();
                    operation = (opcode >> 3) & 7;
                    r = regs[(modRM >> 3) & 7];
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        set_lower_word(rM, calculate16(regs[rM], r));
                    } else {
                        segment_translation();
                        if (operation != 7) {
                            rm = ld16_writable_cpl3();
                            u = calculate16(rm, r);
                            st16_writable_cpl3(u);
                        } else {
                            rm = ld16_readonly_cpl3();
                            calculate16(rm, r);
                        }
                    }
                    goto FETCH_LOOP;
                case 0x103: // ADD
                case 0x10b: // OR
                case 0x113: // ADC
                case 0x11b: // SBB
                case 0x123: // AND
                case 0x12b: // SUB
                case 0x133: // XOR
                case 0x13b: // CMP
                    modRM = fetch8_data();
                    operation = (opcode >> 3) & 7;
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld16_readonly_cpl3();
                    }
                    set_lower_word(reg, calculate16(regs[reg], rm));
                    goto FETCH_LOOP;
                case 0x105: // ADD
                case 0x10d: // OR
                case 0x115: // ADC
                case 0x11d: // SBB
                case 0x125: // AND
                case 0x12d: // SUB
                case 0x135: // XOR
                case 0x13d: // CMP
                    imm = fetch16_data();
                    operation = (opcode >> 3) & 7;
                    set_lower_word(0, calculate16(regs[0], imm));
                    goto FETCH_LOOP;
                case 0x181: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                    modRM = fetch8_data();
                    operation = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        imm = fetch16_data();
                        set_lower_word(rM, calculate16(regs[rM], imm));
                    } else {
                        segment_translation();
                        imm = fetch16_data();
                        if (operation != 7) {
                            rm = ld16_writable_cpl3();
                            u = calculate16(rm, imm);
                            st16_writable_cpl3(u);
                        } else {
                            // LOCK prefix not allowed
                            rm = ld16_readonly_cpl3();
                            calculate16(rm, imm);
                        }
                    }
                    goto FETCH_LOOP;
                case 0x183: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                    modRM = fetch8_data();
                    operation = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        // LOCK prefix not allowed
                        rM = modRM & 7;
                        u = sign_extend_byte(fetch8_data());
                        set_lower_word(rM, calculate16(regs[rM], u));
                    } else {
                        segment_translation();
                        v = sign_extend_byte(fetch8_data());
                        if (operation != 7) {
                            rm = ld16_writable_cpl3();
                            u = calculate16(rm, v);
                            st16_writable_cpl3(u);
                        } else {
                            // LOCK prefix not allowed
                            rm = ld16_readonly_cpl3();
                            calculate16(rm, v);
                        }
                    }
                    goto FETCH_LOOP;
                case 0x140: // INC A
                case 0x141: // INC C
                case 0x142: // INC D
                case 0x143: // INC B
                case 0x144: // INC SP
                case 0x145: // INC BP
                case 0x146: // INC SI
                case 0x147: // INC DI
                    reg = opcode & 7;
                    set_lower_word(reg, aux16_INC(regs[reg]));
                    goto FETCH_LOOP;
                case 0x148: // DEC A
                case 0x149: // DEC C
                case 0x14a: // DEC D
                case 0x14b: // DEC B
                case 0x14c: // DEC SP
                case 0x14d: // DEC BP
                case 0x14e: // DEC SI
                case 0x14f: // DEC DI
                    reg = opcode & 7;
                    set_lower_word(reg, aux16_DEC(regs[reg]));
                    goto FETCH_LOOP;
                case 0x16b: // IMUL
                    modRM = fetch8_data();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld16_readonly_cpl3();
                    }
                    v = sign_extend_byte(fetch8_data());
                    aux16_IMUL(rm, v);
                    set_lower_word(reg, u);
                    goto FETCH_LOOP;
                case 0x169: // IMUL
                    modRM = fetch8_data();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld16_readonly_cpl3();
                    }
                    imm = fetch16_data();
                    aux16_IMUL(rm, imm);
                    set_lower_word(reg, u);
                    goto FETCH_LOOP;
                case 0x185: // TEST
                    modRM = fetch8_data();
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld16_readonly_cpl3();
                    }
                    r = regs[(modRM >> 3) & 7];
                    osm_dst = sign_extend_word(rm & r);
                    osm = 13;
                    goto FETCH_LOOP;
                case 0x1a9: // TEST
                    imm = fetch16_data();
                    osm_dst = sign_extend_word(regs[0] & imm);
                    osm = 13;
                    goto FETCH_LOOP;
                case 0x1f7: // G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
                    modRM = fetch8_data();
                    operation = (modRM >> 3) & 7;
                    // LOCK prefix not allowed if operation != 2 && operation != 3
                    switch (operation) {
                    case 0: // TEST
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        imm = fetch16_data();
                        osm_dst = sign_extend_word(rm & imm);
                        osm = 13;
                        break;
                    case 2: // NOT
                        if ((modRM >> 6) == 3) {
                            // LOCK prefix not allowed
                            rM = modRM & 7;
                            set_lower_word(rM, ~regs[rM]);
                        } else {
                            segment_translation();
                            rm = ld16_writable_cpl3();
                            st16_writable_cpl3(~rm);
                        }
                        break;
                    case 3: // NEG
                        operation = 5;
                        if ((modRM >> 6) == 3) {
                            // LOCK prefix not allowed
                            rM = modRM & 7;
                            set_lower_word(rM, calculate16(0, regs[rM]));
                        } else {
                            operation = 5;
                            segment_translation();
                            rm = ld16_writable_cpl3();
                            u = calculate16(0, rm);
                            st16_writable_cpl3(u);
                        }
                        break;
                    case 4: // MUL AL/X
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        aux16_MUL(regs[0], rm);
                        set_lower_word(0, u);
                        set_lower_word(2, u >> 16);
                        break;
                    case 5: // IMUL AL/X
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        aux16_IMUL(regs[0], rm);
                        set_lower_word(0, u);
                        set_lower_word(2, u >> 16);
                        break;
                    case 6: // DIV AL/X
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        aux16_DIV(rm);
                        break;
                    case 7: // IDIV AL/X
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        aux16_IDIV(rm);
                        break;
                    default:
                        abort(6);
                    }
                    goto FETCH_LOOP;
                case 0x1c1: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
                    modRM = fetch8_data();
                    operation = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        imm = fetch8_data();
                        rM = modRM & 7;
                        set_lower_word(rM, shift16(regs[rM], imm));
                    } else {
                        segment_translation();
                        imm = fetch8_data();
                        rm = ld16_writable_cpl3();
                        u = shift16(rm, imm);
                        st16_writable_cpl3(u);
                    }
                    goto FETCH_LOOP;
                case 0x1d1: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
                    modRM = fetch8_data();
                    operation = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        set_lower_word(rM, shift16(regs[rM], 1));
                    } else {
                        segment_translation();
                        rm = ld16_writable_cpl3();
                        u = shift16(rm, 1);
                        st16_writable_cpl3(u);
                    }
                    goto FETCH_LOOP;
                case 0x1d3: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
                    modRM = fetch8_data();
                    operation = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        set_lower_word(rM, shift16(regs[rM], regs[1] & 0xff));
                    } else {
                        segment_translation();
                        rm = ld16_writable_cpl3();
                        u = shift16(rm, regs[1] & 0xff);
                        st16_writable_cpl3(u);
                    }
                    goto FETCH_LOOP;
                case 0x198: // CBW
                    set_lower_word(0, sign_extend_byte(regs[0] & 0xff));
                    goto FETCH_LOOP;
                case 0x199: // CWD
                    set_lower_word(2, sign_shift_right(regs[0] << 16, 31));
                    goto FETCH_LOOP;
                case 0x190: // NOP
                    goto FETCH_LOOP;
                case 0x150: // PUSH A
                case 0x151: // PUSH C
                case 0x152: // PUSH D
                case 0x153: // PUSH B
                case 0x154: // PUSH SP
                case 0x155: // PUSH BP
                case 0x156: // PUSH SI
                case 0x157: // PUSH DI
                    push16(regs[opcode & 7]);
                    goto FETCH_LOOP;
                case 0x158: // POP A
                case 0x159: // POP C
                case 0x15a: // POP D
                case 0x15b: // POP B
                case 0x15c: // POP SP
                case 0x15d: // POP BP
                case 0x15e: // POP SI
                case 0x15f: // POP DI
                    m = pop16();
                    set_lower_word(opcode & 7, m);
                    goto FETCH_LOOP;
                case 0x160: // PUSHA
                    aux16_PUSHA();
                    goto FETCH_LOOP;
                case 0x161: // POPA
                    aux16_POPA();
                    goto FETCH_LOOP;
                case 0x18f: // POP
                    modRM = fetch8_data();
                    if ((modRM >> 6) == 3) {
                        m = pop16();
                        set_lower_word(modRM & 7, m);
                    } else {
                        u = regs[4];
                        m = pop16();
                        v = regs[4];
                        segment_translation();
                        regs[4] = u;
                        st16_writable_cpl3(m);
                        regs[4] = v;
                    }
                    goto FETCH_LOOP;
                case 0x168: // PUSH
                    imm = fetch16_data();
                    push16(imm);
                    goto FETCH_LOOP;
                case 0x16a: // PUSH
                    u = sign_extend_byte(fetch8_data());
                    push16(u);
                    goto FETCH_LOOP;
                case 0x1c8: // ENTER
                    aux16_ENTER();
                    goto FETCH_LOOP;
                case 0x1c9: // LEAVE
                    aux16_LEAVE();
                    goto FETCH_LOOP;
                case 0x106: // PUSH
                case 0x10e: // PUSH
                case 0x116: // PUSH
                case 0x11e: // PUSH
                    push16(segs[(opcode >> 3) & 3].selector);
                    goto FETCH_LOOP;
                case 0x107: // POP
                case 0x117: // POP
                case 0x11f: // POP
                    m = pop16();
                    set_segment_register((opcode >> 3) & 3, m);
                    goto FETCH_LOOP;
                case 0x18d: // LEA
                    modRM = fetch8_data();
                    if ((modRM >> 6) == 3) {
                        abort(6);
                    }
                    ipr = (ipr & ~0x000fu) | (6 + 1);
                    segment_translation();
                    set_lower_word((modRM >> 3) & 7, lax);
                    goto FETCH_LOOP;
                case 0x1ff: // G5 (INC, DEC, CALL, CALL, JMP, JMP, PUSH, -)
                    modRM = fetch8_data();
                    operation = (modRM >> 3) & 7;
                    // LOCK prefix not allowed if operation != 0 && operation != 1
                    switch (operation) {
                    case 0: // INC
                        if ((modRM >> 6) == 3) {
                            // LOCK prefix not allowed
                            rM = modRM & 7;
                            set_lower_word(rM, aux16_INC(regs[rM]));
                        } else {
                            segment_translation();
                            rm = ld16_writable_cpl3();
                            u = aux16_INC(rm);
                            st16_writable_cpl3(u);
                        }
                        break;
                    case 1: // DEC
                        if ((modRM >> 6) == 3) {
                            // LOCK prefix not allowed
                            rM = modRM & 7;
                            set_lower_word(rM, aux16_DEC(regs[rM]));
                        } else {
                            segment_translation();
                            rm = ld16_writable_cpl3();
                            u = aux16_DEC(rm);
                            st16_writable_cpl3(u);
                        }
                        break;
                    case 2: // CALL
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7] & 0xffff;
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        push16((eip + far - far_start));
                        eip = rm, far = far_start = 0;
                        break;
                    case 3: // CALL
                    case 5: // JMP
                        if ((modRM >> 6) == 3) {
                            abort(6);
                        }
                        segment_translation();
                        m = ld16_readonly_cpl3();
                        lax = lax + 2;
                        m16 = ld16_readonly_cpl3();
                        if (operation == 3) {
                            aux_CALLF(0, m16, m, (eip + far - far_start));
                        } else {
                            aux_JMPF(m16, m);
                        }
                        break;
                    case 6: // PUSH
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        push16(rm);
                        break;
                    case 4: // JMP
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7] & 0xffff;
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        eip = rm, far = far_start = 0;
                        break;
                    default:
                        abort(6);
                    }
                    goto FETCH_LOOP;
                case 0x1eb: // JMP
                    u = sign_extend_byte(fetch8_data());
                    eip = (eip + far - far_start + u) & 0xffff;
                    far = far_start = 0;
                    goto FETCH_LOOP;
                case 0x1e9: // JMP
                    imm = fetch16_data();
                    eip = (eip + far - far_start + imm) & 0xffff;
                    far = far_start = 0;
                    goto FETCH_LOOP;
                case 0x170: // JO
                case 0x171: // JNO
                case 0x172: // JB
                case 0x173: // JNB
                case 0x174: // JZ
                case 0x175: // JNZ
                case 0x176: // JBE
                case 0x177: // JNBE
                case 0x178: // JS
                case 0x179: // JNS
                case 0x17a: // JP
                case 0x17b: // JNP
                case 0x17c: // JL
                case 0x17d: // JNL
                case 0x17e: // JLE
                case 0x17f: // JNLE
                    u = sign_extend_byte(fetch8_data());
                    if (can_jmp(opcode & 0xf)) {
                        eip = (eip + far - far_start + u) & 0xffff;
                        far = far_start = 0;
                    }
                    goto FETCH_LOOP;
                case 0x1c2: // RET
                    u = sign_extend_word(fetch16_data());
                    m = ld16_stack();
                    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 2 + u) & SS_mask);
                    eip = m, far = far_start = 0;
                    goto FETCH_LOOP;
                case 0x1c3: // RET
                    m = pop16();
                    eip = m, far = far_start = 0;
                    goto FETCH_LOOP;
                case 0x1e8: // CALL
                    imm = fetch16_data();
                    push16((eip + far - far_start));
                    eip = (eip + far - far_start + imm) & 0xffff;
                    far = far_start = 0;
                    goto FETCH_LOOP;
                case 0x162: // BOUND
                    aux16_BOUND();
                    goto FETCH_LOOP;
                case 0x1a5: // MOVSW/D
                    aux16_MOVS();
                    goto FETCH_LOOP;
                case 0x1a7: // CMPSW/D
                    aux16_CMPS();
                    goto FETCH_LOOP;
                case 0x1ad: // LOSW/D
                    aux16_LODS();
                    goto FETCH_LOOP;
                case 0x1af: // SCASW/D
                    aux16_SCAS();
                    goto FETCH_LOOP;
                case 0x1ab: // STOSW/D
                    aux16_STOS();
                    goto FETCH_LOOP;
                case 0x16d: // INSW/D
                    aux16_INS();
                    if (get_irq() != 0 && (eflags & 0x00000200)) {
                        goto OUTER_LOOP;
                    }
                    goto FETCH_LOOP;
                case 0x16f: // OUTSW/D
                    aux16_OUTS();
                    if (get_irq() != 0 && (eflags & 0x00000200)) {
                        goto OUTER_LOOP;
                    }
                    goto FETCH_LOOP;
                case 0x1e5: // IN AX,
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl) {
                        abort(13);
                    }
                    imm = fetch8_data();
                    set_lower_word(0, io_read(imm));
                    if (get_irq() != 0 && (eflags & 0x00000200)) {
                        goto OUTER_LOOP;
                    }
                    goto FETCH_LOOP;
                case 0x1e7: // OUT ,AX
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl) {
                        abort(13);
                    }
                    imm = fetch8_data();
                    io_write(imm, regs[0] & 0xffff);
                    if (get_irq() != 0 && (eflags & 0x00000200)) {
                        goto OUTER_LOOP;
                    }
                    goto FETCH_LOOP;
                case 0x1ed: // IN AX,DX
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl) {
                        abort(13);
                    }
                    set_lower_word(0, io_read(regs[2] & 0xffff));
                    if (get_irq() != 0 && (eflags & 0x00000200)) {
                        goto OUTER_LOOP;
                    }
                    goto FETCH_LOOP;
                case 0x1ef: // OUT DX,AX
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl) {
                        abort(13);
                    }
                    io_write(regs[2] & 0xffff, regs[0] & 0xffff);
                    if (get_irq() != 0 && (eflags & 0x00000200)) {
                        goto OUTER_LOOP;
                    }
                    goto FETCH_LOOP;
                case 0x126: // ES segment override prefix
                case 0x12e: // CS segment override prefix
                case 0x136: // SS segment override prefix
                case 0x13e: // DS segment override prefix
                case 0x164: // FS segment override prefix
                case 0x165: // GS segment override prefix
                case 0x1f0: // LOCK prefix
                case 0x1f2: // REPN[EZ] repeat string operation prefix
                case 0x1f3: // REP[EZ] repeat string operation prefix
                case 0x166: // operand-size override prefix
                case 0x167: // address-size override prefix
                case 0x100: // ADD
                case 0x102: // ADD
                case 0x104: // ADD
                case 0x108: // OR
                case 0x10a: // OR
                case 0x10c: // OR
                case 0x110: // ADC
                case 0x112: // ADC
                case 0x114: // ADC
                case 0x118: // SBB
                case 0x11a: // SBB
                case 0x11c: // SBB
                case 0x120: // AND
                case 0x122: // AND
                case 0x124: // AND
                case 0x127: // DAA
                case 0x128: // SUB
                case 0x12a: // SUB
                case 0x12c: // SUB
                case 0x12f: // DAS
                case 0x130: // XOR
                case 0x132: // XOR
                case 0x134: // XOR
                case 0x137: // AAA
                case 0x138: // CMP
                case 0x13a: // CMP
                case 0x13c: // CMP
                case 0x13f: // AAS
                case 0x16c: // INSB
                case 0x16e: // OUTSB
                case 0x180: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                case 0x182: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                case 0x184: // TEST
                case 0x186: // XCHG
                case 0x188: // MOV
                case 0x18a: // MOV
                case 0x18c: // MOV
                case 0x18e: // MOV
                case 0x19a: // CALLF
                case 0x19b: // FWAIT/WAIT
                case 0x19c: // PUSHF
                case 0x19d: // POPF
                case 0x19e: // SAHF
                case 0x19f: // LAHF
                case 0x1a0: // MOV AL,
                case 0x1a2: // MOV ,AL
                case 0x1a4: // MOVSB
                case 0x1a6: // CMPSB
                case 0x1a8: // TEST
                case 0x1aa: // STOSB
                case 0x1ac: // LOSB
                case 0x1ae: // SCASB
                case 0x1b0: // MOV AL
                case 0x1b1: // MOV CL
                case 0x1b2: // MOV DL
                case 0x1b3: // MOV BL
                case 0x1b4: // MOV AH
                case 0x1b5: // MOV CH
                case 0x1b6: // MOV DH
                case 0x1b7: // MOV BH
                case 0x1c0: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
                case 0x1c6: // MOV
                case 0x1ca: // RET
                case 0x1cb: // RET
                case 0x1cc: // INT
                case 0x1cd: // INT
                case 0x1ce: // INTO
                case 0x1cf: // IRET
                case 0x1d0: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
                case 0x1d2: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
                case 0x1d4: // AAM
                case 0x1d5: // AAD
                case 0x1d7: // XLAT
                case 0x1d8: // ESC (80387)
                case 0x1d9: // ESC (80387)
                case 0x1da: // ESC (80387)
                case 0x1db: // ESC (80387)
                case 0x1dc: // ESC (80387)
                case 0x1dd: // ESC (80387)
                case 0x1de: // ESC (80387)
                case 0x1df: // ESC (80387)
                case 0x1e0: // LOOPNE
                case 0x1e1: // LOOPE
                case 0x1e2: // LOOP
                case 0x1e3: // JCXZ
                case 0x1e4: // IN AL,
                case 0x1e6: // OUT ,AL
                case 0x1ea: // JMPF
                case 0x1ec: // IN AL,DX
                case 0x1ee: // OUT DX,AL
                case 0x1f4: // HLT
                case 0x1f5: // CMC
                case 0x1f6: // G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
                case 0x1f8: // CLC
                case 0x1f9: // STC
                case 0x1fa: // CLI
                case 0x1fb: // STI
                case 0x1fc: // CLD
                case 0x1fd: // STD
                case 0x1fe: // G4 (INC, DEC, -, -, -, -, -, -)
                    opcode &= 0xff;
                    break;
                case 0x163: // ARPL
                case 0x1d6: // -
                case 0x1f1: // -
                case 0x10f: // 2-byte instruction escape
                    opcode = fetch8_data();
                    if (ipr & 0x0040) {
                        switch (opcode) {
                            case 0x1a3: // BT
                            case 0x1ab: // BTS
                            case 0x1b1: // CMPXCHG
                            case 0x1b3: // BTR
                            case 0x1ba: // G8 (-, -, -, -, BT, BTS, BTR, BTC)
                            case 0x1bb: // BTC
                            case 0x1c1: // XADD
                                break;
                            default:
                                abort(6);
                        }
                    }
                    opcode |= 0x0100;
                    switch (opcode) {
                    case 0x180: // JO
                    case 0x181: // JNO
                    case 0x182: // JB
                    case 0x183: // JNB
                    case 0x184: // JZ
                    case 0x185: // JNZ
                    case 0x186: // JBE
                    case 0x187: // JNBE
                    case 0x188: // JS
                    case 0x189: // JNS
                    case 0x18a: // JP
                    case 0x18b: // JNP
                    case 0x18c: // JL
                    case 0x18d: // JNL
                    case 0x18e: // JLE
                    case 0x18f: // JNLE
                        imm = fetch16_data();
                        if (can_jmp(opcode & 0xf)) {
                            eip = (eip + far - far_start + imm) & 0xffff;
                            far = far_start = 0;
                        }
                        goto FETCH_LOOP;
                    case 0x140: // CMOVx (80486)
                    case 0x141: // CMOVx (80486)
                    case 0x142: // CMOVx (80486)
                    case 0x143: // CMOVx (80486)
                    case 0x144: // CMOVx (80486)
                    case 0x145: // CMOVx (80486)
                    case 0x146: // CMOVx (80486)
                    case 0x147: // CMOVx (80486)
                    case 0x148: // CMOVx (80486)
                    case 0x149: // CMOVx (80486)
                    case 0x14a: // CMOVx (80486)
                    case 0x14b: // CMOVx (80486)
                    case 0x14c: // CMOVx (80486)
                    case 0x14d: // CMOVx (80486)
                    case 0x14e: // CMOVx (80486)
                    case 0x14f: // CMOVx (80486)
                        modRM = fetch8_data();
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        if (can_jmp(opcode & 0xf)) {
                            set_lower_word((modRM >> 3) & 7, rm);
                        }
                        goto FETCH_LOOP;
                    case 0x1b6: // MOVZX
                        modRM = fetch8_data();
                        reg = (modRM >> 3) & 7;
                        if ((modRM >> 6) == 3) {
                            rM = modRM & 7;
                            rm = (regs[rM & 3] >> ((rM & 4) << 1)) & 0xff;
                        } else {
                            segment_translation();
                            rm = ld8_readonly_cpl3();
                        }
                        set_lower_word(reg, rm);
                        goto FETCH_LOOP;
                    case 0x1be: // MOVSX
                        modRM = fetch8_data();
                        reg = (modRM >> 3) & 7;
                        if ((modRM >> 6) == 3) {
                            rM = modRM & 7;
                            rm = regs[rM & 3] >> ((rM & 4) << 1);
                        } else {
                            segment_translation();
                            rm = ld8_readonly_cpl3();
                        }
                        set_lower_word(reg, sign_extend_byte(rm));
                        goto FETCH_LOOP;
                    case 0x1af: // IMUL
                        modRM = fetch8_data();
                        reg = (modRM >> 3) & 7;
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        aux16_IMUL(regs[reg], rm);
                        set_lower_word(reg, u);
                        goto FETCH_LOOP;
                    case 0x1c1: // XADD (80486)
                        operation = 0;
                        modRM = fetch8_data();
                        reg = (modRM >> 3) & 7;
                        if ((modRM >> 6) == 3) {
                            // LOCK prefix not allowed
                            rM = modRM & 7;
                            r = regs[rM];
                            u = calculate16(r, regs[reg]);
                            set_lower_word(reg, r);
                            set_lower_word(rM, u);
                        } else {
                            segment_translation();
                            rm = ld16_writable_cpl3();
                            u = calculate16(rm, regs[reg]);
                            st16_writable_cpl3(u);
                            set_lower_word(reg, rm);
                        }
                        goto FETCH_LOOP;
                    case 0x1a0: // PUSH FS
                    case 0x1a8: // PUSH GS
                        push16(segs[(opcode >> 3) & 7].selector);
                        goto FETCH_LOOP;
                    case 0x1a1: // POP FS
                    case 0x1a9: // POP GS
                        m = pop16();
                        set_segment_register((opcode >> 3) & 7, m);
                        goto FETCH_LOOP;
                    case 0x1b2: // LSS
                    case 0x1b4: // LFS
                    case 0x1b5: // LGS
                        ld_far_pointer16(opcode & 7);
                        goto FETCH_LOOP;
                    case 0x1a4: // SHLD
                    case 0x1ac: // SHRD
                        modRM = fetch8_data();
                        r = regs[(modRM >> 3) & 7];
                        operation = (opcode >> 3) & 1;
                        if ((modRM >> 6) == 3) {
                            imm = fetch8_data();
                            rM = modRM & 7;
                            set_lower_word(rM, aux16_SHRD_SHLD(regs[rM], r, imm));
                        } else {
                            segment_translation();
                            imm = fetch8_data();
                            rm = ld16_writable_cpl3();
                            u = aux16_SHRD_SHLD(rm, r, imm);
                            st16_writable_cpl3(u);
                        }
                        goto FETCH_LOOP;
                    case 0x1a5: // SHLD
                    case 0x1ad: // SHRD
                        modRM = fetch8_data();
                        r = regs[(modRM >> 3) & 7];
                        operation = (opcode >> 3) & 1;
                        if ((modRM >> 6) == 3) {
                            rM = modRM & 7;
                            set_lower_word(rM, aux16_SHRD_SHLD(regs[rM], r, regs[1]));
                        } else {
                            segment_translation();
                            rm = ld16_writable_cpl3();
                            u = aux16_SHRD_SHLD(rm, r, regs[1]);
                            st16_writable_cpl3(u);
                        }
                        goto FETCH_LOOP;
                    case 0x1ba: // G8 (-, -, -, -, BT, BTS, BTR, BTC)
                        modRM = fetch8_data();
                        operation = (modRM >> 3) & 7;
                        switch (operation) {
                        case 4:
                            if ((modRM >> 6) == 3) {
                                // LOCK prefix not allowed
                                rm = regs[modRM & 7];
                                imm = fetch8_data();
                            } else {
                                segment_translation();
                                imm = fetch8_data();
                                rm = ld16_readonly_cpl3();
                            }
                            aux16_BT(rm, imm);
                            break;
                        case 5:
                        case 6:
                        case 7:
                            operation &= 3;
                            if ((modRM >> 6) == 3) {
                                // LOCK prefix not allowed
                                rM = modRM & 7;
                                imm = fetch8_data();
                                regs[rM] = aux16_BTS_BTR_BTC(regs[rM], imm);
                            } else {
                                segment_translation();
                                imm = fetch8_data();
                                rm = ld16_writable_cpl3();
                                u = aux16_BTS_BTR_BTC(rm, imm);
                                st16_writable_cpl3(u);
                            }
                            break;
                        default:
                            abort(6);
                        }
                        goto FETCH_LOOP;
                    case 0x1a3: // BT
                        modRM = fetch8_data();
                        r = regs[(modRM >> 3) & 7];
                        if ((modRM >> 6) == 3) {
                            // LOCK prefix not allowed
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            lax = lax + (((r & 0xffff) >> 4) << 1);
                            rm = ld16_readonly_cpl3();
                        }
                        aux16_BT(rm, r);
                        goto FETCH_LOOP;
                    case 0x1ab: // BTS
                    case 0x1b3: // BTR
                    case 0x1bb: // BTC
                        modRM = fetch8_data();
                        r = regs[(modRM >> 3) & 7];
                        operation = (opcode >> 3) & 3;
                        if ((modRM >> 6) == 3) {
                            // LOCK prefix not allowed
                            rM = modRM & 7;
                            set_lower_word(rM, aux16_BTS_BTR_BTC(regs[rM], r));
                        } else {
                            segment_translation();
                            lax = lax + (((r & 0xffff) >> 4) << 1);
                            rm = ld16_writable_cpl3();
                            u = aux16_BTS_BTR_BTC(rm, r);
                            st16_writable_cpl3(u);
                        }
                        goto FETCH_LOOP;
                    case 0x1bc: // BSF
                    case 0x1bd: // BSR
                        modRM = fetch8_data();
                        reg = (modRM >> 3) & 7;
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        r = regs[reg];
                        if (opcode & 1) {
                            u = aux16_BSR(r, rm);
                        } else {
                            u = aux16_BSF(r, rm);
                        }
                        set_lower_word(reg, u);
                        goto FETCH_LOOP;
                    case 0x1b1: // CMPXCHG (40486)
                        operation = 5;
                        modRM = fetch8_data();
                        reg = (modRM >> 3) & 7;
                        if ((modRM >> 6) == 3) {
                            // LOCK prefix not allowed
                            rM = modRM & 7;
                            r = regs[rM];
                            u = calculate16(regs[0], r);
                            if (u == 0) {
                                set_lower_word(rM, regs[reg]);
                            } else {
                                set_lower_word(0, r);
                            }
                        } else {
                            segment_translation();
                            rm = ld16_writable_cpl3();
                            u = calculate16(regs[0], rm);
                            if (u == 0) {
                                st16_writable_cpl3(regs[reg]);
                            } else {
                                set_lower_word(0, m);
                            }
                        }
                        goto FETCH_LOOP;
                    case 0x100: // G6 (SLDT, STR, LLDT, LTR, VERR, VERW, -)
                    case 0x101: // G7 (SGDT, SIDT, LGDT, LIDT, SMSW, -, LMSW, -)
                    case 0x102: // LAR
                    case 0x103: // LSL
                    case 0x106: // CLTS
                    case 0x120: // MOV
                    case 0x122: // MOV
                    case 0x123: // MOV
                    case 0x131: // -
                    case 0x190: // SETO
                    case 0x191: // SETNO
                    case 0x192: // SETB
                    case 0x193: // SETNB
                    case 0x194: // SETZ
                    case 0x195: // SETNZ
                    case 0x196: // SETBE
                    case 0x197: // SETNBE
                    case 0x198: // SETS
                    case 0x199: // SETNS
                    case 0x19a: // SETP
                    case 0x19b: // SETNP
                    case 0x19c: // SETL
                    case 0x19d: // SETNL
                    case 0x19e: // SETLE
                    case 0x19f: // SETNLE
                    case 0x1a2: // -
                    case 0x1b0: // CMPXCHG (80486)
                        opcode = 0x0f;
                        far--;
                        break;
                    case 0x104: // -
                    case 0x105: // -
                    case 0x107: // -
                    case 0x108: // -
                    case 0x109: // -
                    case 0x10a: // -
                    case 0x10b: // -
                    case 0x10c: // -
                    case 0x10d: // -
                    case 0x10e: // -
                    case 0x10f: // -
                    case 0x110: // -
                    case 0x111: // -
                    case 0x112: // -
                    case 0x113: // -
                    case 0x114: // -
                    case 0x115: // -
                    case 0x116: // -
                    case 0x117: // -
                    case 0x118: // -
                    case 0x119: // -
                    case 0x11a: // -
                    case 0x11b: // -
                    case 0x11c: // -
                    case 0x11d: // -
                    case 0x11e: // -
                    case 0x11f: // -
                    case 0x121: // MOV
                    case 0x124: // MOV
                    case 0x125: // -
                    case 0x126: // MOV
                    case 0x127: // -
                    case 0x128: // -
                    case 0x129: // -
                    case 0x12a: // -
                    case 0x12b: // -
                    case 0x12c: // -
                    case 0x12d: // -
                    case 0x12e: // -
                    case 0x12f: // -
                    case 0x130: // -
                    case 0x132: // -
                    case 0x133: // -
                    case 0x134: // -
                    case 0x135: // -
                    case 0x136: // -
                    case 0x137: // -
                    case 0x138: // -
                    case 0x139: // -
                    case 0x13a: // -
                    case 0x13b: // -
                    case 0x13c: // -
                    case 0x13d: // -
                    case 0x13e: // -
                    case 0x13f: // -
                    case 0x150: // -
                    case 0x151: // -
                    case 0x152: // -
                    case 0x153: // -
                    case 0x154: // -
                    case 0x155: // -
                    case 0x156: // -
                    case 0x157: // -
                    case 0x158: // -
                    case 0x159: // -
                    case 0x15a: // -
                    case 0x15b: // -
                    case 0x15c: // -
                    case 0x15d: // -
                    case 0x15e: // -
                    case 0x15f: // -
                    case 0x160: // -
                    case 0x161: // -
                    case 0x162: // -
                    case 0x163: // -
                    case 0x164: // -
                    case 0x165: // -
                    case 0x166: // -
                    case 0x167: // -
                    case 0x168: // -
                    case 0x169: // -
                    case 0x16a: // -
                    case 0x16b: // -
                    case 0x16c: // -
                    case 0x16d: // -
                    case 0x16e: // -
                    case 0x16f: // -
                    case 0x170: // -
                    case 0x171: // -
                    case 0x172: // -
                    case 0x173: // -
                    case 0x174: // -
                    case 0x175: // -
                    case 0x176: // -
                    case 0x177: // -
                    case 0x178: // -
                    case 0x179: // -
                    case 0x17a: // -
                    case 0x17b: // -
                    case 0x17c: // -
                    case 0x17d: // -
                    case 0x17e: // -
                    case 0x17f: // -
                    case 0x1a6: // -
                    case 0x1a7: // -
                    case 0x1aa: // -
                    case 0x1ae: // -
                    case 0x1b7: // MOVZX
                    case 0x1b8: // -
                    case 0x1b9: // -
                    case 0x1bf: // MOVSX
                    case 0x1c0: // -
                    default:
                        abort(6);
                    }
                    break;
                default:
                    abort(6);
                }
            }
        }
    FETCH_LOOP:
        ;
    } while (--cycles_remaining);
OUTER_LOOP:
    this->cycles += cycles_requested - cycles_remaining;
    eip = eip + far - far_start;
}
