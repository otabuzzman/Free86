#include "x86.h"

void x86Internal::instruction(int cycles) {
    if (init(cycles)) {
        return;
    }
    do {
        check_opbyte();
        ipr = ipr_default;
        OPbyte |= ipr & 0x0100;
        while (true) {
            switch (OPbyte) {
            case 0x66: // operand-size override prefix
                if (ipr == ipr_default) {
                    operation_size_function(eip_linear, OPbyte);
                }
                if (ipr_default & 0x0100) {
                    ipr &= ~0x0100;
                } else {
                    ipr |= 0x0100;
                }
                OPbyte = phys_mem8[physmem8_ptr++];
                OPbyte |= (ipr & 0x0100);
                break;
            case 0x67: // address-size override prefix
                if (ipr == ipr_default) {
                    operation_size_function(eip_linear, OPbyte);
                }
                if (ipr_default & 0x0080) {
                    ipr &= ~0x0080;
                } else {
                    ipr |= 0x0080;
                }
                OPbyte = phys_mem8[physmem8_ptr++];
                OPbyte |= (ipr & 0x0100);
                break;
            case 0xf0: // LOCK prefix
                if (ipr == ipr_default) {
                    operation_size_function(eip_linear, OPbyte);
                }
                ipr |= 0x0040;
                OPbyte = phys_mem8[physmem8_ptr++];
                OPbyte |= (ipr & 0x0100);
                break;
            case 0xf2: // REPN[EZ] repeat string operation prefix
                if (ipr == ipr_default) {
                    operation_size_function(eip_linear, OPbyte);
                }
                ipr |= 0x0020;
                OPbyte = phys_mem8[physmem8_ptr++];
                OPbyte |= (ipr & 0x0100);
                break;
            case 0xf3: // REP[EZ] repeat string operation prefix
                if (ipr == ipr_default) {
                    operation_size_function(eip_linear, OPbyte);
                }
                ipr |= 0x0010;
                OPbyte = phys_mem8[physmem8_ptr++];
                OPbyte |= (ipr & 0x0100);
                break;
            case 0x26: // ES segment override prefix
            case 0x2e: // CS segment override prefix
            case 0x36: // SS segment override prefix
            case 0x3e: // DS segment override prefix
                if (ipr == ipr_default) {
                    operation_size_function(eip_linear, OPbyte);
                }
                ipr = (ipr & ~0x000f) | (((OPbyte >> 3) & 3) + 1);
                OPbyte = phys_mem8[physmem8_ptr++];
                OPbyte |= (ipr & 0x0100);
                break;
            case 0x64: // FS segment override prefix
            case 0x65: // GS segment override prefix
                if (ipr == ipr_default) {
                    operation_size_function(eip_linear, OPbyte);
                }
                ipr = (ipr & ~0x000f) | ((OPbyte & 7) + 1);
                OPbyte = phys_mem8[physmem8_ptr++];
                OPbyte |= (ipr & 0x0100);
                break;
            case 0xb0: // MOV AL
            case 0xb1: // MOV CL
            case 0xb2: // MOV DL
            case 0xb3: // MOV BL
            case 0xb4: // MOV AH
            case 0xb5: // MOV CH
            case 0xb6: // MOV DH
            case 0xb7: // MOV BH
                x = phys_mem8[physmem8_ptr++];
                OPbyte &= 7;
                last_tlb_val = (OPbyte & 4) << 1;
                regs[OPbyte & 3] = (regs[OPbyte & 3] & ~(0xff << last_tlb_val)) | (((x) & 0xff) << last_tlb_val);
                goto EXEC_LOOP;
            case 0xb8: // MOV A
            case 0xb9: // MOV C
            case 0xba: // MOV D
            case 0xbb: // MOV B
            case 0xbc: // MOV SP
            case 0xbd: // MOV BP
            case 0xbe: // MOV SI
            case 0xbf: // MOV DI
                x = phys_mem8[physmem8_ptr] |
                    (phys_mem8[physmem8_ptr + 1] << 8) |
                    (phys_mem8[physmem8_ptr + 2] << 16) |
                    (phys_mem8[physmem8_ptr + 3] << 24);
                physmem8_ptr += 4;
                regs[OPbyte & 7] = x;
                goto EXEC_LOOP;
            case 0x88: // MOV
                mem8 = phys_mem8[physmem8_ptr++];
                reg_idx1 = (mem8 >> 3) & 7;
                x = (regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1));
                if ((mem8 >> 6) == 3) {
                    reg_idx0 = mem8 & 7;
                    last_tlb_val = (reg_idx0 & 4) << 1;
                    regs[reg_idx0 & 3] = (regs[reg_idx0 & 3] & ~(0xff << last_tlb_val)) | (((x) & 0xff) << last_tlb_val);
                } else {
                    mem8_loc = segment_translation(mem8);
                    last_tlb_val = tlb_write[mem8_loc >> 12];
                    if (last_tlb_val == -1) {
                        __st8_mem8_write(x);
                    } else {
                        phys_mem8[mem8_loc ^ last_tlb_val] = x;
                    }
                }
                goto EXEC_LOOP;
            case 0x89: // MOV
                mem8 = phys_mem8[physmem8_ptr++];
                x = regs[(mem8 >> 3) & 7];
                if ((mem8 >> 6) == 3) {
                    regs[mem8 & 7] = x;
                } else {
                    mem8_loc = segment_translation(mem8);
                    last_tlb_val = tlb_write[mem8_loc >> 12];
                    if ((last_tlb_val | mem8_loc) & 3) {
                        __st32_mem8_write(x);
                    } else {
                        phys_mem32[(mem8_loc ^ last_tlb_val) >> 2] = x;
                    }
                }
                goto EXEC_LOOP;
            case 0x8a: // MOV
                mem8 = phys_mem8[physmem8_ptr++];
                if ((mem8 >> 6) == 3) {
                    reg_idx0 = mem8 & 7;
                    x = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                } else {
                    mem8_loc = segment_translation(mem8);
                    x = (((last_tlb_val = tlb_read[mem8_loc >> 12]) == -1)
                             ? __ld_8bits_mem8_read()
                             : phys_mem8[mem8_loc ^ last_tlb_val]);
                }
                reg_idx1 = (mem8 >> 3) & 7;
                last_tlb_val = (reg_idx1 & 4) << 1;
                regs[reg_idx1 & 3] = (regs[reg_idx1 & 3] & ~(0xff << last_tlb_val)) | (((x) & 0xff) << last_tlb_val);
                goto EXEC_LOOP;
            case 0x8b: // MOV
                mem8 = phys_mem8[physmem8_ptr++];
                if ((mem8 >> 6) == 3) {
                    x = regs[mem8 & 7];
                } else {
                    mem8_loc = segment_translation(mem8);
                    last_tlb_val = tlb_read[mem8_loc >> 12];
                    x = ((last_tlb_val | mem8_loc) & 3
                             ? __ld_32bits_mem8_read()
                             : phys_mem32[(mem8_loc ^ last_tlb_val) >> 2]);
                }
                regs[(mem8 >> 3) & 7] = x;
                goto EXEC_LOOP;
            case 0xa0: // MOV AL,
                mem8_loc = segmented_mem8_loc_for_MOV(false);
                x = ld_8bits_mem8_read();
                regs[0] = (regs[0] & -256) | x;
                goto EXEC_LOOP;
            case 0xa1: // MOV AX,
                mem8_loc = segmented_mem8_loc_for_MOV(false);
                x = ld_32bits_mem8_read();
                regs[0] = x;
                goto EXEC_LOOP;
            case 0xa2: // MOV ,AL
                mem8_loc = segmented_mem8_loc_for_MOV(true);
                st8_mem8_write(regs[0]);
                goto EXEC_LOOP;
            case 0xa3: // MOV ,AX
                if (ipr & 0x0040) { // test386, check LOCK prefix (not allowed)
                    abort(6);
                }
                mem8_loc = segmented_mem8_loc_for_MOV(true);
                st32_mem8_write(regs[0]);
                goto EXEC_LOOP;
            case 0xd7: // XLAT
                mem8_loc = (regs[3] + (regs[0] & 0xff)) >> 0;
                if (ipr & 0x0080) {
                    mem8_loc &= 0xffff;
                }
                reg_idx1 = ipr & 0x000f;
                if (reg_idx1 == 0) {
                    reg_idx1 = 3;
                } else {
                    reg_idx1--;
                }
                mem8_loc = (mem8_loc + segs[reg_idx1].base) >> 0;
                x = ld_8bits_mem8_read();
                set_word_in_register(0, x);
                goto EXEC_LOOP;
            case 0xc6: // MOV
                mem8 = phys_mem8[physmem8_ptr++];
                if ((mem8 >> 6) == 3) {
                    x = phys_mem8[physmem8_ptr++];
                    set_word_in_register(mem8 & 7, x);
                } else {
                    mem8_loc = segment_translation(mem8);
                    x = phys_mem8[physmem8_ptr++];
                    st8_mem8_write(x);
                }
                goto EXEC_LOOP;
            case 0xc7: // MOV
                mem8 = phys_mem8[physmem8_ptr++];
                if ((mem8 >> 6) == 3) {
                    x = phys_mem8[physmem8_ptr] |
                        (phys_mem8[physmem8_ptr + 1] << 8) |
                        (phys_mem8[physmem8_ptr + 2] << 16) |
                        (phys_mem8[physmem8_ptr + 3] << 24);
                    physmem8_ptr += 4;
                    regs[mem8 & 7] = x;
                } else {
                    mem8_loc = segment_translation(mem8);
                    x = phys_mem8[physmem8_ptr] |
                        (phys_mem8[physmem8_ptr + 1] << 8) |
                        (phys_mem8[physmem8_ptr + 2] << 16) |
                        (phys_mem8[physmem8_ptr + 3] << 24);
                    physmem8_ptr += 4;
                    st32_mem8_write(x);
                }
                goto EXEC_LOOP;
            case 0x91: // XCHG C
            case 0x92: // XCHG D
            case 0x93: // XCHG B
            case 0x94: // XCHG SP
            case 0x95: // XCHG BP
            case 0x96: // XCHG SI
            case 0x97: // XCHG DI
                reg_idx1 = OPbyte & 7;
                x = regs[0];
                regs[0] = regs[reg_idx1];
                regs[reg_idx1] = x;
                goto EXEC_LOOP;
            case 0x86: // XCHG
                mem8 = phys_mem8[physmem8_ptr++];
                reg_idx1 = (mem8 >> 3) & 7;
                if ((mem8 >> 6) == 3) {
                    reg_idx0 = mem8 & 7;
                    x = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                    set_word_in_register(reg_idx0, (regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1)));
                } else {
                    mem8_loc = segment_translation(mem8);
                    x = ld_8bits_mem8_write();
                    st8_mem8_write((regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1)));
                }
                set_word_in_register(reg_idx1, x);
                goto EXEC_LOOP;
            case 0x87: // XCHG
                mem8 = phys_mem8[physmem8_ptr++];
                reg_idx1 = (mem8 >> 3) & 7;
                if ((mem8 >> 6) == 3) {
                    reg_idx0 = mem8 & 7;
                    x = regs[reg_idx0];
                    regs[reg_idx0] = regs[reg_idx1];
                } else {
                    mem8_loc = segment_translation(mem8);
                    x = ld_32bits_mem8_write();
                    st32_mem8_write(regs[reg_idx1]);
                }
                regs[reg_idx1] = x;
                goto EXEC_LOOP;
            case 0x8e: // MOV
                mem8 = phys_mem8[physmem8_ptr++];
                reg_idx1 = (mem8 >> 3) & 7;
                if (reg_idx1 >= 6 || reg_idx1 == 1) {
                    abort(6);
                }
                if ((mem8 >> 6) == 3) {
                    x = regs[mem8 & 7] & 0xffff;
                } else {
                    mem8_loc = segment_translation(mem8);
                    x = ld_16bits_mem8_read();
                }
                set_segment_register(reg_idx1, x);
                goto EXEC_LOOP;
            case 0x8c: // MOV
                mem8 = phys_mem8[physmem8_ptr++];
                reg_idx1 = (mem8 >> 3) & 7;
                if (reg_idx1 >= 6) {
                    abort(6);
                }
                x = segs[reg_idx1].selector;
                if ((mem8 >> 6) == 3) {
                    if ((((ipr >> 8) & 1) ^ 1)) {
                        regs[mem8 & 7] = x;
                    } else {
                        set_lower_word_in_register(mem8 & 7, x);
                    }
                } else {
                    mem8_loc = segment_translation(mem8);
                    st16_mem8_write(x);
                }
                goto EXEC_LOOP;
            case 0xc4: // LES
                op_16_load_far_pointer32(0);
                goto EXEC_LOOP;
            case 0xc5: // LDS
                op_16_load_far_pointer32(3);
                goto EXEC_LOOP;
            case 0x00: // ADD
            case 0x08: // OR
            case 0x10: // ADC
            case 0x18: // SBB
            case 0x20: // AND
            case 0x28: // SUB
            case 0x30: // XOR
            case 0x38: // CMP
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = OPbyte >> 3;
                reg_idx1 = (mem8 >> 3) & 7;
                y = (regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1));
                if ((mem8 >> 6) == 3) {
                    reg_idx0 = mem8 & 7;
                    set_word_in_register(reg_idx0, do_8bit_math(conditional_var, (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1)), y));
                } else {
                    mem8_loc = segment_translation(mem8);
                    if (conditional_var != 7) {
                        x = ld_8bits_mem8_write();
                        x = do_8bit_math(conditional_var, x, y);
                        st8_mem8_write(x);
                    } else {
                        x = ld_8bits_mem8_read();
                        do_8bit_math(7, x, y);
                    }
                }
                goto EXEC_LOOP;
            case 0x01: // ADD
                mem8 = phys_mem8[physmem8_ptr++];
                y = regs[(mem8 >> 3) & 7];
                if ((mem8 >> 6) == 3) {
                    reg_idx0 = mem8 & 7;
                    osm_src = y;
                    osm_dst = regs[reg_idx0] = (regs[reg_idx0] + osm_src) >> 0;
                    osm = 2;
                } else {
                    mem8_loc = segment_translation(mem8);
                    x = ld_32bits_mem8_write();
                    osm_src = y;
                    osm_dst = x = (x + osm_src) >> 0;
                    osm = 2;
                    st32_mem8_write(x);
                }
                goto EXEC_LOOP;
            case 0x09: // OR
            case 0x11: // ADC
            case 0x19: // SBB
            case 0x21: // AND
            case 0x29: // SUB
            case 0x31: // XOR
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = OPbyte >> 3;
                y = regs[(mem8 >> 3) & 7];
                if ((mem8 >> 6) == 3) {
                    reg_idx0 = mem8 & 7;
                    regs[reg_idx0] = do_32bit_math(conditional_var, regs[reg_idx0], y);
                } else {
                    mem8_loc = segment_translation(mem8);
                    x = ld_32bits_mem8_write();
                    x = do_32bit_math(conditional_var, x, y);
                    st32_mem8_write(x);
                }
                goto EXEC_LOOP;
            case 0x39: // CMP
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = OPbyte >> 3;
                y = regs[(mem8 >> 3) & 7];
                if ((mem8 >> 6) == 3) {
                    reg_idx0 = mem8 & 7;
                    osm_src = y;
                    osm_dst = (regs[reg_idx0] - osm_src) >> 0;
                    osm = 8;
                } else {
                    mem8_loc = segment_translation(mem8);
                    x = ld_32bits_mem8_read();
                    osm_src = y;
                    osm_dst = (x - osm_src) >> 0;
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
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = OPbyte >> 3;
                reg_idx1 = (mem8 >> 3) & 7;
                if ((mem8 >> 6) == 3) {
                    reg_idx0 = mem8 & 7;
                    y = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                } else {
                    mem8_loc = segment_translation(mem8);
                    y = ld_8bits_mem8_read();
                }
                set_word_in_register(reg_idx1, do_8bit_math(conditional_var, (regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1)), y));
                goto EXEC_LOOP;
            case 0x03: // ADD
                mem8 = phys_mem8[physmem8_ptr++];
                reg_idx1 = (mem8 >> 3) & 7;
                if ((mem8 >> 6) == 3) {
                    y = regs[mem8 & 7];
                } else {
                    mem8_loc = segment_translation(mem8);
                    y = ld_32bits_mem8_read();
                }
                osm_src = y;
                osm_dst = regs[reg_idx1] = (regs[reg_idx1] + osm_src) >> 0;
                osm = 2;
                goto EXEC_LOOP;
            case 0x0b: // OR
            case 0x13: // ADC
            case 0x1b: // SBB
            case 0x23: // AND
            case 0x2b: // SUB
            case 0x33: // XOR
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = OPbyte >> 3;
                reg_idx1 = (mem8 >> 3) & 7;
                if ((mem8 >> 6) == 3) {
                    y = regs[mem8 & 7];
                } else {
                    mem8_loc = segment_translation(mem8);
                    y = ld_32bits_mem8_read();
                }
                regs[reg_idx1] = do_32bit_math(conditional_var, regs[reg_idx1], y);
                goto EXEC_LOOP;
            case 0x3b: // CMP
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = OPbyte >> 3;
                reg_idx1 = (mem8 >> 3) & 7;
                if ((mem8 >> 6) == 3) {
                    y = regs[mem8 & 7];
                } else {
                    mem8_loc = segment_translation(mem8);
                    y = ld_32bits_mem8_read();
                }
                osm_src = y;
                osm_dst = (regs[reg_idx1] - osm_src) >> 0;
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
                y = phys_mem8[physmem8_ptr++];
                conditional_var = OPbyte >> 3;
                set_word_in_register(0, do_8bit_math(conditional_var, regs[0] & 0xff, y));
                goto EXEC_LOOP;
            case 0x05: // ADD
                y = phys_mem8[physmem8_ptr] |
                    (phys_mem8[physmem8_ptr + 1] << 8) |
                    (phys_mem8[physmem8_ptr + 2] << 16) |
                    (phys_mem8[physmem8_ptr + 3] << 24);
                physmem8_ptr += 4;
                osm_src = y;
                osm_dst = regs[0] = (regs[0] + osm_src) >> 0;
                osm = 2;
                goto EXEC_LOOP;
            case 0x0d: // OR
            case 0x15: // ADC
            case 0x1d: // SBB
            case 0x25: // AND
            case 0x2d: // SUB
                y = phys_mem8[physmem8_ptr] |
                    (phys_mem8[physmem8_ptr + 1] << 8) |
                    (phys_mem8[physmem8_ptr + 2] << 16) |
                    (phys_mem8[physmem8_ptr + 3] << 24);
                physmem8_ptr += 4;
                conditional_var = OPbyte >> 3;
                regs[0] = do_32bit_math(conditional_var, regs[0], y);
                goto EXEC_LOOP;
            case 0x35: // XOR
                y = phys_mem8[physmem8_ptr] |
                    (phys_mem8[physmem8_ptr + 1] << 8) |
                    (phys_mem8[physmem8_ptr + 2] << 16) |
                    (phys_mem8[physmem8_ptr + 3] << 24);
                physmem8_ptr += 4;
                osm_dst = regs[0] = regs[0] ^ y;
                osm = 14;
                goto EXEC_LOOP;
            case 0x3d: // CMP
                y = phys_mem8[physmem8_ptr] |
                    (phys_mem8[physmem8_ptr + 1] << 8) |
                    (phys_mem8[physmem8_ptr + 2] << 16) |
                    (phys_mem8[physmem8_ptr + 3] << 24);
                physmem8_ptr += 4;
                osm_src = y;
                osm_dst = (regs[0] - osm_src) >> 0;
                osm = 8;
                goto EXEC_LOOP;
            case 0x80: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
            case 0x82: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = (mem8 >> 3) & 7;
                if ((mem8 >> 6) == 3) {
                    reg_idx0 = mem8 & 7;
                    y = phys_mem8[physmem8_ptr++];
                    set_word_in_register(reg_idx0, do_8bit_math(conditional_var, (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1)), y));
                } else {
                    mem8_loc = segment_translation(mem8);
                    y = phys_mem8[physmem8_ptr++];
                    if (conditional_var != 7) {
                        x = ld_8bits_mem8_write();
                        x = do_8bit_math(conditional_var, x, y);
                        st8_mem8_write(x);
                    } else {
                        x = ld_8bits_mem8_read();
                        do_8bit_math(7, x, y);
                    }
                }
                goto EXEC_LOOP;
            case 0x81: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = (mem8 >> 3) & 7;
                if (conditional_var == 7) {
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_read();
                    }
                    y = phys_mem8[physmem8_ptr] |
                        (phys_mem8[physmem8_ptr + 1] << 8) |
                        (phys_mem8[physmem8_ptr + 2] << 16) |
                        (phys_mem8[physmem8_ptr + 3] << 24);
                    physmem8_ptr += 4;
                    osm_src = y;
                    osm_dst = (x - osm_src) >> 0;
                    osm = 8;
                } else {
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        y = phys_mem8[physmem8_ptr] |
                            (phys_mem8[physmem8_ptr + 1] << 8) |
                            (phys_mem8[physmem8_ptr + 2] << 16) |
                            (phys_mem8[physmem8_ptr + 3] << 24);
                        physmem8_ptr += 4;
                        regs[reg_idx0] = do_32bit_math(conditional_var, regs[reg_idx0], y);
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y = phys_mem8[physmem8_ptr] |
                            (phys_mem8[physmem8_ptr + 1] << 8) |
                            (phys_mem8[physmem8_ptr + 2] << 16) |
                            (phys_mem8[physmem8_ptr + 3] << 24);
                        physmem8_ptr += 4;
                        x = ld_32bits_mem8_write();
                        x = do_32bit_math(conditional_var, x, y);
                        st32_mem8_write(x);
                    }
                }
                goto EXEC_LOOP;
            case 0x83: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = (mem8 >> 3) & 7;
                if (conditional_var == 7) {
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_read();
                    }
                    y = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    osm_src = y;
                    osm_dst = (x - osm_src) >> 0;
                    osm = 8;
                } else {
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        y = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        regs[reg_idx0] = do_32bit_math(conditional_var, regs[reg_idx0], y);
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        x = ld_32bits_mem8_write();
                        x = do_32bit_math(conditional_var, x, y);
                        st32_mem8_write(x);
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
                reg_idx1 = OPbyte & 7;
                if (osm < 25) {
                    ocm_preserved = osm;
                    ocm_dst_preserved = osm_dst;
                }
                regs[reg_idx1] = osm_dst = (regs[reg_idx1] + 1) >> 0;
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
                reg_idx1 = OPbyte & 7;
                if (osm < 25) {
                    ocm_preserved = osm;
                    ocm_dst_preserved = osm_dst;
                }
                regs[reg_idx1] = osm_dst = (regs[reg_idx1] - 1) >> 0;
                osm = 30;
                goto EXEC_LOOP;
            case 0x6b: // IMUL
                mem8 = phys_mem8[physmem8_ptr++];
                reg_idx1 = (mem8 >> 3) & 7;
                if ((mem8 >> 6) == 3) {
                    y = regs[mem8 & 7];
                } else {
                    mem8_loc = segment_translation(mem8);
                    y = ld_32bits_mem8_read();
                }
                z = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                regs[reg_idx1] = op_IMUL32(y, z);
                goto EXEC_LOOP;
            case 0x69: // IMUL
                mem8 = phys_mem8[physmem8_ptr++];
                reg_idx1 = (mem8 >> 3) & 7;
                if ((mem8 >> 6) == 3) {
                    y = regs[mem8 & 7];
                } else {
                    mem8_loc = segment_translation(mem8);
                    y = ld_32bits_mem8_read();
                }
                z = phys_mem8[physmem8_ptr] |
                    (phys_mem8[physmem8_ptr + 1] << 8) |
                    (phys_mem8[physmem8_ptr + 2] << 16) |
                    (phys_mem8[physmem8_ptr + 3] << 24);
                physmem8_ptr += 4;
                regs[reg_idx1] = op_IMUL32(y, z);
                goto EXEC_LOOP;
            case 0x84: // TEST
                mem8 = phys_mem8[physmem8_ptr++];
                if ((mem8 >> 6) == 3) {
                    reg_idx0 = mem8 & 7;
                    x = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                } else {
                    mem8_loc = segment_translation(mem8);
                    x = ld_8bits_mem8_read();
                }
                reg_idx1 = (mem8 >> 3) & 7;
                y = (regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1));
                osm_dst = (((x & y) << 24) >> 24);
                osm = 12;
                goto EXEC_LOOP;
            case 0x85: // TEST
                mem8 = phys_mem8[physmem8_ptr++];
                if ((mem8 >> 6) == 3) {
                    x = regs[mem8 & 7];
                } else {
                    mem8_loc = segment_translation(mem8);
                    x = ld_32bits_mem8_read();
                }
                y = regs[(mem8 >> 3) & 7];
                osm_dst = x & y;
                osm = 14;
                goto EXEC_LOOP;
            case 0xa8: // TEST
                y = phys_mem8[physmem8_ptr++];
                osm_dst = (((regs[0] & y) << 24) >> 24);
                osm = 12;
                goto EXEC_LOOP;
            case 0xa9: // TEST
                y = phys_mem8[physmem8_ptr] |
                    (phys_mem8[physmem8_ptr + 1] << 8) |
                    (phys_mem8[physmem8_ptr + 2] << 16) |
                    (phys_mem8[physmem8_ptr + 3] << 24);
                physmem8_ptr += 4;
                osm_dst = regs[0] & y;
                osm = 14;
                goto EXEC_LOOP;
            case 0xf6: // G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, // IDIV AL/X)
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = (mem8 >> 3) & 7;
                switch (conditional_var) {
                case 0:
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        x = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_8bits_mem8_read();
                    }
                    y = phys_mem8[physmem8_ptr++];
                    osm_dst = (((x & y) << 24) >> 24);
                    osm = 12;
                    break;
                case 2:
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        set_word_in_register(reg_idx0, ~(regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1)));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_8bits_mem8_write();
                        x = ~x;
                        st8_mem8_write(x);
                    }
                    break;
                case 3:
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        set_word_in_register(reg_idx0, do_8bit_math(5, 0, (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1))));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_8bits_mem8_write();
                        x = do_8bit_math(5, 0, x);
                        st8_mem8_write(x);
                    }
                    break;
                case 4:
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        x = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_8bits_mem8_read();
                    }
                    set_lower_word_in_register(0, op_MUL(regs[0], x));
                    break;
                case 5:
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        x = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_8bits_mem8_read();
                    }
                    set_lower_word_in_register(0, op_IMUL(regs[0], x));
                    break;
                case 6:
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        x = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_8bits_mem8_read();
                    }
                    op_DIV(x);
                    break;
                case 7:
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        x = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_8bits_mem8_read();
                    }
                    op_IDIV(x);
                    break;
                default:
                    abort(6);
                }
                goto EXEC_LOOP;
            case 0xf7: // G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = (mem8 >> 3) & 7;
                switch (conditional_var) {
                case 0:
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_read();
                    }
                    y = phys_mem8[physmem8_ptr] |
                        (phys_mem8[physmem8_ptr + 1] << 8) |
                        (phys_mem8[physmem8_ptr + 2] << 16) |
                        (phys_mem8[physmem8_ptr + 3] << 24);
                    physmem8_ptr += 4;
                    osm_dst = x & y;
                    osm = 14;
                    break;
                case 2:
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        regs[reg_idx0] = ~regs[reg_idx0];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_write();
                        x = ~x;
                        st32_mem8_write(x);
                    }
                    break;
                case 3:
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        regs[reg_idx0] = do_32bit_math(5, 0, regs[reg_idx0]);
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_write();
                        x = do_32bit_math(5, 0, x);
                        st32_mem8_write(x);
                    }
                    break;
                case 4:
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_read();
                    }
                    regs[0] = op_MUL32(regs[0], x);
                    regs[2] = v;
                    break;
                case 5:
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_read();
                    }
                    regs[0] = op_IMUL32(regs[0], x);
                    regs[2] = v;
                    break;
                case 6:
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_read();
                    }
                    regs[0] = op_DIV32(regs[2], regs[0], x);
                    regs[2] = v;
                    break;
                case 7:
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_read();
                    }
                    regs[0] = op_IDIV32(regs[2], regs[0], x);
                    regs[2] = v;
                    break;
                default:
                    abort(6);
                }
                goto EXEC_LOOP;
            case 0xc0: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = (mem8 >> 3) & 7;
                if ((mem8 >> 6) == 3) {
                    y = phys_mem8[physmem8_ptr++];
                    reg_idx0 = mem8 & 7;
                    set_word_in_register(reg_idx0, shift8(conditional_var, (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1)), y));
                } else {
                    mem8_loc = segment_translation(mem8);
                    y = phys_mem8[physmem8_ptr++];
                    x = ld_8bits_mem8_write();
                    x = shift8(conditional_var, x, y);
                    st8_mem8_write(x);
                }
                goto EXEC_LOOP;
            case 0xc1: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = (mem8 >> 3) & 7;
                if ((mem8 >> 6) == 3) {
                    y = phys_mem8[physmem8_ptr++];
                    reg_idx0 = mem8 & 7;
                    regs[reg_idx0] = shift32(conditional_var, regs[reg_idx0], y);
                } else {
                    mem8_loc = segment_translation(mem8);
                    y = phys_mem8[physmem8_ptr++];
                    x = ld_32bits_mem8_write();
                    x = shift32(conditional_var, x, y);
                    st32_mem8_write(x);
                }
                goto EXEC_LOOP;
            case 0xd0: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = (mem8 >> 3) & 7;
                if ((mem8 >> 6) == 3) {
                    reg_idx0 = mem8 & 7;
                    set_word_in_register(reg_idx0, shift8(conditional_var, (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1)), 1));
                } else {
                    mem8_loc = segment_translation(mem8);
                    x = ld_8bits_mem8_write();
                    x = shift8(conditional_var, x, 1);
                    st8_mem8_write(x);
                }
                goto EXEC_LOOP;
            case 0xd1: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = (mem8 >> 3) & 7;
                if ((mem8 >> 6) == 3) {
                    reg_idx0 = mem8 & 7;
                    regs[reg_idx0] = shift32(conditional_var, regs[reg_idx0], 1);
                } else {
                    mem8_loc = segment_translation(mem8);
                    x = ld_32bits_mem8_write();
                    x = shift32(conditional_var, x, 1);
                    st32_mem8_write(x);
                }
                goto EXEC_LOOP;
            case 0xd2: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = (mem8 >> 3) & 7;
                y = regs[1] & 0xff;
                if ((mem8 >> 6) == 3) {
                    reg_idx0 = mem8 & 7;
                    set_word_in_register(reg_idx0, shift8(conditional_var, (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1)), y));
                } else {
                    mem8_loc = segment_translation(mem8);
                    x = ld_8bits_mem8_write();
                    x = shift8(conditional_var, x, y);
                    st8_mem8_write(x);
                }
                goto EXEC_LOOP;
            case 0xd3: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = (mem8 >> 3) & 7;
                y = regs[1] & 0xff;
                if ((mem8 >> 6) == 3) {
                    reg_idx0 = mem8 & 7;
                    regs[reg_idx0] = shift32(conditional_var, regs[reg_idx0], y);
                } else {
                    mem8_loc = segment_translation(mem8);
                    x = ld_32bits_mem8_write();
                    x = shift32(conditional_var, x, y);
                    st32_mem8_write(x);
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
                x = regs[OPbyte & 7];
                if (x86_64_long_mode) {
                    mem8_loc = (regs[4] - 4) >> 0;
                    last_tlb_val = tlb_write[mem8_loc >> 12];
                    if ((last_tlb_val | mem8_loc) & 3) {
                        __st32_mem8_write(x);
                    } else {
                        phys_mem32[(mem8_loc ^ last_tlb_val) >> 2] = x;
                    }
                    regs[4] = mem8_loc;
                } else {
                    push_dword_to_stack(x);
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
                    mem8_loc = regs[4];
                    last_tlb_val = tlb_read[mem8_loc >> 12];
                    if ((last_tlb_val | mem8_loc) & 3) {
                        x = __ld_32bits_mem8_read();
                    } else {
                        x = phys_mem32[(mem8_loc ^ last_tlb_val) >> 2];
                    }
                    regs[4] = (mem8_loc + 4) >> 0;
                } else {
                    x = pop_dword_from_stack_read();
                    pop_dword_from_stack_incr_ptr();
                }
                regs[OPbyte & 7] = x;
                goto EXEC_LOOP;
            case 0x60: // PUSHA
                op_PUSHA();
                goto EXEC_LOOP;
            case 0x61: // POPA
                op_POPA();
                goto EXEC_LOOP;
            case 0x8f: // POP
                mem8 = phys_mem8[physmem8_ptr++];
                if ((mem8 >> 6) == 3) {
                    x = pop_dword_from_stack_read();
                    pop_dword_from_stack_incr_ptr();
                    regs[mem8 & 7] = x;
                } else {
                    x = pop_dword_from_stack_read();
                    y = regs[4];
                    pop_dword_from_stack_incr_ptr();
                    z = regs[4];
                    mem8_loc = segment_translation(mem8);
                    regs[4] = y;
                    st32_mem8_write(x);
                    regs[4] = z;
                }
                goto EXEC_LOOP;
            case 0x68: // PUSH
                x = phys_mem8[physmem8_ptr] |
                    (phys_mem8[physmem8_ptr + 1] << 8) |
                    (phys_mem8[physmem8_ptr + 2] << 16) |
                    (phys_mem8[physmem8_ptr + 3] << 24);
                physmem8_ptr += 4;
                if (x86_64_long_mode) {
                    mem8_loc = (regs[4] - 4) >> 0;
                    st32_mem8_write(x);
                    regs[4] = mem8_loc;
                } else {
                    push_dword_to_stack(x);
                }
                goto EXEC_LOOP;
            case 0x6a: // PUSH
                x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                if (x86_64_long_mode) {
                    mem8_loc = (regs[4] - 4) >> 0;
                    st32_mem8_write(x);
                    regs[4] = mem8_loc;
                } else {
                    push_dword_to_stack(x);
                }
                goto EXEC_LOOP;
            case 0xc8: // ENTER
                op_ENTER();
                goto EXEC_LOOP;
            case 0xc9: // LEAVE
                if (x86_64_long_mode) {
                    mem8_loc = regs[5];
                    x = ld_32bits_mem8_read();
                    regs[5] = x;
                    regs[4] = (mem8_loc + 4) >> 0;
                } else {
                    op_LEAVE();
                }
                goto EXEC_LOOP;
            case 0x9c: // PUSHF
                iopl = (eflags >> 12) & 3;
                if ((eflags & 0x00020000) && iopl != 3) {
                    abort(13);
                }
                x = get_FLAGS() & ~(0x00010000 | 0x00020000);
                if ((((ipr >> 8) & 1) ^ 1)) {
                    push_dword_to_stack(x);
                } else {
                    push_word_to_stack(x);
                }
                goto EXEC_LOOP;
            case 0x9d: // POPF
                iopl = (eflags >> 12) & 3;
                if ((eflags & 0x00020000) && iopl != 3) {
                    abort(13);
                }
                if ((((ipr >> 8) & 1) ^ 1)) {
                    x = pop_dword_from_stack_read();
                    pop_dword_from_stack_incr_ptr();
                    y = -1;
                } else {
                    x = pop_word_from_stack_read();
                    pop_word_from_stack_incr_ptr();
                    y = 0xffff;
                }
                z = (0x00000100 | 0x00004000 | 0x00040000 | 0x00200000);
                if (cpl == 0) {
                    z |= 0x00000200 | 0x00003000;
                } else {
                    if (cpl <= iopl) {
                        z |= 0x00000200;
                    }
                }
                set_FLAGS(x, z & y);
                if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0x06: // PUSH
            case 0x0e: // PUSH
            case 0x16: // PUSH
            case 0x1e: // PUSH
                push_dword_to_stack(segs[OPbyte >> 3].selector);
                goto EXEC_LOOP;
            case 0x07: // POP
            case 0x17: // POP
            case 0x1f: // POP
                x = pop_dword_from_stack_read() & 0xffff;
                pop_dword_from_stack_incr_ptr();
                set_segment_register(OPbyte >> 3, x);
                goto EXEC_LOOP;
            case 0x8d: // LEA
                mem8 = phys_mem8[physmem8_ptr++];
                if ((mem8 >> 6) == 3) {
                    abort(6);
                }
                ipr = (ipr & ~0x000f) | (6 + 1);
                regs[(mem8 >> 3) & 7] = segment_translation(mem8);
                goto EXEC_LOOP;
            case 0xfe: // G4 (INC, DEC, -, -, -, -, -)
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = (mem8 >> 3) & 7;
                switch (conditional_var) {
                case 0:
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        set_word_in_register(reg_idx0, increment_8bit((regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1))));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_8bits_mem8_write();
                        x = increment_8bit(x);
                        st8_mem8_write(x);
                    }
                    break;
                case 1:
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        set_word_in_register(reg_idx0, decrement_8bit((regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1))));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_8bits_mem8_write();
                        x = decrement_8bit(x);
                        st8_mem8_write(x);
                    }
                    break;
                default:
                    abort(6);
                }
                goto EXEC_LOOP;
            case 0xff: // G5 (INC, DEC, CALL, CALL, JMP, JMP, PUSH, -)
                mem8 = phys_mem8[physmem8_ptr++];
                conditional_var = (mem8 >> 3) & 7;
                switch (conditional_var) {
                case 0: // INC
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        if (osm < 25) {
                            ocm_preserved = osm;
                            ocm_dst_preserved = osm_dst;
                        }
                        regs[reg_idx0] = osm_dst = (regs[reg_idx0] + 1) >> 0;
                        osm = 27;
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_write();
                        if (osm < 25) {
                            ocm_preserved = osm;
                            ocm_dst_preserved = osm_dst;
                        }
                        x = osm_dst = (x + 1) >> 0;
                        osm = 27;
                        st32_mem8_write(x);
                    }
                    break;
                case 1: // DEC
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        if (osm < 25) {
                            ocm_preserved = osm;
                            ocm_dst_preserved = osm_dst;
                        }
                        regs[reg_idx0] = osm_dst = (regs[reg_idx0] - 1) >> 0;
                        osm = 30;
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_write();
                        if (osm < 25) {
                            ocm_preserved = osm;
                            ocm_dst_preserved = osm_dst;
                        }
                        x = osm_dst = (x - 1) >> 0;
                        osm = 30;
                        st32_mem8_write(x);
                    }
                    break;
                case 2: // CALL
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_read();
                    }
                    y = (eip + physmem8_ptr - initial_mem_ptr);
                    if (x86_64_long_mode) {
                        mem8_loc = (regs[4] - 4) >> 0;
                        st32_mem8_write(y);
                        regs[4] = mem8_loc;
                    } else {
                        push_dword_to_stack(y);
                    }
                    eip = x, physmem8_ptr = initial_mem_ptr = 0;
                    break;
                case 4: // JMP
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_read();
                    }
                    eip = x, physmem8_ptr = initial_mem_ptr = 0;
                    break;
                case 6: // PUSH
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_read();
                    }
                    if (x86_64_long_mode) {
                        mem8_loc = (regs[4] - 4) >> 0;
                        st32_mem8_write(x);
                        regs[4] = mem8_loc;
                    } else {
                        push_dword_to_stack(x);
                    }
                    break;
                case 3: // CALLF
                case 5: // JMPF
                    if ((mem8 >> 6) == 3) {
                        abort(6);
                    }
                    mem8_loc = segment_translation(mem8);
                    x = ld_32bits_mem8_read();
                    mem8_loc = (mem8_loc + 4) >> 0;
                    y = ld_16bits_mem8_read();
                    if (conditional_var == 3) {
                        op_CALLF(1, y, x, (eip + physmem8_ptr - initial_mem_ptr));
                    } else {
                        op_JMPF(y, x);
                    }
                    break;
                default:
                    abort(6);
                }
                goto EXEC_LOOP;
            case 0xeb: // JMP
                x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                physmem8_ptr = (physmem8_ptr + x) >> 0;
                goto EXEC_LOOP;
            case 0xe9: // JMP
                x = phys_mem8[physmem8_ptr] |
                    (phys_mem8[physmem8_ptr + 1] << 8) |
                    (phys_mem8[physmem8_ptr + 2] << 16) |
                    (phys_mem8[physmem8_ptr + 3] << 24);
                physmem8_ptr += 4;
                physmem8_ptr = (physmem8_ptr + x) >> 0;
                goto EXEC_LOOP;
            case 0xea: // JMPF
                if ((((ipr >> 8) & 1) ^ 1)) {
                    {
                        x = phys_mem8[physmem8_ptr] |
                            (phys_mem8[physmem8_ptr + 1] << 8) |
                            (phys_mem8[physmem8_ptr + 2] << 16) |
                            (phys_mem8[physmem8_ptr + 3] << 24);
                        physmem8_ptr += 4;
                    }
                } else {
                    x = ld16_mem8_direct();
                }
                y = ld16_mem8_direct();
                op_JMPF(y, x);
                goto EXEC_LOOP;
            case 0x70: // JO
                if (check_overflow()) {
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                } else {
                    physmem8_ptr = (physmem8_ptr + 1) >> 0;
                }
                goto EXEC_LOOP;
            case 0x71: // JNO
                if (!check_overflow()) {
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                } else {
                    physmem8_ptr = (physmem8_ptr + 1) >> 0;
                }
                goto EXEC_LOOP;
            case 0x72: // JB
                if (check_carry()) {
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                } else {
                    physmem8_ptr = (physmem8_ptr + 1) >> 0;
                }
                goto EXEC_LOOP;
            case 0x73: // JNB
                if (!check_carry()) {
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                } else {
                    physmem8_ptr = (physmem8_ptr + 1) >> 0;
                }
                goto EXEC_LOOP;
            case 0x74: // JZ
                if (osm_dst == 0) {
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                } else {
                    physmem8_ptr = (physmem8_ptr + 1) >> 0;
                }
                goto EXEC_LOOP;
            case 0x75: // JNZ
                if (!(osm_dst == 0)) {
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                } else {
                    physmem8_ptr = (physmem8_ptr + 1) >> 0;
                }
                goto EXEC_LOOP;
            case 0x76: // JBE
                if (check_below_or_equal()) {
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                } else {
                    physmem8_ptr = (physmem8_ptr + 1) >> 0;
                }
                goto EXEC_LOOP;
            case 0x77: // JNBE
                if (!check_below_or_equal()) {
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                } else {
                    physmem8_ptr = (physmem8_ptr + 1) >> 0;
                }
                goto EXEC_LOOP;
            case 0x78: // JS
                if ((osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst < 0))) {
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                } else {
                    physmem8_ptr = (physmem8_ptr + 1) >> 0;
                }
                goto EXEC_LOOP;
            case 0x79: // JNS
                if (!(osm == 24 ? ((osm_src >> 7) & 1) : (osm_dst < 0))) {
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                } else {
                    physmem8_ptr = (physmem8_ptr + 1) >> 0;
                }
                goto EXEC_LOOP;
            case 0x7a: // JP
                if (check_parity()) {
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                } else {
                    physmem8_ptr = (physmem8_ptr + 1) >> 0;
                }
                goto EXEC_LOOP;
            case 0x7b: // JNP
                if (!check_parity()) {
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                } else {
                    physmem8_ptr = (physmem8_ptr + 1) >> 0;
                }
                goto EXEC_LOOP;
            case 0x7c: // JL
                if (check_less_than()) {
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                } else {
                    physmem8_ptr = (physmem8_ptr + 1) >> 0;
                }
                goto EXEC_LOOP;
            case 0x7d: // JNL
                if (!check_less_than()) {
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                } else {
                    physmem8_ptr = (physmem8_ptr + 1) >> 0;
                }
                goto EXEC_LOOP;
            case 0x7e: // JLE
                if (check_less_or_equal()) {
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                } else {
                    physmem8_ptr = (physmem8_ptr + 1) >> 0;
                }
                goto EXEC_LOOP;
            case 0x7f: // JNLE
                if (!check_less_or_equal()) {
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                } else {
                    physmem8_ptr = (physmem8_ptr + 1) >> 0;
                }
                goto EXEC_LOOP;
            case 0xe0: // LOOPNE
            case 0xe1: // LOOPE
            case 0xe2: // LOOP
                x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                if (ipr & 0x0080) {
                    conditional_var = 0xffff;
                } else {
                    conditional_var = -1;
                }
                y = (regs[1] - 1) & conditional_var;
                regs[1] = (regs[1] & ~conditional_var) | y;
                OPbyte &= 3;
                if (OPbyte == 0) {
                    z = !(osm_dst == 0);
                } else if (OPbyte == 1) {
                    z = (osm_dst == 0);
                } else {
                    z = 1;
                }
                if (y && z) {
                    if (ipr & 0x0100) {
                        eip = (eip + physmem8_ptr - initial_mem_ptr + x) & 0xffff, physmem8_ptr = initial_mem_ptr = 0;
                    } else {
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
                    }
                }
                goto EXEC_LOOP;
            case 0xe3: // JCXZ
                x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                if (ipr & 0x0080) {
                    conditional_var = 0xffff;
                } else {
                    conditional_var = -1;
                }
                if ((regs[1] & conditional_var) == 0) {
                    if (ipr & 0x0100) {
                        eip = (eip + physmem8_ptr - initial_mem_ptr + x) & 0xffff, physmem8_ptr = initial_mem_ptr = 0;
                    } else {
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
                    }
                }
                goto EXEC_LOOP;
            case 0xc2: // RET
                y = (ld16_mem8_direct() << 16) >> 16;
                x = pop_dword_from_stack_read();
                regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 4 + y) & SS_mask);
                eip = x, physmem8_ptr = initial_mem_ptr = 0;
                goto EXEC_LOOP;
            case 0xc3: // RET
                if (x86_64_long_mode) {
                    mem8_loc = regs[4];
                    x = ld_32bits_mem8_read();
                    regs[4] = (regs[4] + 4) >> 0;
                } else {
                    x = pop_dword_from_stack_read();
                    pop_dword_from_stack_incr_ptr();
                }
                eip = x, physmem8_ptr = initial_mem_ptr = 0;
                goto EXEC_LOOP;
            case 0xe8: // CALL
                x = phys_mem8[physmem8_ptr] |
                    (phys_mem8[physmem8_ptr + 1] << 8) |
                    (phys_mem8[physmem8_ptr + 2] << 16) |
                    (phys_mem8[physmem8_ptr + 3] << 24);
                physmem8_ptr += 4;
                y = (eip + physmem8_ptr - initial_mem_ptr);
                if (x86_64_long_mode) {
                    mem8_loc = (regs[4] - 4) >> 0;
                    st32_mem8_write(y);
                    regs[4] = mem8_loc;
                } else {
                    push_dword_to_stack(y);
                }
                physmem8_ptr = (physmem8_ptr + x) >> 0;
                goto EXEC_LOOP;
            case 0x9a: // CALLF
                z = (((ipr >> 8) & 1) ^ 1);
                if (z) {
                    x = phys_mem8[physmem8_ptr] |
                        (phys_mem8[physmem8_ptr + 1] << 8) |
                        (phys_mem8[physmem8_ptr + 2] << 16) |
                        (phys_mem8[physmem8_ptr + 3] << 24);
                    physmem8_ptr += 4;
                } else {
                    x = ld16_mem8_direct();
                }
                y = ld16_mem8_direct();
                op_CALLF(z, y, x, (eip + physmem8_ptr - initial_mem_ptr));
                if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xca: // RET
                y = (ld16_mem8_direct() << 16) >> 16;
                op_RETF((((ipr >> 8) & 1) ^ 1), y);
                if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xcb: // RET
                op_RETF((((ipr >> 8) & 1) ^ 1), 0);
                if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xcf: // IRET
                op_IRET((((ipr >> 8) & 1) ^ 1));
                if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0x90: // NOP
                goto EXEC_LOOP;
            case 0xcc: // INT
                y = (eip + physmem8_ptr - initial_mem_ptr);
                do_interrupt(3, 1, 0, y, 0);
                goto EXEC_LOOP;
            case 0xcd: // INT
                x = phys_mem8[physmem8_ptr++];
                if ((eflags & 0x00020000) && ((eflags >> 12) & 3) != 3) {
                    abort(13);
                }
                y = (eip + physmem8_ptr - initial_mem_ptr);
                do_interrupt(x, 1, 0, y, 0);
                goto EXEC_LOOP;
            case 0xce: // INTO
                if (check_overflow()) {
                    y = (eip + physmem8_ptr - initial_mem_ptr);
                    do_interrupt(4, 1, 0, y, 0);
                }
                goto EXEC_LOOP;
            case 0x62: // BOUND
                checkOp_BOUND();
                goto EXEC_LOOP;
            case 0xf5: // CMC
                osm_src = get_conditional_flags() ^ 0x0001;
                osm_dst = ((osm_src >> 6) & 1) ^ 1;
                osm = 24;
                goto EXEC_LOOP;
            case 0xf8: // CLC
                osm_src = get_conditional_flags() & ~0x0001;
                osm_dst = ((osm_src >> 6) & 1) ^ 1;
                osm = 24;
                goto EXEC_LOOP;
            case 0xf9: // STC
                osm_src = get_conditional_flags() | 0x0001;
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
                if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0x9e: // SAHF
                osm_src = ((regs[0] >> 8) & (0x0080 | 0x0040 | 0x0010 | 0x0004 | 0x0001)) | (check_overflow() << 11);
                osm_dst = ((osm_src >> 6) & 1) ^ 1;
                osm = 24;
                goto EXEC_LOOP;
            case 0x9f: // LAHF
                x = get_FLAGS();
                set_word_in_register(4, x);
                goto EXEC_LOOP;
            case 0xf4: // HLT
                if (cpl != 0) {
                    abort(13);
                }
                halted = 1;
                goto OUTER_LOOP;
            case 0xa4: // MOVSB
                stringOp_MOVSB();
                goto EXEC_LOOP;
            case 0xa5: // MOVSW/D
                ipr & 0x0100 ? stringOp_MOVSW() : stringOp_MOVSD();
                goto EXEC_LOOP;
            case 0xaa: // STOSB
                stringOp_STOSB();
                goto EXEC_LOOP;
            case 0xab: // STOSW/D
                ipr & 0x0100 ? stringOp_STOSW() : stringOp_STOSD();
                goto EXEC_LOOP;
            case 0xa6: // CMPSB
                stringOp_CMPSB();
                goto EXEC_LOOP;
            case 0xa7: // CMPSW/D
                ipr & 0x0100 ? stringOp_CMPSW() : stringOp_CMPSD();
                goto EXEC_LOOP;
            case 0xac: // LOSB
                stringOp_LODSB();
                goto EXEC_LOOP;
            case 0xad: // LOSW/D
                ipr & 0x0100 ? stringOp_LODSW() : stringOp_LODSD();
                goto EXEC_LOOP;
            case 0xae: // SCASB
                stringOp_SCASB();
                goto EXEC_LOOP;
            case 0xaf: // SCASW/D
                ipr & 0x0100 ? stringOp_SCASW() : stringOp_SCASD();
                goto EXEC_LOOP;
            case 0x6c: // INSB
                stringOp_INSB();
                if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0x6d: // INSW/D
                ipr & 0x0100 ? stringOp_INSW() : stringOp_INSD();
                if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0x6e: // OUTSB
                stringOp_OUTSB();
                if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0x6f: // OUTSW/D
                ipr & 0x0100 ? stringOp_OUTSW() : stringOp_OUTSD();
                if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
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
                mem8 = phys_mem8[physmem8_ptr++];
                reg_idx1 = (mem8 >> 3) & 7;
                reg_idx0 = mem8 & 7;
                conditional_var = ((OPbyte & 7) << 3) | ((mem8 >> 3) & 7);
                set_lower_word_in_register(0, 0xffff);
                if ((mem8 >> 6) == 3) {
                } else {
                    mem8_loc = segment_translation(mem8);
                }
                goto EXEC_LOOP;
            case 0x9b: // FWAIT/WAIT
                goto EXEC_LOOP;
            case 0xe4: // IN AL,
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                x = phys_mem8[physmem8_ptr++];
                set_word_in_register(0, ld8_port(x));
                if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xe5: // IN AX,
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                x = phys_mem8[physmem8_ptr++];
                regs[0] = ld32_port(x);
                if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xe6: // OUT ,AL
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                x = phys_mem8[physmem8_ptr++];
                st8_port(x, regs[0] & 0xff);
                if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xe7: // OUT ,AX
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                x = phys_mem8[physmem8_ptr++];
                st32_port(x, regs[0]);
                if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xec: // IN AL,DX
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                set_word_in_register(0, ld8_port(regs[2] & 0xffff));
                if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xed: // IN AX,DX
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                regs[0] = ld32_port(regs[2] & 0xffff);
                if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xee: // OUT DX,AL
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                st8_port(regs[2] & 0xffff, regs[0] & 0xff);
                if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0xef: // OUT DX,AX
                iopl = (eflags >> 12) & 3;
                if (cpl > iopl) {
                    abort(13);
                }
                st32_port(regs[2] & 0xffff, regs[0]);
                if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                    goto OUTER_LOOP;
                }
                goto EXEC_LOOP;
            case 0x27: // DAA
                op_DAA();
                goto EXEC_LOOP;
            case 0x2f: // DAS
                op_DAS();
                goto EXEC_LOOP;
            case 0x37: // AAA
                op_AAA();
                goto EXEC_LOOP;
            case 0x3f: // AAS
                op_AAS();
                goto EXEC_LOOP;
            case 0xd4: // AAM
                x = phys_mem8[physmem8_ptr++];
                op_AAM(x);
                goto EXEC_LOOP;
            case 0xd5: // AAD
                x = phys_mem8[physmem8_ptr++];
                op_AAD(x);
                goto EXEC_LOOP;
            case 0x63: // ARPL
                op_ARPL();
                goto EXEC_LOOP;
            case 0xd6: // -
            case 0xf1: // -
                abort(6);
            case 0x0f: // 2-byte instruction escape
                OPbyte = phys_mem8[physmem8_ptr++];
                switch (OPbyte) {
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
                    x = phys_mem8[physmem8_ptr] |
                        (phys_mem8[physmem8_ptr + 1] << 8) |
                        (phys_mem8[physmem8_ptr + 2] << 16) |
                        (phys_mem8[physmem8_ptr + 3] << 24);
                    physmem8_ptr += 4;
                    if (check_status_bits_for_jump(OPbyte & 0xf)) {
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
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
                    mem8 = phys_mem8[physmem8_ptr++];
                    x = check_status_bits_for_jump(OPbyte & 0xf);
                    if ((mem8 >> 6) == 3) {
                        set_word_in_register(mem8 & 7, x);
                    } else {
                        mem8_loc = segment_translation(mem8);
                        st8_mem8_write(x);
                    }
                    goto EXEC_LOOP;
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
                    mem8 = phys_mem8[physmem8_ptr++];
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_read();
                    }
                    if (check_status_bits_for_jump(OPbyte & 0xf)) {
                        regs[(mem8 >> 3) & 7] = x;
                    }
                    goto EXEC_LOOP;
                case 0xb6: // MOVZX
                    mem8 = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        x = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1)) & 0xff;
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = (((last_tlb_val = tlb_read[mem8_loc >> 12]) == -1)
                                 ? __ld_8bits_mem8_read()
                                 : phys_mem8[mem8_loc ^ last_tlb_val]);
                    }
                    regs[reg_idx1] = x;
                    goto EXEC_LOOP;
                case 0xb7: // MOVZX
                    mem8 = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7] & 0xffff;
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_16bits_mem8_read();
                    }
                    regs[reg_idx1] = x;
                    goto EXEC_LOOP;
                case 0xbe: // MOVSX
                    mem8 = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        x = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = (((last_tlb_val = tlb_read[mem8_loc >> 12]) == -1)
                                 ? __ld_8bits_mem8_read()
                                 : phys_mem8[mem8_loc ^ last_tlb_val]);
                    }
                    regs[reg_idx1] = (((x) << 24) >> 24);
                    goto EXEC_LOOP;
                case 0xbf: // MOVSX
                    mem8 = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_16bits_mem8_read();
                    }
                    regs[reg_idx1] = (((x) << 16) >> 16);
                    goto EXEC_LOOP;
                case 0x00: // G6 (SLDT, STR, LLDT, LTR, VERR, VERW, -)
                    if (!(cr0 & (1 << 0)) || (eflags & 0x00020000)) {
                        abort(6);
                    }
                    mem8 = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    switch (conditional_var) {
                    case 0: // SLDT Store Local Descriptor Table Register
                    case 1: // STR Store Task Register
                        if (conditional_var == 0) {
                            x = ldt.selector;
                        } else {
                            x = tr.selector;
                        }
                        if ((mem8 >> 6) == 3) {
                            set_lower_word_in_register(mem8 & 7, x);
                        } else {
                            mem8_loc = segment_translation(mem8);
                            st16_mem8_write(x);
                        }
                        break;
                    case 2: // LDTR Load Local Descriptor Table Register
                    case 3: // LTR Load Task Register
                        if (cpl != 0) {
                            abort(13);
                        }
                        if ((mem8 >> 6) == 3) {
                            x = regs[mem8 & 7] & 0xffff;
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_16bits_mem8_read();
                        }
                        if (conditional_var == 2) {
                            op_LDTR(x);
                        } else {
                            op_LTR(x);
                        }
                        break;
                    case 4: // VERR Verify a Segment for Reading
                    case 5: // VERW Verify a Segment for Writing
                        if ((mem8 >> 6) == 3) {
                            x = regs[mem8 & 7] & 0xffff;
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_16bits_mem8_read();
                        }
                        op_VERR_VERW(x, conditional_var & 1);
                        break;
                    default:
                        abort(6);
                    }
                    goto EXEC_LOOP;
                case 0x01: // G7 (SGDT, SIDT, LGDT, LIDT, SMSW, -, LMSW, -)
                    mem8 = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    switch (conditional_var) {
                    case 2:
                    case 3:
                        if ((mem8 >> 6) == 3) {
                            abort(6);
                        }
                        if (cpl != 0) {
                            abort(13);
                        }
                        mem8_loc = segment_translation(mem8);
                        x = ld_16bits_mem8_read();
                        mem8_loc += 2;
                        y = ld_32bits_mem8_read();
                        if (conditional_var == 2) {
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
                        if ((mem8 >> 6) == 3) {
                            abort(6);
                        }
                        mem8_loc = segment_translation(mem8);
                        tlb_flush_page(mem8_loc & -4096);
                        break;
                    default:
                        abort(6);
                    }
                    goto EXEC_LOOP;
                case 0x02: // LAR
                case 0x03: // LSL
                    op_LAR_LSL((((ipr >> 8) & 1) ^ 1), OPbyte & 1);
                    goto EXEC_LOOP;
                case 0x20: // MOV
                    if (cpl != 0) {
                        abort(13);
                    }
                    mem8 = phys_mem8[physmem8_ptr++];
                    if ((mem8 >> 6) != 3) {
                        abort(6);
                    }
                    reg_idx1 = (mem8 >> 3) & 7;
                    switch (reg_idx1) {
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
                    regs[mem8 & 7] = x;
                    goto EXEC_LOOP;
                case 0x22: // MOV
                    if (cpl != 0) {
                        abort(13);
                    }
                    mem8 = phys_mem8[physmem8_ptr++];
                    if ((mem8 >> 6) != 3) {
                        abort(6);
                    }
                    reg_idx1 = (mem8 >> 3) & 7;
                    x = regs[mem8 & 7];
                    switch (reg_idx1) {
                    case 0:
                        set_CR0(x);
                        break;
                    case 2:
                        cr2 = x;
                        break;
                    case 3:
                        set_CR3(x);
                        break;
                    case 4:
                        set_CR4(x);
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
                    mem8 = phys_mem8[physmem8_ptr++];
                    if ((mem8 >> 6) != 3) {
                        abort(6);
                    }
                    reg_idx1 = (mem8 >> 3) & 7;
                    x = regs[mem8 & 7];
                    if (reg_idx1 == 4 || reg_idx1 == 5) {
                        abort(6);
                    }
                    goto EXEC_LOOP;
                case 0xb2: // LSS
                case 0xb4: // LFS
                case 0xb5: // LGS
                    op_16_load_far_pointer32(OPbyte & 7);
                    goto EXEC_LOOP;
                case 0xa2: // -
                    op_CPUID();
                    goto EXEC_LOOP;
                case 0xa4: // SHLD
                    mem8 = phys_mem8[physmem8_ptr++];
                    y = regs[(mem8 >> 3) & 7];
                    if ((mem8 >> 6) == 3) {
                        z = phys_mem8[physmem8_ptr++];
                        reg_idx0 = mem8 & 7;
                        regs[reg_idx0] = op_SHLD(regs[reg_idx0], y, z);
                    } else {
                        mem8_loc = segment_translation(mem8);
                        z = phys_mem8[physmem8_ptr++];
                        x = ld_32bits_mem8_write();
                        x = op_SHLD(x, y, z);
                        st32_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0xa5: // SHLD
                    mem8 = phys_mem8[physmem8_ptr++];
                    y = regs[(mem8 >> 3) & 7];
                    z = regs[1];
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        regs[reg_idx0] = op_SHLD(regs[reg_idx0], y, z);
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_write();
                        x = op_SHLD(x, y, z);
                        st32_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0xac: // SHRD
                    mem8 = phys_mem8[physmem8_ptr++];
                    y = regs[(mem8 >> 3) & 7];
                    if ((mem8 >> 6) == 3) {
                        z = phys_mem8[physmem8_ptr++];
                        reg_idx0 = mem8 & 7;
                        regs[reg_idx0] = op_SHRD(regs[reg_idx0], y, z);
                    } else {
                        mem8_loc = segment_translation(mem8);
                        z = phys_mem8[physmem8_ptr++];
                        x = ld_32bits_mem8_write();
                        x = op_SHRD(x, y, z);
                        st32_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0xad: // SHRD
                    mem8 = phys_mem8[physmem8_ptr++];
                    y = regs[(mem8 >> 3) & 7];
                    z = regs[1];
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        regs[reg_idx0] = op_SHRD(regs[reg_idx0], y, z);
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_write();
                        x = op_SHRD(x, y, z);
                        st32_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0xba: // G8 (-, -, -, -, BT, BTS, BTR, BTC)
                    mem8 = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    switch (conditional_var) {
                    case 4: // BT
                        if ((mem8 >> 6) == 3) {
                            x = regs[mem8 & 7];
                            y = phys_mem8[physmem8_ptr++];
                        } else {
                            mem8_loc = segment_translation(mem8);
                            y = phys_mem8[physmem8_ptr++];
                            x = ld_32bits_mem8_read();
                        }
                        op_BT(x, y);
                        break;
                    case 5: // BTS
                    case 6: // BTR
                    case 7: // BTC
                        if ((mem8 >> 6) == 3) {
                            reg_idx0 = mem8 & 7;
                            y = phys_mem8[physmem8_ptr++];
                            regs[reg_idx0] = op_BTS_BTR_BTC(conditional_var & 3, regs[reg_idx0], y);
                        } else {
                            mem8_loc = segment_translation(mem8);
                            y = phys_mem8[physmem8_ptr++];
                            x = ld_32bits_mem8_write();
                            x = op_BTS_BTR_BTC(conditional_var & 3, x, y);
                            st32_mem8_write(x);
                        }
                        break;
                    default:
                        abort(6);
                    }
                    goto EXEC_LOOP;
                case 0xa3: // BT
                    mem8 = phys_mem8[physmem8_ptr++];
                    y = regs[(mem8 >> 3) & 7];
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        mem8_loc = (mem8_loc + ((y >> 5) << 2)) >> 0;
                        x = ld_32bits_mem8_read();
                    }
                    op_BT(x, y);
                    goto EXEC_LOOP;
                case 0xab: // BTS
                case 0xb3: // BTR
                case 0xbb: // BTC
                    mem8 = phys_mem8[physmem8_ptr++];
                    y = regs[(mem8 >> 3) & 7];
                    conditional_var = (OPbyte >> 3) & 3;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        regs[reg_idx0] = op_BTS_BTR_BTC(conditional_var, regs[reg_idx0], y);
                    } else {
                        mem8_loc = segment_translation(mem8);
                        mem8_loc = (mem8_loc + ((y >> 5) << 2)) >> 0;
                        x = ld_32bits_mem8_write();
                        x = op_BTS_BTR_BTC(conditional_var, x, y);
                        st32_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0xbc: // BSF
                case 0xbd: // BSR
                    mem8 = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        y = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y = ld_32bits_mem8_read();
                    }
                    if (OPbyte & 1) {
                        regs[reg_idx1] = op_BSR(regs[reg_idx1], y);
                    } else {
                        regs[reg_idx1] = op_BSF(regs[reg_idx1], y);
                    }
                    goto EXEC_LOOP;
                case 0xaf: // IMUL
                    mem8 = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        y = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y = ld_32bits_mem8_read();
                    }
                    regs[reg_idx1] = op_IMUL32(regs[reg_idx1], y);
                    goto EXEC_LOOP;
                case 0x31: // -
                    if ((cr4 & (1 << 2)) && cpl != 0) {
                        abort(13);
                    }
                    x = cycles_processed + (cycles_requested - cycles_remaining);
                    regs[0] = x >> 0;
                    regs[2] = (x / 0x100000000) >> 0;
                    goto EXEC_LOOP;
                case 0xc0: // -
                    mem8 = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        x = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                        y = do_8bit_math(0, x, (regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1)));
                        set_word_in_register(reg_idx1, x);
                        set_word_in_register(reg_idx0, y);
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_8bits_mem8_write();
                        y = do_8bit_math(0, x, (regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1)));
                        st8_mem8_write(y);
                        set_word_in_register(reg_idx1, x);
                    }
                    goto EXEC_LOOP;
                case 0xc1: // -
                    mem8 = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        x = regs[reg_idx0];
                        y = do_32bit_math(0, x, regs[reg_idx1]);
                        regs[reg_idx1] = x;
                        regs[reg_idx0] = y;
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_write();
                        y = do_32bit_math(0, x, regs[reg_idx1]);
                        st32_mem8_write(y);
                        regs[reg_idx1] = x;
                    }
                    goto EXEC_LOOP;
                case 0xb0: // -
                    mem8 = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        x = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                        y = do_8bit_math(5, regs[0], x);
                        if (y == 0) {
                            set_word_in_register(reg_idx0, (regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1)));
                        } else {
                            set_word_in_register(0, x);
                        }
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_8bits_mem8_write();
                        y = do_8bit_math(5, regs[0], x);
                        if (y == 0) {
                            st8_mem8_write((regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1)));
                        } else {
                            set_word_in_register(0, x);
                        }
                    }
                    goto EXEC_LOOP;
                case 0xb1: // -
                    mem8 = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        x = regs[reg_idx0];
                        y = do_32bit_math(5, regs[0], x);
                        if (y == 0) {
                            regs[reg_idx0] = regs[reg_idx1];
                        } else {
                            regs[0] = x;
                        }
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_32bits_mem8_write();
                        y = do_32bit_math(5, regs[0], x);
                        if (y == 0) {
                            st32_mem8_write(regs[reg_idx1]);
                        } else {
                            regs[0] = x;
                        }
                    }
                    goto EXEC_LOOP;
                case 0xa0: // PUSH FS
                case 0xa8: // PUSH GS
                    push_dword_to_stack(segs[(OPbyte >> 3) & 7].selector);
                    goto EXEC_LOOP;
                case 0xa1: // POP FS
                case 0xa9: // POP GS
                    x = pop_dword_from_stack_read() & 0xffff;
                    pop_dword_from_stack_incr_ptr();
                    set_segment_register((OPbyte >> 3) & 7, x);
                    goto EXEC_LOOP;
                case 0xc8: // -
                case 0xc9: // -
                case 0xca: // -
                case 0xcb: // -
                case 0xcc: // -
                case 0xcd: // -
                case 0xce: // -
                case 0xcf: // BSWAP (80486)
                    reg_idx1 = OPbyte & 7;
                    x = regs[reg_idx1];
                    x = ((x >> 24) & 0xff) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24);
                    regs[reg_idx1] = x;
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
                break;
            default:
                switch (OPbyte) {
                case 0x189: // MOV
                    mem8 = phys_mem8[physmem8_ptr++];
                    x = regs[(mem8 >> 3) & 7];
                    if ((mem8 >> 6) == 3) {
                        set_lower_word_in_register(mem8 & 7, x);
                    } else {
                        mem8_loc = segment_translation(mem8);
                        st16_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0x18b: // MOV
                    mem8 = phys_mem8[physmem8_ptr++];
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_16bits_mem8_read();
                    }
                    set_lower_word_in_register((mem8 >> 3) & 7, x);
                    goto EXEC_LOOP;
                case 0x1b8: // MOV A
                case 0x1b9: // MOV C
                case 0x1ba: // MOV D
                case 0x1bb: // MOV B
                case 0x1bc: // MOV SP
                case 0x1bd: // MOV BP
                case 0x1be: // MOV SI
                case 0x1bf: // MOV DI
                    set_lower_word_in_register(OPbyte & 7, ld16_mem8_direct());
                    goto EXEC_LOOP;
                case 0x1a1: // MOV AX,
                    mem8_loc = segmented_mem8_loc_for_MOV(false);
                    x = ld_16bits_mem8_read();
                    set_lower_word_in_register(0, x);
                    goto EXEC_LOOP;
                case 0x1a3: // MOV ,AX
                    mem8_loc = segmented_mem8_loc_for_MOV(true);
                    st16_mem8_write(regs[0]);
                    goto EXEC_LOOP;
                case 0x1c7: // MOV
                    mem8 = phys_mem8[physmem8_ptr++];
                    if ((mem8 >> 6) == 3) {
                        x = ld16_mem8_direct();
                        set_lower_word_in_register(mem8 & 7, x);
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld16_mem8_direct();
                        st16_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0x191: // XCHG C
                case 0x192: // XCHG D
                case 0x193: // XCHG B
                case 0x194: // XCHG SP
                case 0x195: // XCHG BP
                case 0x196: // XCHG SI
                case 0x197: // XCHG DI
                    reg_idx1 = OPbyte & 7;
                    x = regs[0];
                    set_lower_word_in_register(0, regs[reg_idx1]);
                    set_lower_word_in_register(reg_idx1, x);
                    goto EXEC_LOOP;
                case 0x187: // XCHG
                    mem8 = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        x = regs[reg_idx0];
                        set_lower_word_in_register(reg_idx0, regs[reg_idx1]);
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_16bits_mem8_write();
                        st16_mem8_write(regs[reg_idx1]);
                    }
                    set_lower_word_in_register(reg_idx1, x);
                    goto EXEC_LOOP;
                case 0x1c4: // LES
                    op_16_load_far_pointer16(0);
                    goto EXEC_LOOP;
                case 0x1c5: // LDS
                    op_16_load_far_pointer16(3);
                    goto EXEC_LOOP;
                case 0x101: // ADD
                case 0x109: // OR
                case 0x111: // ADC
                case 0x119: // SBB
                case 0x121: // AND
                case 0x129: // SUB
                case 0x131: // XOR
                case 0x139: // CMP
                    mem8 = phys_mem8[physmem8_ptr++];
                    conditional_var = (OPbyte >> 3) & 7;
                    y = regs[(mem8 >> 3) & 7];
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        set_lower_word_in_register(reg_idx0, do_16bit_math(conditional_var, regs[reg_idx0], y));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        if (conditional_var != 7) {
                            x = ld_16bits_mem8_write();
                            x = do_16bit_math(conditional_var, x, y);
                            st16_mem8_write(x);
                        } else {
                            x = ld_16bits_mem8_read();
                            do_16bit_math(7, x, y);
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
                    mem8 = phys_mem8[physmem8_ptr++];
                    conditional_var = (OPbyte >> 3) & 7;
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        y = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y = ld_16bits_mem8_read();
                    }
                    set_lower_word_in_register(reg_idx1, do_16bit_math(conditional_var, regs[reg_idx1], y));
                    goto EXEC_LOOP;
                case 0x105: // ADD
                case 0x10d: // OR
                case 0x115: // ADC
                case 0x11d: // SBB
                case 0x125: // AND
                case 0x12d: // SUB
                case 0x135: // XOR
                case 0x13d: // CMP
                    y = ld16_mem8_direct();
                    conditional_var = (OPbyte >> 3) & 7;
                    set_lower_word_in_register(0, do_16bit_math(conditional_var, regs[0], y));
                    goto EXEC_LOOP;
                case 0x181: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                    mem8 = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        y = ld16_mem8_direct();
                        set_lower_word_in_register(reg_idx0, do_16bit_math(conditional_var, regs[reg_idx0], y));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y = ld16_mem8_direct();
                        if (conditional_var != 7) {
                            x = ld_16bits_mem8_write();
                            x = do_16bit_math(conditional_var, x, y);
                            st16_mem8_write(x);
                        } else {
                            x = ld_16bits_mem8_read();
                            do_16bit_math(7, x, y);
                        }
                    }
                    goto EXEC_LOOP;
                case 0x183: // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
                    mem8 = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        y = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        set_lower_word_in_register(reg_idx0, do_16bit_math(conditional_var, regs[reg_idx0], y));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        if (conditional_var != 7) {
                            x = ld_16bits_mem8_write();
                            x = do_16bit_math(conditional_var, x, y);
                            st16_mem8_write(x);
                        } else {
                            x = ld_16bits_mem8_read();
                            do_16bit_math(7, x, y);
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
                    reg_idx1 = OPbyte & 7;
                    set_lower_word_in_register(reg_idx1, increment_16bit(regs[reg_idx1]));
                    goto EXEC_LOOP;
                case 0x148: // DEC A
                case 0x149: // DEC C
                case 0x14a: // DEC D
                case 0x14b: // DEC B
                case 0x14c: // DEC SP
                case 0x14d: // DEC BP
                case 0x14e: // DEC SI
                case 0x14f: // DEC DI
                    reg_idx1 = OPbyte & 7;
                    set_lower_word_in_register(reg_idx1, decrement_16bit(regs[reg_idx1]));
                    goto EXEC_LOOP;
                case 0x16b: // IMUL
                    mem8 = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        y = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y = ld_16bits_mem8_read();
                    }
                    z = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    set_lower_word_in_register(reg_idx1, op_16_IMUL(y, z));
                    goto EXEC_LOOP;
                case 0x169: // IMUL
                    mem8 = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        y = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y = ld_16bits_mem8_read();
                    }
                    z = ld16_mem8_direct();
                    set_lower_word_in_register(reg_idx1, op_16_IMUL(y, z));
                    goto EXEC_LOOP;
                case 0x185: // TEST
                    mem8 = phys_mem8[physmem8_ptr++];
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_16bits_mem8_read();
                    }
                    y = regs[(mem8 >> 3) & 7];
                    osm_dst = (((x & y) << 16) >> 16);
                    osm = 13;
                    goto EXEC_LOOP;
                case 0x1a9: // TEST
                    y = ld16_mem8_direct();
                    osm_dst = (((regs[0] & y) << 16) >> 16);
                    osm = 13;
                    goto EXEC_LOOP;
                case 0x1f7: // G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
                    mem8 = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    switch (conditional_var) {
                    case 0:
                        if ((mem8 >> 6) == 3) {
                            x = regs[mem8 & 7];
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_16bits_mem8_read();
                        }
                        y = ld16_mem8_direct();
                        osm_dst = (((x & y) << 16) >> 16);
                        osm = 13;
                        break;
                    case 2:
                        if ((mem8 >> 6) == 3) {
                            reg_idx0 = mem8 & 7;
                            set_lower_word_in_register(reg_idx0, ~regs[reg_idx0]);
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_16bits_mem8_write();
                            x = ~x;
                            st16_mem8_write(x);
                        }
                        break;
                    case 3:
                        if ((mem8 >> 6) == 3) {
                            reg_idx0 = mem8 & 7;
                            set_lower_word_in_register(reg_idx0, do_16bit_math(5, 0, regs[reg_idx0]));
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_16bits_mem8_write();
                            x = do_16bit_math(5, 0, x);
                            st16_mem8_write(x);
                        }
                        break;
                    case 4:
                        if ((mem8 >> 6) == 3) {
                            x = regs[mem8 & 7];
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_16bits_mem8_read();
                        }
                        x = op_16_MUL(regs[0], x);
                        set_lower_word_in_register(0, x);
                        set_lower_word_in_register(2, x >> 16);
                        break;
                    case 5:
                        if ((mem8 >> 6) == 3) {
                            x = regs[mem8 & 7];
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_16bits_mem8_read();
                        }
                        x = op_16_IMUL(regs[0], x);
                        set_lower_word_in_register(0, x);
                        set_lower_word_in_register(2, x >> 16);
                        break;
                    case 6:
                        if ((mem8 >> 6) == 3) {
                            x = regs[mem8 & 7];
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_16bits_mem8_read();
                        }
                        op_16_DIV(x);
                        break;
                    case 7:
                        if ((mem8 >> 6) == 3) {
                            x = regs[mem8 & 7];
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_16bits_mem8_read();
                        }
                        op_16_IDIV(x);
                        break;
                    default:
                        abort(6);
                    }
                    goto EXEC_LOOP;
                case 0x1c1: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
                    mem8 = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        y = phys_mem8[physmem8_ptr++];
                        reg_idx0 = mem8 & 7;
                        set_lower_word_in_register(reg_idx0, shift16(conditional_var, regs[reg_idx0], y));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y = phys_mem8[physmem8_ptr++];
                        x = ld_16bits_mem8_write();
                        x = shift16(conditional_var, x, y);
                        st16_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0x1d1: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
                    mem8 = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        set_lower_word_in_register(reg_idx0, shift16(conditional_var, regs[reg_idx0], 1));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_16bits_mem8_write();
                        x = shift16(conditional_var, x, 1);
                        st16_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0x1d3: // G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
                    mem8 = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    y = regs[1] & 0xff;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        set_lower_word_in_register(reg_idx0, shift16(conditional_var, regs[reg_idx0], y));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x = ld_16bits_mem8_write();
                        x = shift16(conditional_var, x, y);
                        st16_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0x198: // CBW
                    set_lower_word_in_register(0, (regs[0] << 24) >> 24);
                    goto EXEC_LOOP;
                case 0x199: // CWD
                    set_lower_word_in_register(2, (regs[0] << 16) >> 31);
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
                    push_word_to_stack(regs[OPbyte & 7]);
                    goto EXEC_LOOP;
                case 0x158: // POP A
                case 0x159: // POP C
                case 0x15a: // POP D
                case 0x15b: // POP B
                case 0x15c: // POP SP
                case 0x15d: // POP BP
                case 0x15e: // POP SI
                case 0x15f: // POP DI
                    x = pop_word_from_stack_read();
                    pop_word_from_stack_incr_ptr();
                    set_lower_word_in_register(OPbyte & 7, x);
                    goto EXEC_LOOP;
                case 0x160: // PUSHA
                    op_16_PUSHA();
                    goto EXEC_LOOP;
                case 0x161: // POPA
                    op_16_POPA();
                    goto EXEC_LOOP;
                case 0x18f: // POP
                    mem8 = phys_mem8[physmem8_ptr++];
                    if ((mem8 >> 6) == 3) {
                        x = pop_word_from_stack_read();
                        pop_word_from_stack_incr_ptr();
                        set_lower_word_in_register(mem8 & 7, x);
                    } else {
                        x = pop_word_from_stack_read();
                        y = regs[4];
                        pop_word_from_stack_incr_ptr();
                        z = regs[4];
                        mem8_loc = segment_translation(mem8);
                        regs[4] = y;
                        st16_mem8_write(x);
                        regs[4] = z;
                    }
                    goto EXEC_LOOP;
                case 0x168: // PUSH
                    x = ld16_mem8_direct();
                    push_word_to_stack(x);
                    goto EXEC_LOOP;
                case 0x16a: // PUSH
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    push_word_to_stack(x);
                    goto EXEC_LOOP;
                case 0x1c8: // ENTER
                    op_16_ENTER();
                    goto EXEC_LOOP;
                case 0x1c9: // LEAVE
                    op_16_LEAVE();
                    goto EXEC_LOOP;
                case 0x106: // PUSH
                case 0x10e: // PUSH
                case 0x116: // PUSH
                case 0x11e: // PUSH
                    push_word_to_stack(segs[(OPbyte >> 3) & 3].selector);
                    goto EXEC_LOOP;
                case 0x107: // POP
                case 0x117: // POP
                case 0x11f: // POP
                    x = pop_word_from_stack_read();
                    pop_word_from_stack_incr_ptr();
                    set_segment_register((OPbyte >> 3) & 3, x);
                    goto EXEC_LOOP;
                case 0x18d: // LEA
                    mem8 = phys_mem8[physmem8_ptr++];
                    if ((mem8 >> 6) == 3) {
                        abort(6);
                    }
                    ipr = (ipr & ~0x000f) | (6 + 1);
                    set_lower_word_in_register((mem8 >> 3) & 7, segment_translation(mem8));
                    goto EXEC_LOOP;
                case 0x1ff: // G5 (INC, DEC, CALL, CALL, JMP, JMP, PUSH, -)
                    mem8 = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    switch (conditional_var) {
                    case 0:
                        if ((mem8 >> 6) == 3) {
                            reg_idx0 = mem8 & 7;
                            set_lower_word_in_register(reg_idx0, increment_16bit(regs[reg_idx0]));
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_16bits_mem8_write();
                            x = increment_16bit(x);
                            st16_mem8_write(x);
                        }
                        break;
                    case 1:
                        if ((mem8 >> 6) == 3) {
                            reg_idx0 = mem8 & 7;
                            set_lower_word_in_register(reg_idx0, decrement_16bit(regs[reg_idx0]));
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_16bits_mem8_write();
                            x = decrement_16bit(x);
                            st16_mem8_write(x);
                        }
                        break;
                    case 2:
                        if ((mem8 >> 6) == 3) {
                            x = regs[mem8 & 7] & 0xffff;
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_16bits_mem8_read();
                        }
                        push_word_to_stack((eip + physmem8_ptr - initial_mem_ptr));
                        eip = x, physmem8_ptr = initial_mem_ptr = 0;
                        break;
                    case 4:
                        if ((mem8 >> 6) == 3) {
                            x = regs[mem8 & 7] & 0xffff;
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_16bits_mem8_read();
                        }
                        eip = x, physmem8_ptr = initial_mem_ptr = 0;
                        break;
                    case 6:
                        if ((mem8 >> 6) == 3) {
                            x = regs[mem8 & 7];
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_16bits_mem8_read();
                        }
                        push_word_to_stack(x);
                        break;
                    case 3:
                    case 5:
                        if ((mem8 >> 6) == 3) {
                            abort(6);
                        }
                        mem8_loc = segment_translation(mem8);
                        x = ld_16bits_mem8_read();
                        mem8_loc = (mem8_loc + 2) >> 0;
                        y = ld_16bits_mem8_read();
                        if (conditional_var == 3) {
                            op_CALLF(0, y, x, (eip + physmem8_ptr - initial_mem_ptr));
                        } else {
                            op_JMPF(y, x);
                        }
                        break;
                    default:
                        abort(6);
                    }
                    goto EXEC_LOOP;
                case 0x1eb: // JMP
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    eip = (eip + physmem8_ptr - initial_mem_ptr + x) & 0xffff, physmem8_ptr = initial_mem_ptr = 0;
                    goto EXEC_LOOP;
                case 0x1e9: // JMP
                    x = ld16_mem8_direct();
                    eip = (eip + physmem8_ptr - initial_mem_ptr + x) & 0xffff, physmem8_ptr = initial_mem_ptr = 0;
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
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    y = check_status_bits_for_jump(OPbyte & 0xf);
                    if (y) {
                        eip = (eip + physmem8_ptr - initial_mem_ptr + x) & 0xffff, physmem8_ptr = initial_mem_ptr = 0;
                    }
                    goto EXEC_LOOP;
                case 0x1c2: // RET
                    y = (ld16_mem8_direct() << 16) >> 16;
                    x = pop_word_from_stack_read();
                    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 2 + y) & SS_mask);
                    eip = x, physmem8_ptr = initial_mem_ptr = 0;
                    goto EXEC_LOOP;
                case 0x1c3: // RET
                    x = pop_word_from_stack_read();
                    pop_word_from_stack_incr_ptr();
                    eip = x, physmem8_ptr = initial_mem_ptr = 0;
                    goto EXEC_LOOP;
                case 0x1e8: // CALL
                    x = ld16_mem8_direct();
                    push_word_to_stack((eip + physmem8_ptr - initial_mem_ptr));
                    eip = (eip + physmem8_ptr - initial_mem_ptr + x) & 0xffff, physmem8_ptr = initial_mem_ptr = 0;
                    goto EXEC_LOOP;
                case 0x162: // BOUND
                    op_16_BOUND();
                    goto EXEC_LOOP;
                case 0x1a5: // MOVSW/D
                    op_16_MOVS();
                    goto EXEC_LOOP;
                case 0x1a7: // CMPSW/D
                    op_16_CMPS();
                    goto EXEC_LOOP;
                case 0x1ad: // LOSW/D
                    op_16_LODS();
                    goto EXEC_LOOP;
                case 0x1af: // SCASW/D
                    op_16_SCAS();
                    goto EXEC_LOOP;
                case 0x1ab: // STOSW/D
                    op_16_STOS();
                    goto EXEC_LOOP;
                case 0x16d: // INSW/D
                    op_16_INS();
                    if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                        goto OUTER_LOOP;
                    }
                    goto EXEC_LOOP;
                case 0x16f: // OUTSW/D
                    op_16_OUTS();
                    if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                        goto OUTER_LOOP;
                    }
                    goto EXEC_LOOP;
                case 0x1e5: // IN AX,
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl) {
                        abort(13);
                    }
                    x = phys_mem8[physmem8_ptr++];
                    set_lower_word_in_register(0, ld16_port(x));
                    if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                        goto OUTER_LOOP;
                    }
                    goto EXEC_LOOP;
                case 0x1e7: // OUT ,AX
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl) {
                        abort(13);
                    }
                    x = phys_mem8[physmem8_ptr++];
                    st16_port(x, regs[0] & 0xffff);
                    if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                        goto OUTER_LOOP;
                    }
                    goto EXEC_LOOP;
                case 0x1ed: // IN AX,DX
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl) {
                        abort(13);
                    }
                    set_lower_word_in_register(0, ld16_port(regs[2] & 0xffff));
                    if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
                        goto OUTER_LOOP;
                    }
                    goto EXEC_LOOP;
                case 0x1ef: // OUT DX,AX
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl) {
                        abort(13);
                    }
                    st16_port(regs[2] & 0xffff, regs[0] & 0xffff);
                    if (get_hard_irq() != 0 && (eflags & 0x00000200)) {
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
                    OPbyte &= 0xff;
                    break;
                case 0x163: // ARPL
                case 0x1d6: // -
                case 0x1f1: // -
                case 0x10f: // 2-byte instruction escape
                    OPbyte = phys_mem8[physmem8_ptr++];
                    OPbyte |= 0x0100;
                    switch (OPbyte) {
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
                        x = ld16_mem8_direct();
                        if (check_status_bits_for_jump(OPbyte & 0xf)) {
                            eip = (eip + physmem8_ptr - initial_mem_ptr + x) & 0xffff, physmem8_ptr = initial_mem_ptr = 0;
                        }
                        goto EXEC_LOOP;
                    case 0x140: // -
                    case 0x141: // -
                    case 0x142: // -
                    case 0x143: // -
                    case 0x144: // -
                    case 0x145: // -
                    case 0x146: // -
                    case 0x147: // -
                    case 0x148: // -
                    case 0x149: // -
                    case 0x14a: // -
                    case 0x14b: // -
                    case 0x14c: // -
                    case 0x14d: // -
                    case 0x14e: // -
                    case 0x14f: // -
                        mem8 = phys_mem8[physmem8_ptr++];
                        if ((mem8 >> 6) == 3) {
                            x = regs[mem8 & 7];
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_16bits_mem8_read();
                        }
                        if (check_status_bits_for_jump(OPbyte & 0xf)) {
                            set_lower_word_in_register((mem8 >> 3) & 7, x);
                        }
                        goto EXEC_LOOP;
                    case 0x1b6: // MOVZX
                        mem8 = phys_mem8[physmem8_ptr++];
                        reg_idx1 = (mem8 >> 3) & 7;
                        if ((mem8 >> 6) == 3) {
                            reg_idx0 = mem8 & 7;
                            x = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1)) & 0xff;
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_8bits_mem8_read();
                        }
                        set_lower_word_in_register(reg_idx1, x);
                        goto EXEC_LOOP;
                    case 0x1be: // MOVSX
                        mem8 = phys_mem8[physmem8_ptr++];
                        reg_idx1 = (mem8 >> 3) & 7;
                        if ((mem8 >> 6) == 3) {
                            reg_idx0 = mem8 & 7;
                            x = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_8bits_mem8_read();
                        }
                        set_lower_word_in_register(reg_idx1, (((x) << 24) >> 24));
                        goto EXEC_LOOP;
                    case 0x1af: // IMUL
                        mem8 = phys_mem8[physmem8_ptr++];
                        reg_idx1 = (mem8 >> 3) & 7;
                        if ((mem8 >> 6) == 3) {
                            y = regs[mem8 & 7];
                        } else {
                            mem8_loc = segment_translation(mem8);
                            y = ld_16bits_mem8_read();
                        }
                        set_lower_word_in_register(reg_idx1, op_16_IMUL(regs[reg_idx1], y));
                        goto EXEC_LOOP;
                    case 0x1c1: // -
                        mem8 = phys_mem8[physmem8_ptr++];
                        reg_idx1 = (mem8 >> 3) & 7;
                        if ((mem8 >> 6) == 3) {
                            reg_idx0 = mem8 & 7;
                            x = regs[reg_idx0];
                            y = do_16bit_math(0, x, regs[reg_idx1]);
                            set_lower_word_in_register(reg_idx1, x);
                            set_lower_word_in_register(reg_idx0, y);
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_16bits_mem8_write();
                            y = do_16bit_math(0, x, regs[reg_idx1]);
                            st16_mem8_write(y);
                            set_lower_word_in_register(reg_idx1, x);
                        }
                        goto EXEC_LOOP;
                    case 0x1a0: // PUSH FS
                    case 0x1a8: // PUSH GS
                        push_word_to_stack(segs[(OPbyte >> 3) & 7].selector);
                        goto EXEC_LOOP;
                    case 0x1a1: // POP FS
                    case 0x1a9: // POP GS
                        x = pop_word_from_stack_read();
                        pop_word_from_stack_incr_ptr();
                        set_segment_register((OPbyte >> 3) & 7, x);
                        goto EXEC_LOOP;
                    case 0x1b2: // LSS
                    case 0x1b4: // LFS
                    case 0x1b5: // LGS
                        op_16_load_far_pointer16(OPbyte & 7);
                        goto EXEC_LOOP;
                    case 0x1a4: // SHLD
                    case 0x1ac: // SHRD
                        mem8 = phys_mem8[physmem8_ptr++];
                        y = regs[(mem8 >> 3) & 7];
                        conditional_var = (OPbyte >> 3) & 1;
                        if ((mem8 >> 6) == 3) {
                            z = phys_mem8[physmem8_ptr++];
                            reg_idx0 = mem8 & 7;
                            set_lower_word_in_register(reg_idx0, op_16_SHRD_SHLD(conditional_var, regs[reg_idx0], y, z));
                        } else {
                            mem8_loc = segment_translation(mem8);
                            z = phys_mem8[physmem8_ptr++];
                            x = ld_16bits_mem8_write();
                            x = op_16_SHRD_SHLD(conditional_var, x, y, z);
                            st16_mem8_write(x);
                        }
                        goto EXEC_LOOP;
                    case 0x1a5: // SHLD
                    case 0x1ad: // SHRD
                        mem8 = phys_mem8[physmem8_ptr++];
                        y = regs[(mem8 >> 3) & 7];
                        z = regs[1];
                        conditional_var = (OPbyte >> 3) & 1;
                        if ((mem8 >> 6) == 3) {
                            reg_idx0 = mem8 & 7;
                            set_lower_word_in_register(reg_idx0, op_16_SHRD_SHLD(conditional_var, regs[reg_idx0], y, z));
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_16bits_mem8_write();
                            x = op_16_SHRD_SHLD(conditional_var, x, y, z);
                            st16_mem8_write(x);
                        }
                        goto EXEC_LOOP;
                    case 0x1ba: // G8 (-, -, -, -, BT, BTS, BTR, BTC)
                        mem8 = phys_mem8[physmem8_ptr++];
                        conditional_var = (mem8 >> 3) & 7;
                        switch (conditional_var) {
                        case 4:
                            if ((mem8 >> 6) == 3) {
                                x = regs[mem8 & 7];
                                y = phys_mem8[physmem8_ptr++];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                y = phys_mem8[physmem8_ptr++];
                                x = ld_16bits_mem8_read();
                            }
                            op_16_BT(x, y);
                            break;
                        case 5:
                        case 6:
                        case 7:
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                y = phys_mem8[physmem8_ptr++];
                                regs[reg_idx0] = op_16_BTS_BTR_BTC(conditional_var & 3, regs[reg_idx0], y);
                            } else {
                                mem8_loc = segment_translation(mem8);
                                y = phys_mem8[physmem8_ptr++];
                                x = ld_16bits_mem8_write();
                                x = op_16_BTS_BTR_BTC(conditional_var & 3, x, y);
                                st16_mem8_write(x);
                            }
                            break;
                        default:
                            abort(6);
                        }
                        goto EXEC_LOOP;
                    case 0x1a3: // BT
                        mem8 = phys_mem8[physmem8_ptr++];
                        y = regs[(mem8 >> 3) & 7];
                        if ((mem8 >> 6) == 3) {
                            x = regs[mem8 & 7];
                        } else {
                            mem8_loc = segment_translation(mem8);
                            mem8_loc = (mem8_loc + (((y & 0xffff) >> 4) << 1)) >> 0;
                            x = ld_16bits_mem8_read();
                        }
                        op_16_BT(x, y);
                        goto EXEC_LOOP;
                    case 0x1ab: // BTS
                    case 0x1b3: // BTR
                    case 0x1bb: // BTC
                        mem8 = phys_mem8[physmem8_ptr++];
                        y = regs[(mem8 >> 3) & 7];
                        conditional_var = (OPbyte >> 3) & 3;
                        if ((mem8 >> 6) == 3) {
                            reg_idx0 = mem8 & 7;
                            set_lower_word_in_register(reg_idx0, op_16_BTS_BTR_BTC(conditional_var, regs[reg_idx0], y));
                        } else {
                            mem8_loc = segment_translation(mem8);
                            mem8_loc = (mem8_loc + (((y & 0xffff) >> 4) << 1)) >> 0;
                            x = ld_16bits_mem8_write();
                            x = op_16_BTS_BTR_BTC(conditional_var, x, y);
                            st16_mem8_write(x);
                        }
                        goto EXEC_LOOP;
                    case 0x1bc: // BSF
                    case 0x1bd: // BSR
                        mem8 = phys_mem8[physmem8_ptr++];
                        reg_idx1 = (mem8 >> 3) & 7;
                        if ((mem8 >> 6) == 3) {
                            y = regs[mem8 & 7];
                        } else {
                            mem8_loc = segment_translation(mem8);
                            y = ld_16bits_mem8_read();
                        }
                        x = regs[reg_idx1];
                        if (OPbyte & 1) {
                            x = op_16_BSR(x, y);
                        } else {
                            x = op_16_BSF(x, y);
                        }
                        set_lower_word_in_register(reg_idx1, x);
                        goto EXEC_LOOP;
                    case 0x1b1: // -
                        mem8 = phys_mem8[physmem8_ptr++];
                        reg_idx1 = (mem8 >> 3) & 7;
                        if ((mem8 >> 6) == 3) {
                            reg_idx0 = mem8 & 7;
                            x = regs[reg_idx0];
                            y = do_16bit_math(5, regs[0], x);
                            if (y == 0) {
                                set_lower_word_in_register(reg_idx0, regs[reg_idx1]);
                            } else {
                                set_lower_word_in_register(0, x);
                            }
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x = ld_16bits_mem8_write();
                            y = do_16bit_math(5, regs[0], x);
                            if (y == 0) {
                                st16_mem8_write(regs[reg_idx1]);
                            } else {
                                set_lower_word_in_register(0, x);
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
                    case 0x1b0: // -
                        OPbyte = 0x0f;
                        physmem8_ptr--;
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
    cycles_processed += (cycles_requested - cycles_remaining);
    eip = (eip + physmem8_ptr - initial_mem_ptr);
}
