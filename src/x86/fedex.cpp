#include "free86.h"

void Free86::fetch_decode_execute(uint64_t cycles) {
    int sreg, hL; // H (0x80) or L (0x00) byte selector
    if (halted) {
        if (get_irq() != 0 && (eflags & 0x00000200)) {
            halted = 0;
        } else {
            return;
        }
    }
    cycles_requested = cycles;
    cycles_remaining = cycles;
    far = far_start = 0;
    update_SSB(); // segments state block
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
        while (true) { // loop over instruction bytes
            switch (opcode) {
            case 0x66: // operand-size override prefix
                if (ipr == ipr_default) {
                    instruction_length(opcode);
                }
                if (ipr_default & 0x0100) {
                    ipr &= ~0x0100;
                } else {
                    ipr |= 0x0100;
                }
                opcode = ld8_direct();
                opcode |= ipr & 0x0100;
                break;
            case 0x67: // address-size override prefix
                if (ipr == ipr_default) {
                    instruction_length(opcode);
                }
                if (ipr_default & 0x0080) {
                    ipr &= ~0x0080;
                } else {
                    ipr |= 0x0080;
                }
                opcode = ld8_direct();
                opcode |= ipr & 0x0100;
                break;
            case 0xf0: // LOCK prefix
                if (ipr == ipr_default) {
                    instruction_length(opcode);
                }
                ipr |= 0x0040;
                opcode = ld8_direct();
                opcode |= ipr & 0x0100;
                break;
            case 0xf2: // REPN[EZ] repeat string operation prefix
                if (ipr == ipr_default) {
                    instruction_length(opcode);
                }
                ipr |= 0x0020;
                opcode = ld8_direct();
                opcode |= ipr & 0x0100;
                break;
            case 0xf3: // REP[EZ] repeat string operation prefix
                if (ipr == ipr_default) {
                    instruction_length(opcode);
                }
                ipr |= 0x0010;
                opcode = ld8_direct();
                opcode |= ipr & 0x0100;
                break;
            case 0x26: // ES segment override prefix
            case 0x2e: // CS segment override prefix
            case 0x36: // SS segment override prefix
            case 0x3e: // DS segment override prefix
                if (ipr == ipr_default) {
                    instruction_length(opcode);
                }
                ipr = (ipr & ~0x000f) | (((opcode >> 3) & 3) + 1);
                opcode = ld8_direct();
                opcode |= ipr & 0x0100;
                break;
            case 0x64: // FS segment override prefix
            case 0x65: // GS segment override prefix
                if (ipr == ipr_default) {
                    instruction_length(opcode);
                }
                ipr = (ipr & ~0x000f) | ((opcode & 7) + 1);
                opcode = ld8_direct();
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
                x = ld8_direct();
                opcode &= 7;
                hL = (opcode & 4) << 1;
                regs[opcode & 3] = (regs[opcode & 3] & ~(0xff << hL)) | ((x & 0xff) << hL);
                goto EXEC_LOOP;
            case 0xb8: // MOV A
            case 0xb9: // MOV C
            case 0xba: // MOV D
            case 0xbb: // MOV B
            case 0xbc: // MOV SP
            case 0xbd: // MOV BP
            case 0xbe: // MOV SI
            case 0xbf: // MOV DI
                x = ld_direct();
                regs[opcode & 7] = x;
                goto EXEC_LOOP;
            case 0x88: // MOV
                modRM = ld8_direct();
                reg = (modRM >> 3) & 7;
                x = regs[reg & 3] >> ((reg & 4) << 1);
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    hL = (rM & 4) << 1;
                    regs[rM & 3] = (regs[rM & 3] & ~(0xff << hL)) | ((x & 0xff) << hL);
                } else {
                    segment_translation();
                    st8_writable_cpl3(x);
                }
                goto EXEC_LOOP;
            case 0x89: // MOV
                modRM = ld8_direct();
                x = regs[(modRM >> 3) & 7];
                if ((modRM >> 6) == 3) {
                    regs[modRM & 7] = x;
                } else {
                    segment_translation();
                    st_writable_cpl3(x);
                }
                goto EXEC_LOOP;
            case 0x8a: // MOV
                modRM = ld8_direct();
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    x = regs[rM & 3] >> ((rM & 4) << 1);
                } else {
                    segment_translation();
                    x = ld8_readonly_cpl3();
                }
                reg = (modRM >> 3) & 7;
                hL = (reg & 4) << 1;
                regs[reg & 3] = (regs[reg & 3] & ~(0xff << hL)) | ((x & 0xff) << hL);
                goto EXEC_LOOP;
            case 0x8b: // MOV
                modRM = ld8_direct();
                if ((modRM >> 6) == 3) {
                    rm = regs[modRM & 7];
                } else {
                    segment_translation();
                    rm = ld_readonly_cpl3();
                }
                regs[(modRM >> 3) & 7] = rm;
                goto EXEC_LOOP;
            case 0xa0: // MOV AL,
                offset_to_linear(false);
                x = ld8_readonly_cpl3();
                regs[0] = (regs[0] & -256) | x;
                goto EXEC_LOOP;
            case 0xa1: // MOV AX,
                offset_to_linear(false);
                x = ld_readonly_cpl3();
                regs[0] = x;
                goto EXEC_LOOP;
            case 0xa2: // MOV ,AL
                offset_to_linear(true);
                st8_writable_cpl3(regs[0]);
                goto EXEC_LOOP;
            case 0xa3: // MOV ,AX
                if (ipr & 0x0040) { // test386, check LOCK prefix (not allowed)
                    abort(6);
                }
                offset_to_linear(true);
                st_writable_cpl3(regs[0]);
                goto EXEC_LOOP;
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
                lax = lax + segs[sreg].base;
                x = ld8_readonly_cpl3();
                set_lower_byte(0, x);
                goto EXEC_LOOP;
            case 0xc6: // MOV
                modRM = ld8_direct();
                if ((modRM >> 6) == 3) {
                    x = ld8_direct();
                    set_lower_byte(modRM & 7, x);
                } else {
                    segment_translation();
                    x = ld8_direct();
                    st8_writable_cpl3(x);
                }
                goto EXEC_LOOP;
            case 0xc7: // MOV
                modRM = ld8_direct();
                if ((modRM >> 6) == 3) {
                    x = ld_direct();
                    regs[modRM & 7] = x;
                } else {
                    segment_translation();
                    x = ld_direct();
                    st_writable_cpl3(x);
                }
                goto EXEC_LOOP;
            case 0x91: // XCHG C
            case 0x92: // XCHG D
            case 0x93: // XCHG B
            case 0x94: // XCHG SP
            case 0x95: // XCHG BP
            case 0x96: // XCHG SI
            case 0x97: // XCHG DI
                reg = opcode & 7;
                x = regs[0];
                regs[0] = regs[reg];
                regs[reg] = x;
                goto EXEC_LOOP;
            case 0x86: // XCHG
                modRM = ld8_direct();
                reg = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    x = regs[rM & 3] >> ((rM & 4) << 1);
                    set_lower_byte(rM, (regs[reg & 3] >> ((reg & 4) << 1)));
                } else {
                    segment_translation();
                    x = ld8_writable_cpl3();
                    st8_writable_cpl3((regs[reg & 3] >> ((reg & 4) << 1)));
                }
                set_lower_byte(reg, x);
                goto EXEC_LOOP;
            case 0x87: // XCHG
                modRM = ld8_direct();
                reg = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    x = regs[rM];
                    regs[rM] = regs[reg];
                } else {
                    segment_translation();
                    x = ld_writable_cpl3();
                    st_writable_cpl3(regs[reg]);
                }
                regs[reg] = x;
                goto EXEC_LOOP;
            case 0x8e: // MOV
                modRM = ld8_direct();
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
                goto EXEC_LOOP;
            case 0x8c: // MOV
                modRM = ld8_direct();
                reg = (modRM >> 3) & 7;
                if (reg >= 6) {
                    abort(6);
                }
                x = segs[reg].selector;
                if ((modRM >> 6) == 3) {
                    if (((ipr >> 8) & 1) ^ 1) {
                        regs[modRM & 7] = x;
                    } else {
                        set_lower_word(modRM & 7, x);
                    }
                } else {
                    segment_translation();
                    st16_writable_cpl3(x);
                }
                goto EXEC_LOOP;
            case 0xc4: // LES
                ld_far_pointer(0);
                goto EXEC_LOOP;
            case 0xc5: // LDS
                ld_far_pointer(3);
                goto EXEC_LOOP;
            case 0x00: // ADD
            case 0x08: // OR
            case 0x10: // ADC
            case 0x18: // SBB
            case 0x20: // AND
            case 0x28: // SUB
            case 0x30: // XOR
            case 0x38: // CMP
                modRM = ld8_direct();
                operation = opcode >> 3;
                reg = (modRM >> 3) & 7;
                y = regs[reg & 3] >> ((reg & 4) << 1);
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    set_lower_byte(rM, calculate8((regs[rM & 3] >> ((rM & 4) << 1)), y));
                } else {
                    segment_translation();
                    if (operation != 7) {
                        x = ld8_writable_cpl3();
                        x = calculate8(x, y);
                        st8_writable_cpl3(x);
                    } else {
                        x = ld8_readonly_cpl3();
                        calculate8(x, y);
                    }
                }
                goto EXEC_LOOP;
            case 0x01: // ADD
                modRM = ld8_direct();
                y = regs[(modRM >> 3) & 7];
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    osm_src = y;
                    osm_dst = regs[rM] = regs[rM] + osm_src;
                    osm = 2;
                } else {
                    segment_translation();
                    x = ld_writable_cpl3();
                    osm_src = y;
                    osm_dst = x = x + osm_src;
                    osm = 2;
                    st_writable_cpl3(x);
                }
                goto EXEC_LOOP;
            case 0x09: // OR
            case 0x11: // ADC
            case 0x19: // SBB
            case 0x21: // AND
            case 0x29: // SUB
            case 0x31: // XOR
                modRM = ld8_direct();
                operation = opcode >> 3;
                y = regs[(modRM >> 3) & 7];
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    regs[rM] = calculate(regs[rM], y);
                } else {
                    segment_translation();
                    x = ld_writable_cpl3();
                    x = calculate(x, y);
                    st_writable_cpl3(x);
                }
                goto EXEC_LOOP;
            case 0x39: // CMP
                modRM = ld8_direct();
                operation = opcode >> 3;
                y = regs[(modRM >> 3) & 7];
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    osm_src = y;
                    osm_dst = regs[rM] - osm_src;
                    osm = 8;
                } else {
                    segment_translation();
                    x = ld_readonly_cpl3();
                    osm_src = y;
                    osm_dst = x - osm_src;
                    osm = 8;
                }
                goto EXEC_LOOP;
            case 0x02: // ADD
            case 0x0a: // OR
            case 0x12: // ADC
            case 0x1a: // SBB
            case 0x22: // AND
            case 0x2a: // SUB
            case 0x32: // XOR
            case 0x3a: // CMP
                modRM = ld8_direct();
                operation = opcode >> 3;
                reg = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    y = regs[rM & 3] >> ((rM & 4) << 1);
                } else {
                    segment_translation();
                    y = ld8_readonly_cpl3();
                }
                set_lower_byte(reg, calculate8((regs[reg & 3] >> ((reg & 4) << 1)), y));
                goto EXEC_LOOP;
            case 0x03: // ADD
                modRM = ld8_direct();
                reg = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rm = regs[modRM & 7];
                } else {
                    segment_translation();
                    rm = ld_readonly_cpl3();
                }
                osm_src = rm;
                osm_dst = regs[reg] = regs[reg] + osm_src;
                osm = 2;
                goto EXEC_LOOP;
            case 0x0b: // OR
            case 0x13: // ADC
            case 0x1b: // SBB
            case 0x23: // AND
            case 0x2b: // SUB
            case 0x33: // XOR
                modRM = ld8_direct();
                operation = opcode >> 3;
                reg = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rm = regs[modRM & 7];
                } else {
                    segment_translation();
                    rm = ld_readonly_cpl3();
                }
                regs[reg] = calculate(regs[reg], rm);
                goto EXEC_LOOP;
            case 0x3b: // CMP
                modRM = ld8_direct();
                operation = opcode >> 3;
                reg = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rm = regs[modRM & 7];
                } else {
                    segment_translation();
                    rm = ld_readonly_cpl3();
                }
                osm_src = rm;
                osm_dst = regs[reg] - osm_src;
                osm = 8;
                goto EXEC_LOOP;
            case 0x04: // ADD
            case 0x0c: // OR
            case 0x14: // ADC
            case 0x1c: // SBB
            case 0x24: // AND
            case 0x2c: // SUB
            case 0x34: // XOR
            case 0x3c: // CMP
                y = ld8_direct();
                operation = opcode >> 3;
                set_lower_byte(0, calculate8(regs[0] & 0xff, y));
                goto EXEC_LOOP;
            case 0x05: // ADD
                y = ld_direct();
                osm_src = y;
                osm_dst = regs[0] = regs[0] + osm_src;
                osm = 2;
                goto EXEC_LOOP;
            case 0x0d: // OR
            case 0x15: // ADC
            case 0x1d: // SBB
            case 0x25: // AND
            case 0x2d: // SUB
                y = ld_direct();
                operation = opcode >> 3;
                regs[0] = calculate(regs[0], y);
                goto EXEC_LOOP;
            case 0x35: // XOR
                y = ld_direct();
                osm_dst = regs[0] = regs[0] ^ y;
                osm = 14;
                goto EXEC_LOOP;
            case 0x3d: // CMP
                y = ld_direct();
                osm_src = y;
                osm_dst = regs[0] - osm_src;
                osm = 8;
                goto EXEC_LOOP;
            case 0x80: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
            case 0x82: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                modRM = ld8_direct();
                operation = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    y = ld8_direct();
                    set_lower_byte(rM, calculate8((regs[rM & 3] >> ((rM & 4) << 1)), y));
                } else {
                    segment_translation();
                    y = ld8_direct();
                    if (operation != 7) {
                        x = ld8_writable_cpl3();
                        x = calculate8(x, y);
                        st8_writable_cpl3(x);
                    } else {
                        x = ld8_readonly_cpl3();
                        calculate8(x, y);
                    }
                }
                goto EXEC_LOOP;
            case 0x81: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                modRM = ld8_direct();
                operation = (modRM >> 3) & 7;
                if (operation == 7) {
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    y = ld_direct();
                    osm_src = y;
                    osm_dst = rm - osm_src;
                    osm = 8;
                } else {
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        y = ld_direct();
                        regs[rM] = calculate(regs[rM], y);
                    } else {
                        segment_translation();
                        y = ld_direct();
                        x = ld_writable_cpl3();
                        x = calculate(x, y);
                        st_writable_cpl3(x);
                    }
                }
                goto EXEC_LOOP;
            case 0x83: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                modRM = ld8_direct();
                operation = (modRM >> 3) & 7;
                if (operation == 7) {
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    y = (ld8_direct() << 24) >> 24;
                    osm_src = y;
                    osm_dst = rm - osm_src;
                    osm = 8;
                } else {
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        y = (ld8_direct() << 24) >> 24;
                        regs[rM] = calculate(regs[rM], y);
                    } else {
                        segment_translation();
                        y = (ld8_direct() << 24) >> 24;
                        x = ld_writable_cpl3();
                        x = calculate(x, y);
                        st_writable_cpl3(x);
                    }
                }
                goto EXEC_LOOP;
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
                    ocm_preserved = osm;
                    ocm_dst_preserved = osm_dst;
                }
                regs[reg] = osm_dst = regs[reg] + 1;
                osm = 27;
                goto EXEC_LOOP;
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
                    ocm_preserved = osm;
                    ocm_dst_preserved = osm_dst;
                }
                regs[reg] = osm_dst = regs[reg] - 1;
                osm = 30;
                goto EXEC_LOOP;
            case 0x6b: // IMUL
                modRM = ld8_direct();
                reg = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rm = regs[modRM & 7];
                } else {
                    segment_translation();
                    rm = ld_readonly_cpl3();
                }
                z = (ld8_direct() << 24) >> 24;
                aux_IMUL(rm, z);
                regs[reg] = x;
                goto EXEC_LOOP;
            case 0x69: // IMUL
                modRM = ld8_direct();
                reg = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rm = regs[modRM & 7];
                } else {
                    segment_translation();
                    rm = ld_readonly_cpl3();
                }
                z = ld_direct();
                aux_IMUL(rm, z);
                regs[reg] = x;
                goto EXEC_LOOP;
            case 0x84: // TEST
                modRM = ld8_direct();
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    x = regs[rM & 3] >> ((rM & 4) << 1);
                } else {
                    segment_translation();
                    x = ld8_readonly_cpl3();
                }
                reg = (modRM >> 3) & 7;
                y = regs[reg & 3] >> ((reg & 4) << 1);
                osm_dst = ((x & y) << 24) >> 24;
                osm = 12;
                goto EXEC_LOOP;
            case 0x85: // TEST
                modRM = ld8_direct();
                if ((modRM >> 6) == 3) {
                    rm = regs[modRM & 7];
                } else {
                    segment_translation();
                    rm = ld_readonly_cpl3();
                }
                y = regs[(modRM >> 3) & 7];
                osm_dst = rm & y;
                osm = 14;
                goto EXEC_LOOP;
            case 0xa8: // TEST
                y = ld8_direct();
                osm_dst = ((regs[0] & y) << 24) >> 24;
                osm = 12;
                goto EXEC_LOOP;
            case 0xa9: // TEST
                y = ld_direct();
                osm_dst = regs[0] & y;
                osm = 14;
                goto EXEC_LOOP;
            case 0xf6: // G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, // IDIV AL/X)
                modRM = ld8_direct();
                operation = (modRM >> 3) & 7;
                switch (operation) {
                case 0:
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        x = regs[rM & 3] >> ((rM & 4) << 1);
                    } else {
                        segment_translation();
                        x = ld8_readonly_cpl3();
                    }
                    y = ld8_direct();
                    osm_dst = ((x & y) << 24) >> 24;
                    osm = 12;
                    break;
                case 2:
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        set_lower_byte(rM, ~(regs[rM & 3] >> ((rM & 4) << 1)));
                    } else {
                        segment_translation();
                        x = ld8_writable_cpl3();
                        x = ~x;
                        st8_writable_cpl3(x);
                    }
                    break;
                case 3:
                    operation = 5;
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        set_lower_byte(rM, calculate8(0, (regs[rM & 3] >> ((rM & 4) << 1))));
                    } else {
                        segment_translation();
                        x = ld8_writable_cpl3();
                        x = calculate8(0, x);
                        st8_writable_cpl3(x);
                    }
                    break;
                case 4:
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        x = regs[rM & 3] >> ((rM & 4) << 1);
                    } else {
                        segment_translation();
                        x = ld8_readonly_cpl3();
                    }
                    aux_MUL8(regs[0], x);
                    set_lower_word(0, x);
                    break;
                case 5:
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        x = regs[rM & 3] >> ((rM & 4) << 1);
                    } else {
                        segment_translation();
                        x = ld8_readonly_cpl3();
                    }
                    aux_IMUL8(regs[0], x);
                    set_lower_word(0, x);
                    break;
                case 6:
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        x = regs[rM & 3] >> ((rM & 4) << 1);
                    } else {
                        segment_translation();
                        x = ld8_readonly_cpl3();
                    }
                    aux_DIV8(x);
                    break;
                case 7:
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        x = regs[rM & 3] >> ((rM & 4) << 1);
                    } else {
                        segment_translation();
                        x = ld8_readonly_cpl3();
                    }
                    aux_IDIV8(x);
                    break;
                default:
                    abort(6);
                }
                goto EXEC_LOOP;
            case 0xf7: // G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
                modRM = ld8_direct();
                operation = (modRM >> 3) & 7;
                switch (operation) {
                case 0:
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    y = ld_direct();
                    osm_dst = rm & y;
                    osm = 14;
                    break;
                case 2:
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        regs[rM] = ~regs[rM];
                    } else {
                        segment_translation();
                        x = ld_writable_cpl3();
                        x = ~x;
                        st_writable_cpl3(x);
                    }
                    break;
                case 3:
                    operation = 5;
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        regs[rM] = calculate(0, regs[rM]);
                    } else {
                        segment_translation();
                        x = ld_writable_cpl3();
                        x = calculate(0, x);
                        st_writable_cpl3(x);
                    }
                    break;
                case 4:
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    aux_MUL(regs[0], rm);
                    regs[0] = rm;
                    regs[2] = v;
                    break;
                case 5:
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    aux_IMUL(regs[0], rm);
                    regs[0] = rm;
                    regs[2] = v;
                    break;
                case 6:
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    regs[0] = aux_DIV(regs[2], regs[0], rm);
                    regs[2] = v;
                    break;
                case 7:
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    regs[0] = aux_IDIV(regs[2], regs[0], rm);
                    regs[2] = v;
                    break;
                default:
                    abort(6);
                }
                goto EXEC_LOOP;
            case 0xc0: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
                modRM = ld8_direct();
                operation = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    y = ld8_direct();
                    rM = modRM & 7;
                    set_lower_byte(rM, shift8((regs[rM & 3] >> ((rM & 4) << 1)), y));
                } else {
                    segment_translation();
                    y = ld8_direct();
                    x = ld8_writable_cpl3();
                    x = shift8(x, y);
                    st8_writable_cpl3(x);
                }
                goto EXEC_LOOP;
            case 0xc1: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
                modRM = ld8_direct();
                operation = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    y = ld8_direct();
                    rM = modRM & 7;
                    regs[rM] = shift(regs[rM], y);
                } else {
                    segment_translation();
                    y = ld8_direct();
                    x = ld_writable_cpl3();
                    x = shift(x, y);
                    st_writable_cpl3(x);
                }
                goto EXEC_LOOP;
            case 0xd0: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
                modRM = ld8_direct();
                operation = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    set_lower_byte(rM, shift8((regs[rM & 3] >> ((rM & 4) << 1)), 1));
                } else {
                    segment_translation();
                    x = ld8_writable_cpl3();
                    x = shift8(x, 1);
                    st8_writable_cpl3(x);
                }
                goto EXEC_LOOP;
            case 0xd1: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
                modRM = ld8_direct();
                operation = (modRM >> 3) & 7;
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    regs[rM] = shift(regs[rM], 1);
                } else {
                    segment_translation();
                    x = ld_writable_cpl3();
                    x = shift(x, 1);
                    st_writable_cpl3(x);
                }
                goto EXEC_LOOP;
            case 0xd2: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
                modRM = ld8_direct();
                operation = (modRM >> 3) & 7;
                y = regs[1] & 0xff;
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    set_lower_byte(rM, shift8((regs[rM & 3] >> ((rM & 4) << 1)), y));
                } else {
                    segment_translation();
                    x = ld8_writable_cpl3();
                    x = shift8(x, y);
                    st8_writable_cpl3(x);
                }
                goto EXEC_LOOP;
            case 0xd3: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
                modRM = ld8_direct();
                operation = (modRM >> 3) & 7;
                y = regs[1] & 0xff;
                if ((modRM >> 6) == 3) {
                    rM = modRM & 7;
                    regs[rM] = shift(regs[rM], y);
                } else {
                    segment_translation();
                    x = ld_writable_cpl3();
                    x = shift(x, y);
                    st_writable_cpl3(x);
                }
                goto EXEC_LOOP;
            case 0x98: // CBW
                regs[0] = (regs[0] << 16) >> 16;
                goto EXEC_LOOP;
            case 0x99: // CWD
                regs[2] = regs[0] >> 31;
                goto EXEC_LOOP;
            case 0x50: // PUSH A
            case 0x51: // PUSH C
            case 0x52: // PUSH D
            case 0x53: // PUSH B
            case 0x54: // PUSH SP
            case 0x55: // PUSH BP
            case 0x56: // PUSH SI
            case 0x57: // PUSH DI
                x = regs[opcode & 7];
                if (x86_64_long_mode) {
                    lax = regs[4] - 4;
                    st_writable_cpl3(x);
                    regs[4] = lax;
                } else {
                    push_dword(x);
                }
                goto EXEC_LOOP;
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
                    x = ld_readonly_cpl3();
                    regs[4] = lax + 4;
                } else {
                    x = read_stack_dword();
                    pop_dword();
                }
                regs[opcode & 7] = x;
                goto EXEC_LOOP;
            case 0x60: // PUSHA
                aux_PUSHA();
                goto EXEC_LOOP;
            case 0x61: // POPA
                aux_POPA();
                goto EXEC_LOOP;
            case 0x8f: // POP
                modRM = ld8_direct();
                if ((modRM >> 6) == 3) {
                    x = read_stack_dword();
                    pop_dword();
                    regs[modRM & 7] = x;
                } else {
                    x = read_stack_dword();
                    y = regs[4];
                    pop_dword();
                    z = regs[4];
                    segment_translation();
                    regs[4] = y;
                    st_writable_cpl3(x);
                    regs[4] = z;
                }
                goto EXEC_LOOP;
            case 0x68: // PUSH
                x = ld_direct();
                if (x86_64_long_mode) {
                    lax = regs[4] - 4;
                    st_writable_cpl3(x);
                    regs[4] = lax;
                } else {
                    push_dword(x);
                }
                goto EXEC_LOOP;
            case 0x6a: // PUSH
                x = (ld8_direct() << 24) >> 24;
                if (x86_64_long_mode) {
                    lax = regs[4] - 4;
                    st_writable_cpl3(x);
                    regs[4] = lax;
                } else {
                    push_dword(x);
                }
                goto EXEC_LOOP;
            case 0xc8: // ENTER
                aux_ENTER();
                goto EXEC_LOOP;
            case 0xc9: // LEAVE
                if (x86_64_long_mode) {
                    lax = regs[5];
                    x = ld_readonly_cpl3();
                    regs[5] = x;
                    regs[4] = lax + 4;
                } else {
                    aux_LEAVE();
                }
                goto EXEC_LOOP;
            case 0x9c: // PUSHF
                iopl = (eflags >> 12) & 3;
                if ((eflags & 0x00020000) && iopl != 3) {
                    abort(13);
                }
                x = get_EFLAGS() & ~(0x00010000 | 0x00020000);
                if (((ipr >> 8) & 1) ^ 1) {
                    push_dword(x);
                } else {
                    push_word(x);
                }
                goto EXEC_LOOP;
            case 0x9d: // POPF
                iopl = (eflags >> 12) & 3;
                if ((eflags & 0x00020000) && iopl != 3) {
                    abort(13);
                }
                if (((ipr >> 8) & 1) ^ 1) {
                    x = read_stack_dword();
                    pop_dword();
                    y = -1;
                } else {
                    x = read_stack_word();
                    pop_word();
                    y = 0xffff;
                }
                z = 0x00000100 | 0x00004000 | 0x00040000 | 0x00200000;
                if (cpl == 0) {
                    z |= 0x00000200 | 0x00003000;
                } else {
                    if (cpl <= iopl) {
                        z |= 0x00000200;
                    }
                }
                set_EFLAGS(x, z & y);
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0x06: // PUSH
            case 0x0e: // PUSH
            case 0x16: // PUSH
            case 0x1e: // PUSH
                push_dword(segs[opcode >> 3].selector);
                goto EXEC_LOOP;
            case 0x07: // POP
            case 0x17: // POP
            case 0x1f: // POP
                x = read_stack_dword() & 0xffff;
                pop_dword();
                set_segment_register(opcode >> 3, x);
                goto EXEC_LOOP;
            case 0x8d: // LEA
                modRM = ld8_direct();
                if ((modRM >> 6) == 3) {
                    abort(6);
                }
                ipr = (ipr & ~0x000f) | (6 + 1);
                segment_translation();
                regs[(modRM >> 3) & 7] = lax;
                goto EXEC_LOOP;
            case 0xfe: // G4 (INC, DEC, -, -, -, -, -)
                modRM = ld8_direct();
                operation = (modRM >> 3) & 7;
                switch (operation) {
                case 0:
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        set_lower_byte(rM, aux_INC8((regs[rM & 3] >> ((rM & 4) << 1))));
                    } else {
                        segment_translation();
                        x = ld8_writable_cpl3();
                        x = aux_INC8(x);
                        st8_writable_cpl3(x);
                    }
                    break;
                case 1:
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        set_lower_byte(rM, aux_DEC8((regs[rM & 3] >> ((rM & 4) << 1))));
                    } else {
                        segment_translation();
                        x = ld8_writable_cpl3();
                        x = aux_DEC8(x);
                        st8_writable_cpl3(x);
                    }
                    break;
                default:
                    abort(6);
                }
                goto EXEC_LOOP;
            case 0xff: // G5 (INC, DEC, CALL, CALL, JMP, JMP, PUSH, -)
                modRM = ld8_direct();
                operation = (modRM >> 3) & 7;
                switch (operation) {
                case 0: // INC
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        if (osm < 25) {
                            ocm_preserved = osm;
                            ocm_dst_preserved = osm_dst;
                        }
                        regs[rM] = osm_dst = regs[rM] + 1;
                        osm = 27;
                    } else {
                        segment_translation();
                        x = ld_writable_cpl3();
                        if (osm < 25) {
                            ocm_preserved = osm;
                            ocm_dst_preserved = osm_dst;
                        }
                        x = osm_dst = x + 1;
                        osm = 27;
                        st_writable_cpl3(x);
                    }
                    break;
                case 1: // DEC
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        if (osm < 25) {
                            ocm_preserved = osm;
                            ocm_dst_preserved = osm_dst;
                        }
                        regs[rM] = osm_dst = regs[rM] - 1;
                        osm = 30;
                    } else {
                        segment_translation();
                        x = ld_writable_cpl3();
                        if (osm < 25) {
                            ocm_preserved = osm;
                            ocm_dst_preserved = osm_dst;
                        }
                        x = osm_dst = x - 1;
                        osm = 30;
                        st_writable_cpl3(x);
                    }
                    break;
                case 2: // CALL
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    y = eip + far - far_start;
                    if (x86_64_long_mode) {
                        lax = regs[4] - 4;
                        st_writable_cpl3(y);
                        regs[4] = lax;
                    } else {
                        push_dword(y);
                    }
                    eip = rm, far = far_start = 0;
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
                        push_dword(rm);
                    }
                    break;
                case 3: // CALLF
                case 5: // JMPF
                    if ((modRM >> 6) == 3) {
                        abort(6);
                    }
                    segment_translation();
                    x = ld_readonly_cpl3();
                    lax = lax + 4;
                    y = ld16_readonly_cpl3();
                    if (operation == 3) {
                        aux_CALLF(1, y, x, (eip + far - far_start));
                    } else {
                        aux_JMPF(y, x);
                    }
                    break;
                default:
                    abort(6);
                }
                goto EXEC_LOOP;
            case 0xeb: // JMP
                x = (ld8_direct() << 24) >> 24;
                far = far + x;
                goto EXEC_LOOP;
            case 0xe9: // JMP
                x = ld_direct();
                far = far + x;
                goto EXEC_LOOP;
            case 0xea: // JMPF
                if (((ipr >> 8) & 1) ^ 1) {
                    x = ld_direct();
                } else {
                    x = ld16_direct();
                }
                y = ld16_direct();
                aux_JMPF(y, x);
                goto EXEC_LOOP;
            case 0x70: // JO
                if (is_OF()) {
                    x = (ld8_direct() << 24) >> 24;
                    far = far + x;
                } else {
                    far = far + 1;
                }
                goto EXEC_LOOP;
            case 0x71: // JNO
                if (!is_OF()) {
                    x = (ld8_direct() << 24) >> 24;
                    far = far + x;
                } else {
                    far = far + 1;
                }
                goto EXEC_LOOP;
            case 0x72: // JB
                if (is_CF()) {
                    x = (ld8_direct() << 24) >> 24;
                    far = far + x;
                } else {
                    far = far + 1;
                }
                goto EXEC_LOOP;
            case 0x73: // JNB
                if (!is_CF()) {
                    x = (ld8_direct() << 24) >> 24;
                    far = far + x;
                } else {
                    far = far + 1;
                }
                goto EXEC_LOOP;
            case 0x74: // JZ
                if (osm_dst == 0) {
                    x = (ld8_direct() << 24) >> 24;
                    far = far + x;
                } else {
                    far = far + 1;
                }
                goto EXEC_LOOP;
            case 0x75: // JNZ
                if (!(osm_dst == 0)) {
                    x = (ld8_direct() << 24) >> 24;
                    far = far + x;
                } else {
                    far = far + 1;
                }
                goto EXEC_LOOP;
            case 0x76: // JBE
                if (is_BE()) {
                    x = (ld8_direct() << 24) >> 24;
                    far = far + x;
                } else {
                    far = far + 1;
                }
                goto EXEC_LOOP;
            case 0x77: // JNBE
                if (!is_BE()) {
                    x = (ld8_direct() << 24) >> 24;
                    far = far + x;
                } else {
                    far = far + 1;
                }
                goto EXEC_LOOP;
            case 0x78: // JS
                if (osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst < 0)) {
                    x = (ld8_direct() << 24) >> 24;
                    far = far + x;
                } else {
                    far = far + 1;
                }
                goto EXEC_LOOP;
            case 0x79: // JNS
                if (!(osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst < 0))) {
                    x = (ld8_direct() << 24) >> 24;
                    far = far + x;
                } else {
                    far = far + 1;
                }
                goto EXEC_LOOP;
            case 0x7a: // JP
                if (is_PF()) {
                    x = (ld8_direct() << 24) >> 24;
                    far = far + x;
                } else {
                    far = far + 1;
                }
                goto EXEC_LOOP;
            case 0x7b: // JNP
                if (!is_PF()) {
                    x = (ld8_direct() << 24) >> 24;
                    far = far + x;
                } else {
                    far = far + 1;
                }
                goto EXEC_LOOP;
            case 0x7c: // JL
                if (is_LT()) {
                    x = (ld8_direct() << 24) >> 24;
                    far = far + x;
                } else {
                    far = far + 1;
                }
                goto EXEC_LOOP;
            case 0x7d: // JNL
                if (!is_LT()) {
                    x = (ld8_direct() << 24) >> 24;
                    far = far + x;
                } else {
                    far = far + 1;
                }
                goto EXEC_LOOP;
            case 0x7e: // JLE
                if (is_LE()) {
                    x = (ld8_direct() << 24) >> 24;
                    far = far + x;
                } else {
                    far = far + 1;
                }
                goto EXEC_LOOP;
            case 0x7f: // JNLE
                if (!is_LE()) {
                    x = (ld8_direct() << 24) >> 24;
                    far = far + x;
                } else {
                    far = far + 1;
                }
                goto EXEC_LOOP;
            case 0xe0: // LOOPNE
            case 0xe1: // LOOPE
            case 0xe2: // LOOP
                x = (ld8_direct() << 24) >> 24;
                if (ipr & 0x0080) {
                    v = 0xffff;
                } else {
                    v = -1;
                }
                y = (regs[1] - 1) & v;
                regs[1] = (regs[1] & ~v) | y;
                opcode &= 3;
                if (opcode == 0) {
                    z = !(osm_dst == 0);
                } else if (opcode == 1) {
                    z = osm_dst == 0;
                } else {
                    z = 1;
                }
                if (y && z) {
                    if (ipr & 0x0100) {
                        eip = (eip + far - far_start + x) & 0xffff, far = far_start = 0;
                    } else {
                        far = far + x;
                    }
                }
                goto EXEC_LOOP;
            case 0xe3: // JCXZ
                x = (ld8_direct() << 24) >> 24;
                if (ipr & 0x0080) {
                    v = 0xffff;
                } else {
                    v = -1;
                }
                if ((regs[1] & v) == 0) {
                    if (ipr & 0x0100) {
                        eip = (eip + far - far_start + x) & 0xffff, far = far_start = 0;
                    } else {
                        far = far + x;
                    }
                }
                goto EXEC_LOOP;
            case 0xc2: // RET
                y = (ld16_direct() << 16) >> 16;
                x = read_stack_dword();
                regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 4 + y) & SS_mask);
                eip = x, far = far_start = 0;
                goto EXEC_LOOP;
            case 0xc3: // RET
                if (x86_64_long_mode) {
                    lax = regs[4];
                    x = ld_readonly_cpl3();
                    regs[4] = regs[4] + 4;
                } else {
                    x = read_stack_dword();
                    pop_dword();
                }
                eip = x, far = far_start = 0;
                goto EXEC_LOOP;
            case 0xe8: // CALL
                x = ld_direct();
                y = eip + far - far_start;
                if (x86_64_long_mode) {
                    lax = regs[4] - 4;
                    st_writable_cpl3(y);
                    regs[4] = lax;
                } else {
                    push_dword(y);
                }
                far = far + x;
                goto EXEC_LOOP;
            case 0x9a: // CALLF
                z = ((ipr >> 8) & 1) ^ 1;
                if (z) {
                    x = ld_direct();
                } else {
                    x = ld16_direct();
                }
                y = ld16_direct();
                aux_CALLF(z, y, x, (eip + far - far_start));
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xca: // RET
                y = (ld16_direct() << 16) >> 16;
                aux_RETF((((ipr >> 8) & 1) ^ 1), y);
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xcb: // RET
                aux_RETF((((ipr >> 8) & 1) ^ 1), 0);
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xcf: // IRET
                aux_IRET((((ipr >> 8) & 1) ^ 1));
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0x90: // NOP
                goto EXEC_LOOP;
            case 0xcc: // INT
                y = eip + far - far_start;
                raise_interrupt(3, 0, 0, 1, y);
                goto EXEC_LOOP;
            case 0xcd: // INT
                x = ld8_direct();
                if ((eflags & 0x00020000) && ((eflags >> 12) & 3) != 3) {
                    abort(13);
                }
                y = eip + far - far_start;
                raise_interrupt(x, 0, 0, 1, y);
                goto EXEC_LOOP;
            case 0xce: // INTO
                if (is_OF()) {
                    y = eip + far - far_start;
                    raise_interrupt(4, 0, 0, 1, y);
                }
                goto EXEC_LOOP;
            case 0x62: // BOUND
                aux_BOUND();
                goto EXEC_LOOP;
            case 0xf5: // CMC
                osm_src = compile_eflags() ^ 0x0001;
                osm_dst = ((osm_src >> 6) & 1) ^ 1;
                osm = 24;
                goto EXEC_LOOP;
            case 0xf8: // CLC
                osm_src = compile_eflags() & ~0x0001;
                osm_dst = ((osm_src >> 6) & 1) ^ 1;
                osm = 24;
                goto EXEC_LOOP;
            case 0xf9: // STC
                osm_src = compile_eflags() | 0x0001;
                osm_dst = ((osm_src >> 6) & 1) ^ 1;
                osm = 24;
                goto EXEC_LOOP;
            case 0xfc: // CLD
                df = 1;
                goto EXEC_LOOP;
            case 0xfd: // STD
                df = -1;
                goto EXEC_LOOP;
            case 0xfa: // CLI
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                eflags &= ~0x00000200;
                goto EXEC_LOOP;
            case 0xfb: // STI
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                eflags |= 0x00000200;
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0x9e: // SAHF
                osm_src = ((regs[0] >> 8) & (0x0080 | 0x0040 | 0x0010 | 0x0004 | 0x0001)) | (is_OF() << 11);
                osm_dst = ((osm_src >> 6) & 1) ^ 1;
                osm = 24;
                goto EXEC_LOOP;
            case 0x9f: // LAHF
                x = get_EFLAGS();
                set_lower_byte(4, x);
                goto EXEC_LOOP;
            case 0xf4: // HLT
                if (cpl != 0) {
                    abort(13);
                }
                halted = 1;
                goto OUTER_LOOP;
            case 0xa4: // MOVSB
                aux_MOVSB();
                goto EXEC_LOOP;
            case 0xa5: // MOVSW/D
                ipr & 0x0100 ? aux_MOVSW() : aux_MOVSD();
                goto EXEC_LOOP;
            case 0xaa: // STOSB
                aux_STOSB();
                goto EXEC_LOOP;
            case 0xab: // STOSW/D
                ipr & 0x0100 ? aux_STOSW() : aux_STOSD();
                goto EXEC_LOOP;
            case 0xa6: // CMPSB
                aux_CMPSB();
                goto EXEC_LOOP;
            case 0xa7: // CMPSW/D
                ipr & 0x0100 ? aux_CMPSW() : aux_CMPSD();
                goto EXEC_LOOP;
            case 0xac: // LOSB
                aux_LODSB();
                goto EXEC_LOOP;
            case 0xad: // LOSW/D
                ipr & 0x0100 ? aux_LODSW() : aux_LODSD();
                goto EXEC_LOOP;
            case 0xae: // SCASB
                aux_SCASB();
                goto EXEC_LOOP;
            case 0xaf: // SCASW/D
                ipr & 0x0100 ? aux_SCASW() : aux_SCASD();
                goto EXEC_LOOP;
            case 0x6c: // INSB
                aux_INSB();
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0x6d: // INSW/D
                ipr & 0x0100 ? aux_INSW() : aux_INSD();
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0x6e: // OUTSB
                aux_OUTSB();
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0x6f: // OUTSW/D
                ipr & 0x0100 ? aux_OUTSW() : aux_OUTSD();
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xd8: // ESC (80387)
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
                modRM = ld8_direct();
                reg = (modRM >> 3) & 7;
                rM = modRM & 7;
                operation = ((opcode & 7) << 3) | ((modRM >> 3) & 7);
                set_lower_word(0, 0xffff);
                if ((modRM >> 6) == 3) {
                } else {
                    segment_translation();
                }
                goto EXEC_LOOP;
            case 0x9b: // FWAIT/WAIT
                goto EXEC_LOOP;
            case 0xe4: // IN AL,
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                x = ld8_direct();
                set_lower_byte(0, ld8_io(x));
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xe5: // IN AX,
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                x = ld8_direct();
                regs[0] = ld_io(x);
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xe6: // OUT ,AL
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                x = ld8_direct();
                st8_io(x, regs[0] & 0xff);
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xe7: // OUT ,AX
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                x = ld8_direct();
                st_io(x, regs[0]);
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xec: // IN AL,DX
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                set_lower_byte(0, ld8_io(regs[2] & 0xffff));
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xed: // IN AX,DX
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                regs[0] = ld_io(regs[2] & 0xffff);
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xee: // OUT DX,AL
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                st8_io(regs[2] & 0xffff, regs[0] & 0xff);
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xef: // OUT DX,AX
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                st_io(regs[2] & 0xffff, regs[0]);
                if (get_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0x27: // DAA
                aux_DAA();
                goto EXEC_LOOP;
            case 0x2f: // DAS
                aux_DAS();
                goto EXEC_LOOP;
            case 0x37: // AAA
                aux_AAA();
                goto EXEC_LOOP;
            case 0x3f: // AAS
                aux_AAS();
                goto EXEC_LOOP;
            case 0xd4: // AAM
                x = ld8_direct();
                aux_AAM(x);
                goto EXEC_LOOP;
            case 0xd5: // AAD
                x = ld8_direct();
                aux_AAD(x);
                goto EXEC_LOOP;
            case 0x63: // ARPL
                aux_ARPL();
                goto EXEC_LOOP;
            case 0xd6: // -
            case 0xf1: // -
                abort(6);
            case 0x0f: // 2-byte instruction escape
                opcode = ld8_direct();
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
                    x = ld_direct();
                    if (can_jump(opcode & 0xf)) {
                        far = far + x;
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
                    modRM = ld8_direct();
                    x = can_jump(opcode & 0xf);
                    if ((modRM >> 6) == 3) {
                        set_lower_byte(modRM & 7, x);
                    } else {
                        segment_translation();
                        st8_writable_cpl3(x);
                    }
                    goto EXEC_LOOP;
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
                    modRM = ld8_direct();
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    if (can_jump(opcode & 0xf)) {
                        regs[(modRM >> 3) & 7] = rm;
                    }
                    goto EXEC_LOOP;
                case 0xb6: // MOVZX
                    modRM = ld8_direct();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        x = (regs[rM & 3] >> ((rM & 4) << 1)) & 0xff;
                    } else {
                        segment_translation();
                        x = ld8_readonly_cpl3();
                    }
                    regs[reg] = x;
                    goto EXEC_LOOP;
                case 0xb7: // MOVZX
                    modRM = ld8_direct();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7] & 0xffff;
                    } else {
                        segment_translation();
                        rm = ld16_readonly_cpl3();
                    }
                    regs[reg] = rm;
                    goto EXEC_LOOP;
                case 0xbe: // MOVSX
                    modRM = ld8_direct();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        x = regs[rM & 3] >> ((rM & 4) << 1);
                    } else {
                        segment_translation();
                        x = ld8_readonly_cpl3();
                    }
                    regs[reg] = (x << 24) >> 24;
                    goto EXEC_LOOP;
                case 0xbf: // MOVSX
                    modRM = ld8_direct();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld16_readonly_cpl3();
                    }
                    regs[reg] = (rm << 16) >> 16;
                    goto EXEC_LOOP;
                case 0x00: // G6 (SLDT, STR, LLDT, LTR, VERR, VERW, -)
                    if (!is_protected() || (eflags & 0x00020000)) {
                        abort(6);
                    }
                    modRM = ld8_direct();
                    operation = (modRM >> 3) & 7;
                    switch (operation) {
                    case 0: // SLDT Store Local Descriptor Table Register
                    case 1: // STR Store Task Register
                        if (operation == 0) {
                            x = ldt.selector;
                        } else {
                            x = tr.selector;
                        }
                        if ((modRM >> 6) == 3) {
                            set_lower_word(modRM & 7, x);
                        } else {
                            segment_translation();
                            st16_writable_cpl3(x);
                        }
                        break;
                    case 2: // LDTR Load Local Descriptor Table Register
                    case 3: // LTR Load Task Register
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
                    case 4: // VERR Verify a Segment for Reading
                    case 5: // VERW Verify a Segment for Writing
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
                    goto EXEC_LOOP;
                case 0x01: // G7 (SGDT, SIDT, LGDT, LIDT, SMSW, -, LMSW, -)
                    modRM = ld8_direct();
                    operation = (modRM >> 3) & 7;
                    switch (operation) {
                    case 2:
                    case 3:
                        if ((modRM >> 6) == 3) {
                            abort(6);
                        }
                        if (cpl != 0) {
                            abort(13);
                        }
                        segment_translation();
                        x = ld16_readonly_cpl3();
                        lax += 2;
                        y = ld_readonly_cpl3();
                        if (operation == 2) {
                            gdt.base = y;
                            gdt.limit = x;
                        } else {
                            idt.base = y;
                            idt.limit = x;
                        }
                        break;
                    case 7:
                        if (cpl != 0) {
                            abort(13);
                        }
                        if ((modRM >> 6) == 3) {
                            abort(6);
                        }
                        segment_translation();
                        tlb_flush_page(lax & -4096);
                        break;
                    default:
                        abort(6);
                    }
                    goto EXEC_LOOP;
                case 0x02: // LAR
                case 0x03: // LSL
                    aux_LAR_LSL((((ipr >> 8) & 1) ^ 1), opcode & 1);
                    goto EXEC_LOOP;
                case 0x20: // MOV
                    if (cpl != 0) {
                        abort(13);
                    }
                    modRM = ld8_direct();
                    if ((modRM >> 6) != 3) {
                        abort(6);
                    }
                    reg = (modRM >> 3) & 7;
                    switch (reg) {
                    case 0:
                        x = cr0;
                        break;
                    case 2:
                        x = cr2;
                        break;
                    case 3:
                        x = cr3;
                        break;
                    case 4:
                        x = cr4;
                        break;
                    default:
                        abort(6);
                    }
                    regs[modRM & 7] = x;
                    goto EXEC_LOOP;
                case 0x22: // MOV
                    if (cpl != 0) {
                        abort(13);
                    }
                    modRM = ld8_direct();
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
                        set_CR4(rm);
                        break;
                    default:
                        abort(6);
                    }
                    goto EXEC_LOOP;
                case 0x06: // CLTS
                    if (cpl != 0) {
                        abort(13);
                    }
                    set_CR0(cr0 & ~(1 << 3));
                    goto EXEC_LOOP;
                case 0x23: // MOV
                    if (cpl != 0) {
                        abort(13);
                    }
                    modRM = ld8_direct();
                    if ((modRM >> 6) != 3) {
                        abort(6);
                    }
                    reg = (modRM >> 3) & 7;
                    rm = regs[modRM & 7];
                    if (reg == 4 || reg == 5) {
                        abort(6);
                    }
                    goto EXEC_LOOP;
                case 0xb2: // LSS
                case 0xb4: // LFS
                case 0xb5: // LGS
                    ld_far_pointer(opcode & 7);
                    goto EXEC_LOOP;
                case 0xa2: // CPUID (80486)
                    aux_CPUID();
                    goto EXEC_LOOP;
                case 0xa4: // SHLD
                    modRM = ld8_direct();
                    y = regs[(modRM >> 3) & 7];
                    if ((modRM >> 6) == 3) {
                        z = ld8_direct();
                        rM = modRM & 7;
                        regs[rM] = aux_SHLD(regs[rM], y, z);
                    } else {
                        segment_translation();
                        z = ld8_direct();
                        x = ld_writable_cpl3();
                        x = aux_SHLD(x, y, z);
                        st_writable_cpl3(x);
                    }
                    goto EXEC_LOOP;
                case 0xa5: // SHLD
                    modRM = ld8_direct();
                    y = regs[(modRM >> 3) & 7];
                    z = regs[1];
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        regs[rM] = aux_SHLD(regs[rM], y, z);
                    } else {
                        segment_translation();
                        x = ld_writable_cpl3();
                        x = aux_SHLD(x, y, z);
                        st_writable_cpl3(x);
                    }
                    goto EXEC_LOOP;
                case 0xac: // SHRD
                    modRM = ld8_direct();
                    y = regs[(modRM >> 3) & 7];
                    if ((modRM >> 6) == 3) {
                        z = ld8_direct();
                        rM = modRM & 7;
                        regs[rM] = aux_SHRD(regs[rM], y, z);
                    } else {
                        segment_translation();
                        z = ld8_direct();
                        x = ld_writable_cpl3();
                        x = aux_SHRD(x, y, z);
                        st_writable_cpl3(x);
                    }
                    goto EXEC_LOOP;
                case 0xad: // SHRD
                    modRM = ld8_direct();
                    y = regs[(modRM >> 3) & 7];
                    z = regs[1];
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        regs[rM] = aux_SHRD(regs[rM], y, z);
                    } else {
                        segment_translation();
                        x = ld_writable_cpl3();
                        x = aux_SHRD(x, y, z);
                        st_writable_cpl3(x);
                    }
                    goto EXEC_LOOP;
                case 0xba: // G8 (-, -, -, -, BT, BTS, BTR, BTC)
                    modRM = ld8_direct();
                    operation = (modRM >> 3) & 7;
                    switch (operation) {
                    case 4: // BT
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                            y = ld8_direct();
                        } else {
                            segment_translation();
                            y = ld8_direct();
                            rm = ld_readonly_cpl3();
                        }
                        aux_BT(rm, y);
                        break;
                    case 5: // BTS
                    case 6: // BTR
                    case 7: // BTC
                        operation &= 3;
                        if ((modRM >> 6) == 3) {
                            rM = modRM & 7;
                            y = ld8_direct();
                            regs[rM] = aux_BTS_BTR_BTC(regs[rM], y);
                        } else {
                            segment_translation();
                            y = ld8_direct();
                            x = ld_writable_cpl3();
                            x = aux_BTS_BTR_BTC(x, y);
                            st_writable_cpl3(x);
                        }
                        break;
                    default:
                        abort(6);
                    }
                    goto EXEC_LOOP;
                case 0xa3: // BT
                    modRM = ld8_direct();
                    y = regs[(modRM >> 3) & 7];
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        lax = lax + ((y >> 5) << 2);
                        rm = ld_readonly_cpl3();
                    }
                    aux_BT(rm, y);
                    goto EXEC_LOOP;
                case 0xab: // BTS
                case 0xb3: // BTR
                case 0xbb: // BTC
                    modRM = ld8_direct();
                    y = regs[(modRM >> 3) & 7];
                    operation = (opcode >> 3) & 3;
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        regs[rM] = aux_BTS_BTR_BTC(regs[rM], y);
                    } else {
                        segment_translation();
                        lax = lax + ((y >> 5) << 2);
                        x = ld_writable_cpl3();
                        x = aux_BTS_BTR_BTC(x, y);
                        st_writable_cpl3(x);
                    }
                    goto EXEC_LOOP;
                case 0xbc: // BSF
                case 0xbd: // BSR
                    modRM = ld8_direct();
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
                    goto EXEC_LOOP;
                case 0xaf: // IMUL
                    modRM = ld8_direct();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld_readonly_cpl3();
                    }
                    aux_IMUL(regs[reg], rm);
                    regs[reg] = x;
                    goto EXEC_LOOP;
                case 0x31: // RDTSC (80486)
                    if ((cr4 & (1 << 2)) && cpl != 0) {
                        abort(13);
                    }
                    {
                        uint64_t t = this->cycles + (cycles_requested - cycles_remaining);
                        regs[0] = t;
                        regs[2] = t >> 32;
                    }
                    goto EXEC_LOOP;
                case 0xc0: // XADD (80486)
                    operation = 0;
                    modRM = ld8_direct();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        x = regs[rM & 3] >> ((rM & 4) << 1);
                        y = calculate8(x, (regs[reg & 3] >> ((reg & 4) << 1)));
                        set_lower_byte(reg, x);
                        set_lower_byte(rM, y);
                    } else {
                        segment_translation();
                        x = ld8_writable_cpl3();
                        y = calculate8(x, (regs[reg & 3] >> ((reg & 4) << 1)));
                        st8_writable_cpl3(y);
                        set_lower_byte(reg, x);
                    }
                    goto EXEC_LOOP;
                case 0xc1: // XADD (80486)
                    operation = 0;
                    modRM = ld8_direct();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        x = regs[rM];
                        y = calculate(x, regs[reg]);
                        regs[reg] = x;
                        regs[rM] = y;
                    } else {
                        segment_translation();
                        x = ld_writable_cpl3();
                        y = calculate(x, regs[reg]);
                        st_writable_cpl3(y);
                        regs[reg] = x;
                    }
                    goto EXEC_LOOP;
                case 0xb0: // CMPXCHG (80486)
                    operation = 5;
                    modRM = ld8_direct();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        x = regs[rM & 3] >> ((rM & 4) << 1);
                        y = calculate8(regs[0], x);
                        if (y == 0) {
                            set_lower_byte(rM, (regs[reg & 3] >> ((reg & 4) << 1)));
                        } else {
                            set_lower_byte(0, x);
                        }
                    } else {
                        segment_translation();
                        x = ld8_writable_cpl3();
                        y = calculate8(regs[0], x);
                        if (y == 0) {
                            st8_writable_cpl3((regs[reg & 3] >> ((reg & 4) << 1)));
                        } else {
                            set_lower_byte(0, x);
                        }
                    }
                    goto EXEC_LOOP;
                case 0xb1: // CMPXCHG (80486)
                    operation = 5;
                    modRM = ld8_direct();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        x = regs[rM];
                        y = calculate(regs[0], x);
                        if (y == 0) {
                            regs[rM] = regs[reg];
                        } else {
                            regs[0] = x;
                        }
                    } else {
                        segment_translation();
                        x = ld_writable_cpl3();
                        y = calculate(regs[0], x);
                        if (y == 0) {
                            st_writable_cpl3(regs[reg]);
                        } else {
                            regs[0] = x;
                        }
                    }
                    goto EXEC_LOOP;
                case 0xa0: // PUSH FS
                case 0xa8: // PUSH GS
                    push_dword(segs[(opcode >> 3) & 7].selector);
                    goto EXEC_LOOP;
                case 0xa1: // POP FS
                case 0xa9: // POP GS
                    x = read_stack_dword() & 0xffff;
                    pop_dword();
                    set_segment_register((opcode >> 3) & 7, x);
                    goto EXEC_LOOP;
                case 0xc8: // -
                case 0xc9: // -
                case 0xca: // -
                case 0xcb: // -
                case 0xcc: // -
                case 0xcd: // -
                case 0xce: // -
                case 0xcf: // BSWAP (80486)
                    reg = opcode & 7;
                    x = regs[reg];
                    x = ((x >> 24) & 0xff) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24);
                    regs[reg] = x;
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
                switch (opcode) {
                case 0x189: // MOV
                    modRM = ld8_direct();
                    x = regs[(modRM >> 3) & 7];
                    if ((modRM >> 6) == 3) {
                        set_lower_word(modRM & 7, x);
                    } else {
                        segment_translation();
                        st16_writable_cpl3(x);
                    }
                    goto EXEC_LOOP;
                case 0x18b: // MOV
                    modRM = ld8_direct();
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld16_readonly_cpl3();
                    }
                    set_lower_word((modRM >> 3) & 7, rm);
                    goto EXEC_LOOP;
                case 0x1b8: // MOV A
                case 0x1b9: // MOV C
                case 0x1ba: // MOV D
                case 0x1bb: // MOV B
                case 0x1bc: // MOV SP
                case 0x1bd: // MOV BP
                case 0x1be: // MOV SI
                case 0x1bf: // MOV DI
                    set_lower_word(opcode & 7, ld16_direct());
                    goto EXEC_LOOP;
                case 0x1a1: // MOV AX,
                    offset_to_linear(false);
                    x = ld16_readonly_cpl3();
                    set_lower_word(0, x);
                    goto EXEC_LOOP;
                case 0x1a3: // MOV ,AX
                    offset_to_linear(true);
                    st16_writable_cpl3(regs[0]);
                    goto EXEC_LOOP;
                case 0x1c7: // MOV
                    modRM = ld8_direct();
                    if ((modRM >> 6) == 3) {
                        x = ld16_direct();
                        set_lower_word(modRM & 7, x);
                    } else {
                        segment_translation();
                        x = ld16_direct();
                        st16_writable_cpl3(x);
                    }
                    goto EXEC_LOOP;
                case 0x191: // XCHG C
                case 0x192: // XCHG D
                case 0x193: // XCHG B
                case 0x194: // XCHG SP
                case 0x195: // XCHG BP
                case 0x196: // XCHG SI
                case 0x197: // XCHG DI
                    reg = opcode & 7;
                    x = regs[0];
                    set_lower_word(0, regs[reg]);
                    set_lower_word(reg, x);
                    goto EXEC_LOOP;
                case 0x187: // XCHG
                    modRM = ld8_direct();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        x = regs[rM];
                        set_lower_word(rM, regs[reg]);
                    } else {
                        segment_translation();
                        x = ld16_writable_cpl3();
                        st16_writable_cpl3(regs[reg]);
                    }
                    set_lower_word(reg, x);
                    goto EXEC_LOOP;
                case 0x1c4: // LES
                    ld_far_pointer16(0);
                    goto EXEC_LOOP;
                case 0x1c5: // LDS
                    ld_far_pointer16(3);
                    goto EXEC_LOOP;
                case 0x101: // ADD
                case 0x109: // OR
                case 0x111: // ADC
                case 0x119: // SBB
                case 0x121: // AND
                case 0x129: // SUB
                case 0x131: // XOR
                case 0x139: // CMP
                    modRM = ld8_direct();
                    operation = (opcode >> 3) & 7;
                    y = regs[(modRM >> 3) & 7];
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        set_lower_word(rM, calculate16(regs[rM], y));
                    } else {
                        segment_translation();
                        if (operation != 7) {
                            x = ld16_writable_cpl3();
                            x = calculate16(x, y);
                            st16_writable_cpl3(x);
                        } else {
                            x = ld16_readonly_cpl3();
                            calculate16(x, y);
                        }
                    }
                    goto EXEC_LOOP;
                case 0x103: // ADD
                case 0x10b: // OR
                case 0x113: // ADC
                case 0x11b: // SBB
                case 0x123: // AND
                case 0x12b: // SUB
                case 0x133: // XOR
                case 0x13b: // CMP
                    modRM = ld8_direct();
                    operation = (opcode >> 3) & 7;
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld16_readonly_cpl3();
                    }
                    set_lower_word(reg, calculate16(regs[reg], rm));
                    goto EXEC_LOOP;
                case 0x105: // ADD
                case 0x10d: // OR
                case 0x115: // ADC
                case 0x11d: // SBB
                case 0x125: // AND
                case 0x12d: // SUB
                case 0x135: // XOR
                case 0x13d: // CMP
                    y = ld16_direct();
                    operation = (opcode >> 3) & 7;
                    set_lower_word(0, calculate16(regs[0], y));
                    goto EXEC_LOOP;
                case 0x181: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                    modRM = ld8_direct();
                    operation = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        y = ld16_direct();
                        set_lower_word(rM, calculate16(regs[rM], y));
                    } else {
                        segment_translation();
                        y = ld16_direct();
                        if (operation != 7) {
                            x = ld16_writable_cpl3();
                            x = calculate16(x, y);
                            st16_writable_cpl3(x);
                        } else {
                            operation = 7;
                            x = ld16_readonly_cpl3();
                            calculate16(x, y);
                        }
                    }
                    goto EXEC_LOOP;
                case 0x183: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                    modRM = ld8_direct();
                    operation = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        y = (ld8_direct() << 24) >> 24;
                        set_lower_word(rM, calculate16(regs[rM], y));
                    } else {
                        segment_translation();
                        y = (ld8_direct() << 24) >> 24;
                        if (operation != 7) {
                            x = ld16_writable_cpl3();
                            x = calculate16(x, y);
                            st16_writable_cpl3(x);
                        } else {
                            x = ld16_readonly_cpl3();
                            calculate16(x, y);
                        }
                    }
                    goto EXEC_LOOP;
                case 0x140: // INC A
                case 0x141: // INC C
                case 0x142: // INC D
                case 0x143: // INC B
                case 0x144: // INC SP
                case 0x145: // INC BP
                case 0x146: // INC SI
                case 0x147: // INC DI
                    reg = opcode & 7;
                    set_lower_word(reg, aux_INC16(regs[reg]));
                    goto EXEC_LOOP;
                case 0x148: // DEC A
                case 0x149: // DEC C
                case 0x14a: // DEC D
                case 0x14b: // DEC B
                case 0x14c: // DEC SP
                case 0x14d: // DEC BP
                case 0x14e: // DEC SI
                case 0x14f: // DEC DI
                    reg = opcode & 7;
                    set_lower_word(reg, aux_DEC16(regs[reg]));
                    goto EXEC_LOOP;
                case 0x16b: // IMUL
                    modRM = ld8_direct();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld16_readonly_cpl3();
                    }
                    z = (ld8_direct() << 24) >> 24;
                    aux_IMUL16(rm, z);
                    set_lower_word(reg, x);
                    goto EXEC_LOOP;
                case 0x169: // IMUL
                    modRM = ld8_direct();
                    reg = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld16_readonly_cpl3();
                    }
                    z = ld16_direct();
                    aux_IMUL16(rm, z);
                    set_lower_word(reg, x);
                    goto EXEC_LOOP;
                case 0x185: // TEST
                    modRM = ld8_direct();
                    if ((modRM >> 6) == 3) {
                        rm = regs[modRM & 7];
                    } else {
                        segment_translation();
                        rm = ld16_readonly_cpl3();
                    }
                    y = regs[(modRM >> 3) & 7];
                    osm_dst = ((rm & y) << 16) >> 16;
                    osm = 13;
                    goto EXEC_LOOP;
                case 0x1a9: // TEST
                    y = ld16_direct();
                    osm_dst = ((regs[0] & y) << 16) >> 16;
                    osm = 13;
                    goto EXEC_LOOP;
                case 0x1f7: // G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
                    modRM = ld8_direct();
                    operation = (modRM >> 3) & 7;
                    switch (operation) {
                    case 0:
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        y = ld16_direct();
                        osm_dst = ((rm & y) << 16) >> 16;
                        osm = 13;
                        break;
                    case 2:
                        if ((modRM >> 6) == 3) {
                            rM = modRM & 7;
                            set_lower_word(rM, ~regs[rM]);
                        } else {
                            segment_translation();
                            x = ld16_writable_cpl3();
                            x = ~x;
                            st16_writable_cpl3(x);
                        }
                        break;
                    case 3:
                        operation = 5;
                        if ((modRM >> 6) == 3) {
                            rM = modRM & 7;
                            set_lower_word(rM, calculate16(0, regs[rM]));
                        } else {
                            operation = 5;
                            segment_translation();
                            x = ld16_writable_cpl3();
                            x = calculate16(0, x);
                            st16_writable_cpl3(x);
                        }
                        break;
                    case 4:
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        aux_MUL16(regs[0], rm);
                        set_lower_word(0, x);
                        set_lower_word(2, x >> 16);
                        break;
                    case 5:
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        aux_IMUL16(regs[0], rm);
                        set_lower_word(0, x);
                        set_lower_word(2, x >> 16);
                        break;
                    case 6:
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        aux_DIV16(rm);
                        break;
                    case 7:
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        aux_IDIV16(rm);
                        break;
                    default:
                        abort(6);
                    }
                    goto EXEC_LOOP;
                case 0x1c1: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
                    modRM = ld8_direct();
                    operation = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        y = ld8_direct();
                        rM = modRM & 7;
                        set_lower_word(rM, shift16(regs[rM], y));
                    } else {
                        segment_translation();
                        y = ld8_direct();
                        x = ld16_writable_cpl3();
                        x = shift16(x, y);
                        st16_writable_cpl3(x);
                    }
                    goto EXEC_LOOP;
                case 0x1d1: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
                    modRM = ld8_direct();
                    operation = (modRM >> 3) & 7;
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        set_lower_word(rM, shift16(regs[rM], 1));
                    } else {
                        segment_translation();
                        x = ld16_writable_cpl3();
                        x = shift16(x, 1);
                        st16_writable_cpl3(x);
                    }
                    goto EXEC_LOOP;
                case 0x1d3: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
                    modRM = ld8_direct();
                    operation = (modRM >> 3) & 7;
                    y = regs[1] & 0xff;
                    if ((modRM >> 6) == 3) {
                        rM = modRM & 7;
                        set_lower_word(rM, shift16(regs[rM], y));
                    } else {
                        segment_translation();
                        x = ld16_writable_cpl3();
                        x = shift16(x, y);
                        st16_writable_cpl3(x);
                    }
                    goto EXEC_LOOP;
                case 0x198: // CBW
                    set_lower_word(0, (regs[0] << 24) >> 24);
                    goto EXEC_LOOP;
                case 0x199: // CWD
                    set_lower_word(2, (regs[0] << 16) >> 31);
                    goto EXEC_LOOP;
                case 0x190: // NOP
                    goto EXEC_LOOP;
                case 0x150: // PUSH A
                case 0x151: // PUSH C
                case 0x152: // PUSH D
                case 0x153: // PUSH B
                case 0x154: // PUSH SP
                case 0x155: // PUSH BP
                case 0x156: // PUSH SI
                case 0x157: // PUSH DI
                    push_word(regs[opcode & 7]);
                    goto EXEC_LOOP;
                case 0x158: // POP A
                case 0x159: // POP C
                case 0x15a: // POP D
                case 0x15b: // POP B
                case 0x15c: // POP SP
                case 0x15d: // POP BP
                case 0x15e: // POP SI
                case 0x15f: // POP DI
                    x = read_stack_word();
                    pop_word();
                    set_lower_word(opcode & 7, x);
                    goto EXEC_LOOP;
                case 0x160: // PUSHA
                    aux_PUSHA16();
                    goto EXEC_LOOP;
                case 0x161: // POPA
                    aux_POPA16();
                    goto EXEC_LOOP;
                case 0x18f: // POP
                    modRM = ld8_direct();
                    if ((modRM >> 6) == 3) {
                        x = read_stack_word();
                        pop_word();
                        set_lower_word(modRM & 7, x);
                    } else {
                        x = read_stack_word();
                        y = regs[4];
                        pop_word();
                        z = regs[4];
                        segment_translation();
                        regs[4] = y;
                        st16_writable_cpl3(x);
                        regs[4] = z;
                    }
                    goto EXEC_LOOP;
                case 0x168: // PUSH
                    x = ld16_direct();
                    push_word(x);
                    goto EXEC_LOOP;
                case 0x16a: // PUSH
                    x = (ld8_direct() << 24) >> 24;
                    push_word(x);
                    goto EXEC_LOOP;
                case 0x1c8: // ENTER
                    aux_ENTER16();
                    goto EXEC_LOOP;
                case 0x1c9: // LEAVE
                    aux_LEAVE16();
                    goto EXEC_LOOP;
                case 0x106: // PUSH
                case 0x10e: // PUSH
                case 0x116: // PUSH
                case 0x11e: // PUSH
                    push_word(segs[(opcode >> 3) & 3].selector);
                    goto EXEC_LOOP;
                case 0x107: // POP
                case 0x117: // POP
                case 0x11f: // POP
                    x = read_stack_word();
                    pop_word();
                    set_segment_register((opcode >> 3) & 3, x);
                    goto EXEC_LOOP;
                case 0x18d: // LEA
                    modRM = ld8_direct();
                    if ((modRM >> 6) == 3) {
                        abort(6);
                    }
                    ipr = (ipr & ~0x000f) | (6 + 1);
                    segment_translation();
                    set_lower_word((modRM >> 3) & 7, lax);
                    goto EXEC_LOOP;
                case 0x1ff: // G5 (INC, DEC, CALL, CALL, JMP, JMP, PUSH, -)
                    modRM = ld8_direct();
                    operation = (modRM >> 3) & 7;
                    switch (operation) {
                    case 0:
                        if ((modRM >> 6) == 3) {
                            rM = modRM & 7;
                            set_lower_word(rM, aux_INC16(regs[rM]));
                        } else {
                            segment_translation();
                            x = ld16_writable_cpl3();
                            x = aux_INC16(x);
                            st16_writable_cpl3(x);
                        }
                        break;
                    case 1:
                        if ((modRM >> 6) == 3) {
                            rM = modRM & 7;
                            set_lower_word(rM, aux_DEC16(regs[rM]));
                        } else {
                            segment_translation();
                            x = ld16_writable_cpl3();
                            x = aux_DEC16(x);
                            st16_writable_cpl3(x);
                        }
                        break;
                    case 2:
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7] & 0xffff;
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        push_word((eip + far - far_start));
                        eip = rm, far = far_start = 0;
                        break;
                    case 4:
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7] & 0xffff;
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        eip = rm, far = far_start = 0;
                        break;
                    case 6:
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        push_word(rm);
                        break;
                    case 3:
                    case 5:
                        if ((modRM >> 6) == 3) {
                            abort(6);
                        }
                        segment_translation();
                        x = ld16_readonly_cpl3();
                        lax = lax + 2;
                        y = ld16_readonly_cpl3();
                        if (operation == 3) {
                            aux_CALLF(0, y, x, (eip + far - far_start));
                        } else {
                            aux_JMPF(y, x);
                        }
                        break;
                    default:
                        abort(6);
                    }
                    goto EXEC_LOOP;
                case 0x1eb: // JMP
                    x = (ld8_direct() << 24) >> 24;
                    eip = (eip + far - far_start + x) & 0xffff, far = far_start = 0;
                    goto EXEC_LOOP;
                case 0x1e9: // JMP
                    x = ld16_direct();
                    eip = (eip + far - far_start + x) & 0xffff, far = far_start = 0;
                    goto EXEC_LOOP;
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
                    x = (ld8_direct() << 24) >> 24;
                    if (can_jump(opcode & 0xf)) {
                        eip = (eip + far - far_start + x) & 0xffff, far = far_start = 0;
                    }
                    goto EXEC_LOOP;
                case 0x1c2: // RET
                    y = (ld16_direct() << 16) >> 16;
                    x = read_stack_word();
                    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 2 + y) & SS_mask);
                    eip = x, far = far_start = 0;
                    goto EXEC_LOOP;
                case 0x1c3: // RET
                    x = read_stack_word();
                    pop_word();
                    eip = x, far = far_start = 0;
                    goto EXEC_LOOP;
                case 0x1e8: // CALL
                    x = ld16_direct();
                    push_word((eip + far - far_start));
                    eip = (eip + far - far_start + x) & 0xffff, far = far_start = 0;
                    goto EXEC_LOOP;
                case 0x162: // BOUND
                    aux_BOUND16();
                    goto EXEC_LOOP;
                case 0x1a5: // MOVSW/D
                    aux_MOVS16();
                    goto EXEC_LOOP;
                case 0x1a7: // CMPSW/D
                    aux_CMPS16();
                    goto EXEC_LOOP;
                case 0x1ad: // LOSW/D
                    aux_LODS16();
                    goto EXEC_LOOP;
                case 0x1af: // SCASW/D
                    aux_SCAS16();
                    goto EXEC_LOOP;
                case 0x1ab: // STOSW/D
                    aux_STOS16();
                    goto EXEC_LOOP;
                case 0x16d: // INSW/D
                    aux_INS16();
                    if (get_irq() != 0 && (eflags & 0x00000200)) {
                        goto OUTER_LOOP;
                    }
                    goto EXEC_LOOP;
                case 0x16f: // OUTSW/D
                    aux_OUTS16();
                    if (get_irq() != 0 && (eflags & 0x00000200)) {
                        goto OUTER_LOOP;
                    }
                    goto EXEC_LOOP;
                case 0x1e5: // IN AX,
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl) {
                        abort(13);
                    }
                    x = ld8_direct();
                    set_lower_word(0, ld16_io(x));
                    if (get_irq() != 0 && (eflags & 0x00000200)) {
                        goto OUTER_LOOP;
                    }
                    goto EXEC_LOOP;
                case 0x1e7: // OUT ,AX
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl) {
                        abort(13);
                    }
                    x = ld8_direct();
                    st16_io(x, regs[0] & 0xffff);
                    if (get_irq() != 0 && (eflags & 0x00000200)) {
                        goto OUTER_LOOP;
                    }
                    goto EXEC_LOOP;
                case 0x1ed: // IN AX,DX
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl) {
                        abort(13);
                    }
                    set_lower_word(0, ld16_io(regs[2] & 0xffff));
                    if (get_irq() != 0 && (eflags & 0x00000200)) {
                        goto OUTER_LOOP;
                    }
                    goto EXEC_LOOP;
                case 0x1ef: // OUT DX,AX
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl) {
                        abort(13);
                    }
                    st16_io(regs[2] & 0xffff, regs[0] & 0xffff);
                    if (get_irq() != 0 && (eflags & 0x00000200)) {
                        goto OUTER_LOOP;
                    }
                    goto EXEC_LOOP;
                case 0x166: // operand-size override prefix
                case 0x167: // address-size override prefix
                case 0x1f0: // LOCK prefix
                case 0x1f2: // REPN[EZ] repeat string operation prefix
                case 0x1f3: // REP[EZ] repeat string operation prefix
                case 0x126: // ES segment override prefix
                case 0x12e: // CS segment override prefix
                case 0x136: // SS segment override prefix
                case 0x13e: // DS segment override prefix
                case 0x164: // FS segment override prefix
                case 0x165: // GS segment override prefix
                case 0x100: // ADD
                case 0x108: // OR
                case 0x110: // ADC
                case 0x118: // SBB
                case 0x120: // AND
                case 0x128: // SUB
                case 0x130: // XOR
                case 0x138: // CMP
                case 0x102: // ADD
                case 0x10a: // OR
                case 0x112: // ADC
                case 0x11a: // SBB
                case 0x122: // AND
                case 0x12a: // SUB
                case 0x132: // XOR
                case 0x13a: // CMP
                case 0x104: // ADD
                case 0x10c: // OR
                case 0x114: // ADC
                case 0x11c: // SBB
                case 0x124: // AND
                case 0x12c: // SUB
                case 0x134: // XOR
                case 0x13c: // CMP
                case 0x1a0: // MOV AL,
                case 0x1a2: // MOV ,AL
                case 0x1d8: // ESC (80387)
                case 0x1d9: // ESC (80387)
                case 0x1da: // ESC (80387)
                case 0x1db: // ESC (80387)
                case 0x1dc: // ESC (80387)
                case 0x1dd: // ESC (80387)
                case 0x1de: // ESC (80387)
                case 0x1df: // ESC (80387)
                case 0x184: // TEST
                case 0x1a8: // TEST
                case 0x1f6: // G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
                case 0x1c0: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
                case 0x1d0: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
                case 0x1d2: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
                case 0x1fe: // G4 (INC, DEC, -, -, -, -, -)
                case 0x1cd: // INT
                case 0x1ce: // INTO
                case 0x1f5: // CMC
                case 0x1f8: // CLC
                case 0x1f9: // STC
                case 0x1fc: // CLD
                case 0x1fd: // STD
                case 0x1fa: // CLI
                case 0x1fb: // STI
                case 0x19e: // SAHF
                case 0x19f: // LAHF
                case 0x1f4: // HLT
                case 0x127: // DAA
                case 0x12f: // DAS
                case 0x137: // AAA
                case 0x13f: // AAS
                case 0x1d4: // AAM
                case 0x1d5: // AAD
                case 0x16c: // INSB
                case 0x16e: // OUTSB
                case 0x1a4: // MOVSB
                case 0x1a6: // CMPSB
                case 0x1aa: // STOSB
                case 0x1ac: // LOSB
                case 0x1ae: // SCASB
                case 0x180: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                case 0x182: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                case 0x186: // XCHG
                case 0x188: // MOV
                case 0x18a: // MOV
                case 0x18c: // MOV
                case 0x18e: // MOV
                case 0x19b: // FWAIT/WAIT
                case 0x1b0: // MOV AL
                case 0x1b1: // MOV CL
                case 0x1b2: // MOV DL
                case 0x1b3: // MOV BL
                case 0x1b4: // MOV AH
                case 0x1b5: // MOV CH
                case 0x1b6: // MOV DH
                case 0x1b7: // MOV BH
                case 0x1c6: // MOV
                case 0x1cc: // INT
                case 0x1d7: // XLAT
                case 0x1e4: // IN AL,
                case 0x1e6: // OUT ,AL
                case 0x1ec: // IN AL,DX
                case 0x1ee: // OUT DX,AL
                case 0x1cf: // IRET
                case 0x1ca: // RET
                case 0x1cb: // RET
                case 0x19a: // CALLF
                case 0x19c: // PUSHF
                case 0x19d: // POPF
                case 0x1ea: // JMPF
                case 0x1e0: // LOOPNE
                case 0x1e1: // LOOPE
                case 0x1e2: // LOOP
                case 0x1e3: // JCXZ
                    opcode &= 0xff;
                    break;
                case 0x163: // ARPL
                case 0x1d6: // -
                case 0x1f1: // -
                case 0x10f: // 2-byte instruction escape
                    opcode = ld8_direct();
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
                        x = ld16_direct();
                        if (can_jump(opcode & 0xf)) {
                            eip = (eip + far - far_start + x) & 0xffff, far = far_start = 0;
                        }
                        goto EXEC_LOOP;
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
                        modRM = ld8_direct();
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        if (can_jump(opcode & 0xf)) {
                            set_lower_word((modRM >> 3) & 7, rm);
                        }
                        goto EXEC_LOOP;
                    case 0x1b6: // MOVZX
                        modRM = ld8_direct();
                        reg = (modRM >> 3) & 7;
                        if ((modRM >> 6) == 3) {
                            rM = modRM & 7;
                            x = (regs[rM & 3] >> ((rM & 4) << 1)) & 0xff;
                        } else {
                            segment_translation();
                            x = ld8_readonly_cpl3();
                        }
                        set_lower_word(reg, x);
                        goto EXEC_LOOP;
                    case 0x1be: // MOVSX
                        modRM = ld8_direct();
                        reg = (modRM >> 3) & 7;
                        if ((modRM >> 6) == 3) {
                            rM = modRM & 7;
                            x = regs[rM & 3] >> ((rM & 4) << 1);
                        } else {
                            segment_translation();
                            x = ld8_readonly_cpl3();
                        }
                        set_lower_word(reg, ((x << 24) >> 24));
                        goto EXEC_LOOP;
                    case 0x1af: // IMUL
                        modRM = ld8_direct();
                        reg = (modRM >> 3) & 7;
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        aux_IMUL16(regs[reg], rm);
                        set_lower_word(reg, x);
                        goto EXEC_LOOP;
                    case 0x1c1: // XADD (80486)
                        operation = 0;
                        modRM = ld8_direct();
                        reg = (modRM >> 3) & 7;
                        if ((modRM >> 6) == 3) {
                            rM = modRM & 7;
                            x = regs[rM];
                            y = calculate16(x, regs[reg]);
                            set_lower_word(reg, x);
                            set_lower_word(rM, y);
                        } else {
                            segment_translation();
                            x = ld16_writable_cpl3();
                            y = calculate16(x, regs[reg]);
                            st16_writable_cpl3(y);
                            set_lower_word(reg, x);
                        }
                        goto EXEC_LOOP;
                    case 0x1a0: // PUSH FS
                    case 0x1a8: // PUSH GS
                        push_word(segs[(opcode >> 3) & 7].selector);
                        goto EXEC_LOOP;
                    case 0x1a1: // POP FS
                    case 0x1a9: // POP GS
                        x = read_stack_word();
                        pop_word();
                        set_segment_register((opcode >> 3) & 7, x);
                        goto EXEC_LOOP;
                    case 0x1b2: // LSS
                    case 0x1b4: // LFS
                    case 0x1b5: // LGS
                        ld_far_pointer16(opcode & 7);
                        goto EXEC_LOOP;
                    case 0x1a4: // SHLD
                    case 0x1ac: // SHRD
                        modRM = ld8_direct();
                        y = regs[(modRM >> 3) & 7];
                        operation = (opcode >> 3) & 1;
                        if ((modRM >> 6) == 3) {
                            z = ld8_direct();
                            rM = modRM & 7;
                            set_lower_word(rM, aux_SHRD16_SHLD16(regs[rM], y, z));
                        } else {
                            segment_translation();
                            z = ld8_direct();
                            x = ld16_writable_cpl3();
                            x = aux_SHRD16_SHLD16(x, y, z);
                            st16_writable_cpl3(x);
                        }
                        goto EXEC_LOOP;
                    case 0x1a5: // SHLD
                    case 0x1ad: // SHRD
                        modRM = ld8_direct();
                        y = regs[(modRM >> 3) & 7];
                        z = regs[1];
                        operation = (opcode >> 3) & 1;
                        if ((modRM >> 6) == 3) {
                            rM = modRM & 7;
                            set_lower_word(rM, aux_SHRD16_SHLD16(regs[rM], y, z));
                        } else {
                            segment_translation();
                            x = ld16_writable_cpl3();
                            x = aux_SHRD16_SHLD16(x, y, z);
                            st16_writable_cpl3(x);
                        }
                        goto EXEC_LOOP;
                    case 0x1ba: // G8 (-, -, -, -, BT, BTS, BTR, BTC)
                        modRM = ld8_direct();
                        operation = (modRM >> 3) & 7;
                        switch (operation) {
                        case 4:
                            if ((modRM >> 6) == 3) {
                                rm = regs[modRM & 7];
                                y = ld8_direct();
                            } else {
                                segment_translation();
                                y = ld8_direct();
                                rm = ld16_readonly_cpl3();
                            }
                            aux_BT16(rm, y);
                            break;
                        case 5:
                        case 6:
                        case 7:
                            operation &= 3;
                            if ((modRM >> 6) == 3) {
                                rM = modRM & 7;
                                y = ld8_direct();
                                regs[rM] = aux_BTS16_BTR16_BTC16(regs[rM], y);
                            } else {
                                segment_translation();
                                y = ld8_direct();
                                x = ld16_writable_cpl3();
                                x = aux_BTS16_BTR16_BTC16(x, y);
                                st16_writable_cpl3(x);
                            }
                            break;
                        default:
                            abort(6);
                        }
                        goto EXEC_LOOP;
                    case 0x1a3: // BT
                        modRM = ld8_direct();
                        y = regs[(modRM >> 3) & 7];
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            lax = lax + (((y & 0xffff) >> 4) << 1);
                            rm = ld16_readonly_cpl3();
                        }
                        aux_BT16(rm, y);
                        goto EXEC_LOOP;
                    case 0x1ab: // BTS
                    case 0x1b3: // BTR
                    case 0x1bb: // BTC
                        modRM = ld8_direct();
                        y = regs[(modRM >> 3) & 7];
                        operation = (opcode >> 3) & 3;
                        if ((modRM >> 6) == 3) {
                            rM = modRM & 7;
                            set_lower_word(rM, aux_BTS16_BTR16_BTC16(regs[rM], y));
                        } else {
                            segment_translation();
                            lax = lax + (((y & 0xffff) >> 4) << 1);
                            x = ld16_writable_cpl3();
                            x = aux_BTS16_BTR16_BTC16(x, y);
                            st16_writable_cpl3(x);
                        }
                        goto EXEC_LOOP;
                    case 0x1bc: // BSF
                    case 0x1bd: // BSR
                        modRM = ld8_direct();
                        reg = (modRM >> 3) & 7;
                        if ((modRM >> 6) == 3) {
                            rm = regs[modRM & 7];
                        } else {
                            segment_translation();
                            rm = ld16_readonly_cpl3();
                        }
                        x = regs[reg];
                        if (opcode & 1) {
                            x = aux_BSR16(x, rm);
                        } else {
                            x = aux_BSF16(x, rm);
                        }
                        set_lower_word(reg, x);
                        goto EXEC_LOOP;
                    case 0x1b1: // CMPXCHG (40486)
                        operation = 5;
                        modRM = ld8_direct();
                        reg = (modRM >> 3) & 7;
                        if ((modRM >> 6) == 3) {
                            rM = modRM & 7;
                            x = regs[rM];
                            y = calculate16(regs[0], x);
                            if (y == 0) {
                                set_lower_word(rM, regs[reg]);
                            } else {
                                set_lower_word(0, x);
                            }
                        } else {
                            segment_translation();
                            x = ld16_writable_cpl3();
                            y = calculate16(regs[0], x);
                            if (y == 0) {
                                st16_writable_cpl3(regs[reg]);
                            } else {
                                set_lower_word(0, x);
                            }
                        }
                        goto EXEC_LOOP;
                    case 0x100: // G6 (SLDT, STR, LLDT, LTR, VERR, VERW, -)
                    case 0x101: // G7 (SGDT, SIDT, LGDT, LIDT, SMSW, -, LMSW, -)
                    case 0x102: // LAR
                    case 0x103: // LSL
                    case 0x120: // MOV
                    case 0x122: // MOV
                    case 0x106: // CLTS
                    case 0x123: // MOV
                    case 0x1a2: // -
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
    EXEC_LOOP:;
    } while (--cycles_remaining);
OUTER_LOOP:
    this->cycles += cycles_requested - cycles_remaining;
    eip = eip + far - far_start;
}
