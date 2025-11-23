#ifndef _X86_H
#define _X86_H

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "../CMOS.h"
#include "../KBD.h"
#include "../ringbuffer.h"

typedef struct SegmentDescriptor {
    int selector;
    uint32_t base;
    uint32_t limit;
    int flags;
} SegmentDescriptor;

typedef struct Interrupt {
    int id = -1; // 0-31 termed `Exceptions'
    int error_code;
} Interrupt;

class x86Internal {
  public:
    int tlb_pages[2048]{0};
    int tlb_pages_count = 0;
    int tlb_size = 0x100000; // 1M
    int *tlb_read_kernel;
    int *tlb_write_kernel;
    int *tlb_read_user;
    int *tlb_write_user;
    int *tlb_read; // current (user or kernel)
    int *tlb_write;
    int tlb_hash;

    int mem_size;

    uint8_t *phys_mem8;
    uint16_t *phys_mem16;
    uint32_t *phys_mem32;

    // EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    int regs[8];
    int eflags;

    uint32_t eip;
    uint32_t eip_linear;
/*
   Fetch address register
   
   The fetch address register (FAR) stores the physical memory address
   of the next byte to be retrieved in the current fetch cycle.
 */
    uint32_t far;       // fetch address register
    uint32_t far_start; // first fetch address of current cycle

    // ES, CS, SS, DS, FS, GS, LDT, TR
    SegmentDescriptor segs[7];
    int df; // direction Flag

    int cpl;  // current privilege level
    int dpl;  // descriptor privilege level
    int iopl; // IO privilege level

    SegmentDescriptor gdt; // GDT register
    SegmentDescriptor ldt; // LDT register
    SegmentDescriptor tr;  // task register
    SegmentDescriptor idt; // IDT register

    int cr0;
    int cr2;
    int cr3;
    int cr4; // 80486

    int halted = 0;

    Interrupt interrupt;

    int opcode; // sort of fetch data register (FDR, aka MDR)

/*
   Operand Size Mode

   The operand size mode (OSM) of an instruction defines how each status flag modified by
   the instruction is calculated from the source and destination operands of the instruction.
   Instructions with multiple operand sizes may have multiple OSM or share an OSM with some
   or all sizes. OSM is specific to this emulator and not part of the processor architecture,
   from which it was derived. There are 31 OSMs, which are encoded by integers 0 to 30.

                             +-------+----+----+-----------+
                             |  (implicit) OSM             |
   +-------------+-----------+-------+----+----+-----------+----+----+----+----+----+----+
   | instruction | condition | 8 bit | 16 | 32 | condition | OF | SF | ZF | AF | PF | CF |
   +-------------+-----------+-------+----+----+-----------+----+----+----+----+----+----+
   | AAA         |           | (i)24 |    |    |           | -  | -  | -  | TM | -  | M  |
   | AAS         |           | (i)24 |    |    |           | -  | -  | -  | TM | -  | M  |
   | AAD         |           |   12  |    |    |           | -  | M  | M  | -  | M  | -  |
   | AAM         |           |   12  |    |    |           | -  | M  | M  | -  | M  | -  |
   | DAA         |           | (i)24 |    |    |           | -  | M  | M  | TM | M  | TM |
   | DAS         |           | (i)24 |    |    |           | -  | M  | M  | TM | M  | TM |
   | ADC         |           |    0  |  1 |  2 | CF 0      | M  | M  | M  | M  | M  | TM |
   | ADC         |           |    3  |  4 |  5 | CF 1      | M  | M  | M  | M  | M  | TM |
   | ADD         |           |    0  |  1 |  2 |           | M  | M  | M  | M  | M  | M  |
   | ADD         |           |       |    |  8 | OP 0x81,  | M  | M  | M  | M  | M  | M  |
   |             |           |       |    |    | OP 0x83   | M  | M  | M  | M  | M  | M  |
   | SBB         |           |    6  |  7 |  8 | CF 0      | M  | M  | M  | M  | M  | TM |
   | SBB         |           |    9  | 10 | 11 | CF 1      | M  | M  | M  | M  | M  | TM |
   | SUB         |           |    6  |  7 |  8 |           | M  | M  | M  | M  | M  | M  |
   | CMP         |           |    6  |  7 |  8 |           | M  | M  | M  | M  | M  | M  |
   | CMPS        |           |       |    |    |           | M  | M  | M  | M  | M  | M  |
   | SCAS        |           |       |    |    |           | M  | M  | M  | M  | M  | M  |
   | NEG         |           |       |    |    |           | M  | M  | M  | M  | M  | M  |
   | DEC         |           |   28  | 29 | 30 |           | M  | M  | M  | M  | M  | U  |
   | INC         |           |   25  | 26 | 27 |           | M  | M  | M  | M  | M  | U  |
   | IMUL        |           |   21  | 22 | 23 |           | M  | -  | -  | -  | -  | M  |
   | MUL         |           |   21  | 22 | 23 |           | M  | -  | -  | -  | -  | M  |
   | RCL/RCR     | count  1  |   24  | 24 | 24 |           | M  | U  | U  | U  | U  | TM |
   | RCL/RCR     | count >1  |   24  | 24 | 24 |           | -  | U  | U  | U  | U  | TM |
   | ROL/ROR     | count  1  |   24  | 24 | 24 |           | M  | U  | U  | U  | U  | M  |
   | ROL/ROR     | count >1  |   24  | 24 | 24 |           | -  | U  | U  | U  | U  | M  |
   | SAR/SHR     | count  1  |   18  | 19 | 20 |           | M  | M  | M  | -  | M  | M  |
   | SAR/SHR     | count >1  |   18  | 19 | 20 |           | -  | M  | M  | -  | M  | M  |
   | SAL/SHL     | count  1  |   15  | 16 | 17 |           | M  | M  | M  | -  | M  | M  |
   | SAL/SHL     | count >1  |   15  | 16 | 17 |           | -  | M  | M  | -  | M  | M  |
   | SHLD        |           |       | 19 | 17 |           | -  | M  | M  | -  | M  | M  |
   | SHRD        |           |       | 19 | 20 |           | -  | M  | M  | -  | M  | M  |
   | BSF/BSR     |           |       |    | 14 |           | -  | -  | M  | -  | -  | -  |
   | BT/BT[SRC]  |           |       | 19 | 20 |           | -  | -  | -  | -  | -  | M  |
   | AND         |           |   12  | 13 | 14 |           | 0  | M  | M  | -  | M  | 0  |
   | OR          |           |   12  | 13 | 14 |           | 0  | M  | M  | -  | M  | 0  |
   | TEST        |           |   12  | 13 | 14 |           | 0  | M  | M  | -  | M  | 0  |
   | XOR         |           |   12  | 13 | 14 |           | 0  | M  | M  | -  | M  | 0  |
   +-------------+-----------+-------+----+----+-----------+----+----+----+----+----+----+
                                                                   (PM (1986), Appendix C)

   0 : instruction resets,
   T : tests,
   M : modifies flag,
   U : leaves flag undefined,
   - : does not affect flag.
 */
    int osm;
    int osm_src;
    int osm_dst;
/*
   `osm_preserved'/ `osm_dst_preserved' preserve OMS/ destination of instruction
   before INC/ DEC but not including INC/ DEC. This is for later calculation of CF
   which is not modified by INC/ DEC. CF calculation after one or more
   successive INC/ DEC is therefore based on the values for OSM, source and
   destination before INC/ DEC. It is not necessary to also preserve source,
   since INC/ DEC do not store the implicit value 1 in `osm_src', which therefore
   remains valid.
 */
    int ocm_preserved;
    int ocm_dst_preserved;

/*
   Instruction prefix register

   The instruction prefix register (IPR) captures the instruction prefixes
   of the current retrieval cycle, each in its own bit. IPR is specific to
   this emulator and not part of the processor architecture, from which it
   was derived.

   0x0001 ES segment override prefix    (0x26)
   0x0002 CS segment override prefix    (0x2e)
   0x0003 SS segment override prefix    (0x36)
   0x0004 DS segment override prefix    (0x3e)
   0x0005 FS segment override prefix    (0x64)
   0x0006 GS segment override prefix    (0x65)
   0x0010 REPZ  string operation prefix (0xf3)
   0x0020 REPNZ string operation prefix (0xf2)
   0x0040 LOCK  signal prefix           (0xf0)
   0x0080 address-size override prefix  (0x67)
   0x0100 operand-size override prefix  (0x66)
 */
    int ipr; // instruction prefix register
    int ipr_default; // reflects D flag (PM (1986), 16.1)
                     // also belongs to the SSB (below)
/*
   Segments state block

   Variables in the segment state block (SSB) reflect code and stack segment
   sizes as well as address and operand size prefixes.
 */
    int CS_base; // shortcut for segs[1].base
    int SS_base; // shortcut for segs[2].base
    int SS_mask; // 16 or 32 bit SS size

    // https://en.wikipedia.org/wiki/X86_memory_segmentation
    bool x86_64_long_mode = false;

    // intermediate values
    int operation; // bits 5..3 of opcode or modR/M byte
    int reg_idx0, reg_idx1; // register indices (0-7)
    int x, y, z, v;         // anything else

    // linear byte address...
    uint32_t mem8_loc;
    int mem8; // ...and value

    int cycles_requested;
    int cycles_remaining;
    int cycles_processed;

    // clang-format off
    const std::vector<int> parity_LUT = {
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
        0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
        0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
        0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
        0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
        0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
        0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
        0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
        0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
    };
    const std::vector<int> do_shift8_LUT  = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2, 3, 4, 5, 6,
        7, 8, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2, 3, 4
    };
    const std::vector<int> do_shift16_LUT = {
         0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 0, 1, 2, 3, 4, 5, 6, 7, 8,  9, 10, 11, 12, 13, 14
    };
    // clang-format on

    x86Internal(int mem_size);
    ~x86Internal();

    void reset() {
        // Intel IA-32 SDM (latest), Vol. 3A, 11.1.1
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
        cr0 = (1 << 4); // 80387 present
    }

    void st8_phys(int mem8_loc, std::string str) {
        auto s = str.c_str();
        for (int i = 0; i < str.length(); i++) {
            st8_phys(mem8_loc++, s[i] & 0xff);
        }
        st8_phys(mem8_loc, 0);
    }
    uint8_t ld8_phys(int mem8_loc) {
        return phys_mem8[mem8_loc];
    }
    void st8_phys(int mem8_loc, uint8_t x) {
        phys_mem8[mem8_loc] = x;
    }
    int ld32_phys(int mem8_loc) {
        return phys_mem32[mem8_loc >> 2];
    }
    void st32_phys(int mem8_loc, int x) {
        phys_mem32[mem8_loc >> 2] = x;
    }
    void tlb_set_page(uint32_t linear_address, int pte, int tlb_set_write, int tlb_set_user) {
        int tlb_hash = linear_address ^ pte; // poor man's XOR hash
        uint32_t lat20 = linear_address >> 12;
        if (tlb_read_kernel[lat20] == -1) {
            if (tlb_pages_count >= 2048) {
                tlb_flush_all((lat20 - 1) & 0xfffff);
            }
            tlb_pages[tlb_pages_count++] = lat20;
        }
        tlb_read_kernel[lat20] = tlb_hash;
        if (tlb_set_write) {
            tlb_write_kernel[lat20] = tlb_hash;
        } else {
            tlb_write_kernel[lat20] = -1;
        }
        if (tlb_set_user) {
            tlb_read_user[lat20] = tlb_hash;
            if (tlb_set_write) {
                tlb_write_user[lat20] = tlb_hash;
            } else {
                tlb_write_user[lat20] = -1;
            }
        } else {
            tlb_read_user[lat20] = -1;
            tlb_write_user[lat20] = -1;
        }
    }
    void tlb_flush_page(uint32_t linear_address) {
        uint32_t lat20 = linear_address >> 12;
        tlb_clear(lat20);
    }
    void tlb_flush_all() {
        for (int i = 0; i < tlb_pages_count; i++) {
            uint32_t lat20 = tlb_pages[i];
            tlb_clear(lat20);
        }
        tlb_pages_count = 0;
    }
    void tlb_flush_all(uint32_t lat20) {
        int n = 0;
        for (int i = 0; i < tlb_pages_count; i++) {
            uint32_t tlb_lat20 = tlb_pages[i];
            if (tlb_lat20 == lat20) {
                tlb_pages[n++] = tlb_lat20;
            } else {
                tlb_clear(tlb_lat20);
            }
        }
        tlb_pages_count = n;
    }
    void tlb_clear(uint32_t lat20) {
        tlb_read_kernel[lat20] = -1;
        tlb_write_kernel[lat20] = -1;
        tlb_read_user[lat20] = -1;
        tlb_write_user[lat20] = -1;
    }

    std::string hex_rep(int x, int n) {
        std::string s;
        char h[] = "0123456789ABCDEF";
        for (int i = n - 1; i >= 0; i--) {
            s = s + h[(x >> (i * 4)) & 15];
        }
        return s;
    }
    std::string _4_bytes_(int n) {
        return hex_rep(n, 8);
    }
    std::string _2_bytes_(int n) {
        return hex_rep(n, 2);
    }
    std::string _1_byte_(int n) {
        return hex_rep(n, 4);
    }

    void fetch_decode_execute(int cycles);
    void update_SSB();

    int instruction_length(int opcode, int eip_linear);

    void set_CR0(int Qd);
    void set_CR3(int new_pdb);
    void set_CR4(int newval);
    bool check_real__v86();
    bool check_protected();

    void fetch_opcode();

    virtual int get_hard_irq() = 0;
    virtual int get_hard_intno() = 0;

    virtual int ioport_read(int mem8_loc) = 0;
    virtual void ioport_write(int mem8_loc, int data) = 0;

    int ld8_port(int port_num);
    int ld16_port(int port_num);
    int ld32_port(int port_num);
    void st8_port(int port_num, int x);
    void st16_port(int port_num, int x);
    void st32_port(int port_num, int x);

    void do_tlb_set_page(int Gd, int Hd, bool ja);
    int do_tlb_lookup(int mem8_loc, int ud);

    int __ld8_mem8_kernel_read();
    int ld8_mem8_kernel_read();
    int __ld16_mem8_kernel_read();
    int ld16_mem8_kernel_read();
    int __ld32_mem8_kernel_read();
    int ld32_mem8_kernel_read();
    int ld16_mem8_direct();
    int __ld8_mem8_read();
    int ld8_mem8_read();
    int __ld16_mem8_read();
    int ld16_mem8_read();
    int __ld32_mem8_read();
    int ld32_mem8_read();
    int __ld8_mem8_write();
    int ld8_mem8_write();
    int __ld16_mem8_write();
    int ld16_mem8_write();
    int __ld32_mem8_write();
    int ld32_mem8_write();
    void __st8_mem8_write(int x);
    void st8_mem8_write(int x);
    void __st16_mem8_write(int x);
    void st16_mem8_write(int x);
    void __st32_mem8_write(int x);
    void st32_mem8_write(int x);
    void __st8_mem8_kernel_write(int x);
    void st8_mem8_kernel_write(int x);
    void __st16_mem8_kernel_write(int x);
    void st16_mem8_kernel_write(int x);
    void __st32_mem8_kernel_write(int x);
    void st32_mem8_kernel_write(int x);

    int segment_translation(int mem8);
    int convert_offset_to_linear(bool writable);
    void update_segment_register(int reg_idx, int selector, uint32_t base, uint32_t limit, int flags);
    void set_segment_register(int reg_idx, int selector);
    void set_segment_register_real__v86(int reg_idx, int selector);
    void set_segment_register_protected(int reg_idx, int selector);
    int is_segment_accessible(int selector, bool writable);

    void set_lower_byte(int reg_idx, int x);
    void set_lower_word(int reg_idx, int x);

    int do_arithmetic8(int operation, int Yb, int Zb);
    int do_arithmetic16(int operation, int Yb, int Zb);
    int do_arithmetic32(int operation, int Yb, int Zb);
    int op_INC8(int x);
    int op_INC16(int x);
    int op_DEC8(int x);
    int op_DEC16(int x);
    int do_shift8(int operation, int Yb, int Zb);
    int do_shift16(int operation, int Yb, int Zb);
    int do_shift32(int operation, uint32_t Yb, int Zb);

    int op_SHRD_SHLD16(int operation, int Yb, int Zb, int pc);
    int op_SHLD(int Yb, int Zb, int pc);
    int op_SHRD(int Yb, int Zb, int pc);
    void op_BT16(int Yb, int Zb);
    void op_BT(int Yb, int Zb);
    int op_BTS_BTR_BTC16(int operation, int Yb, int Zb);
    int op_BTS_BTR_BTC(int operation, int Yb, int Zb);
    int op_BSF16(int Yb, int Zb);
    int op_BSF(int Yb, int Zb);
    int op_BSR16(int Yb, int Zb);
    int op_BSR(int Yb, int Zb);
    void op_DIV(int opcode);
    void op_IDIV(int opcode);
    void op_DIV16(int opcode);
    void op_IDIV16(int opcode);
    int op_DIV32(uint32_t Ic, uint32_t Jc, uint32_t opcode);
    int op_IDIV32(int Ic, int Jc, int opcode);
    int op_MUL(int a, int opcode);
    int op_IMUL(int a, int opcode);
    int op_MUL16(int a, int opcode);
    int op_IMUL16(int a, int opcode);
    int do_multiply32(int _a, int opcode);
    int op_MUL32(int a, int opcode);
    int op_IMUL32(int a, int opcode);

    bool is_CF();
    bool is_OF();
    bool is_BE();
    int is_PF();
    int is_LT();
    int is_LE();
    int is_AF();
    int can_jump(int condition);
    int compile_flags(bool shift = false);

    int get_EFLAGS();
    void set_EFLAGS(int flag_bits, int ld);

    void abort(int interrupt_id, int error_code = 0);

    void set_current_permission_level(int value);

    void push_word(int x);
    void push_dword(int x);
    void pop_word();
    void pop_dword();
    int read_stack_word();
    int read_stack_dword();

    int compile_sizemask(int dte_upper_dword);

    void load_xdt_descriptor(int *descriptor_table_entry, int selector);
    void load_tss_descriptor(int *descriptor_table_entry, int dpl);
    int compile_dte_limit(int dte_lower_dword, int dte_upper_dword);
    int compile_dte_base(int dte_lower_dword, int dte_upper_dword);
    void compile_segment_descriptor(SegmentDescriptor *sd, int dte_lower_dword, int dte_upper_dword);

    void do_interrupt_protected_mode(int interrupt_id, int ne, int error_code, int oe, int pe);
    void do_interrupt_real__v86_mode(int interrupt_id, int ne, int error_code, int oe, int pe);
    void do_interrupt(int interrupt_id, int ne, int error_code, int oe, int pe);

    void op_LDTR(int selector);
    void op_LTR(int selector);
    void do_JMPF_virtual_mode(int selector, int Le);
    void do_JMPF(int selector, int Le);
    void op_JMPF(int selector, int Le);
    void op_CALLF_real__v86_mode(bool is_32_bit, int selector, int Le, int oe);
    void op_CALLF_protected_mode(bool is_32_bit, int selector, int Le, int oe);
    void op_CALLF(bool is_32_bit, int selector, int Le, int oe);
    void do_return_real__v86_mode(bool is_32_bit, bool is_iret, int imm16);
    void do_return_protected_mode(bool is_32_bit, bool is_iret, int imm16);
    void Pe(int reg_idx, int cpl);
    void op_IRET(bool is_32_bit);
    void op_RETF(bool is_32_bit, int imm16);
    void op_LAR_LSL(bool is_32_bit, bool is_lsl);
    int of(int selector, bool is_lsl);
    void op_VERR_VERW(int selector, bool is_verw);
    void op_ARPL();
    void op_CPUID();
    void op_AAM(int base);
    void op_AAD(int base);
    void op_AAA();
    void op_AAS();
    void op_DAA();
    void op_DAS();
    void op_BOUND16();
    void op_BOUND();
    void op_PUSHA16();
    void op_PUSHA();
    void op_POPA16();
    void op_POPA();
    void op_LEAVE16();
    void op_LEAVE();
    void op_ENTER16();
    void op_ENTER();
    void op_load_far_pointer32(int Sb);
    void op_load_far_pointer16(int Sb);
    void op_INSB();
    void op_OUTSB();
    void op_MOVSB();
    void op_STOSB();
    void op_CMPSB();
    void op_LODSB();
    void op_SCASB();
    void op_INS16();
    void op_OUTS16();
    void op_MOVS16();
    void op_STOS16();
    void op_CMPS16();
    void op_LODS16();
    void op_SCAS16();
    void op_INSW();
    void op_OUTSW();
    void op_MOVSW();
    void op_STOSW();
    void op_CMPSW();
    void op_LODSW();
    void op_SCASW();
    void op_INSD();
    void op_OUTSD();
    void op_MOVSD();
    void op_STOSD();
    void op_CMPSD();
    void op_LODSD();
    void op_SCASD();
};
#endif // _X86_H
