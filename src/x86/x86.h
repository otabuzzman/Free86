#ifndef _X86_H
#define _X86_H

#include <fstream>
#include <vector>

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
    int data, y, z, v;         // anything else

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
    virtual ~x86Internal();

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

    [[noreturn]] void abort(int interrupt_id, int error_code = 0);

    void tlb_set_page(uint32_t linear_address, int pte, int writable, int user) {
        int tlb_hash = linear_address ^ pte; // poor man's XOR hash
        uint32_t lat20 = linear_address >> 12;
        if (tlb_read_kernel[lat20] == -1) {
            if (tlb_pages_count >= 2048) {
                tlb_flush_all((lat20 - 1) & 0xfffff);
            }
            tlb_pages[tlb_pages_count++] = lat20;
        }
        tlb_read_kernel[lat20] = tlb_hash;
        if (writable) {
            tlb_write_kernel[lat20] = tlb_hash;
        } else {
            tlb_write_kernel[lat20] = -1;
        }
        if (user) {
            tlb_read_user[lat20] = tlb_hash;
            if (writable) {
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

    void do_tlb_set_page(int linear_address, int writable, bool user);
    int do_tlb_lookup(int linear_address, int writable);

    void fetch_decode_execute(int cycles);

    void fetch_opcode();
    void update_SSB();

    int instruction_length(int opcode);

    void set_CR0(int bits);
    void set_CR3(int bits);
    void set_CR4(int bits);
    bool check_real__v86();
    bool check_protected();
    void set_current_privilege_level(int data);

    virtual int get_hard_irq() = 0;
    virtual int get_hard_intno() = 0;

    virtual int ioport_read(int port_num) = 0;
    virtual void ioport_write(int port_num, int data) = 0;

    int ld8_port(int port_num);
    int ld16_port(int port_num);
    int ld32_port(int port_num);
    void st8_port(int port_num, int byte);
    void st16_port(int port_num, int word);
    void st32_port(int port_num, int dword);

    uint8_t ld8_phys(int address) {
        return phys_mem8[address];
    }
    void st8_phys(int address, uint8_t byte) {
        phys_mem8[address] = byte;
    }
    void st8_phys(int address, std::string data) {
        auto s = data.c_str();
        auto l = data.length();
        for (int i = 0; i < l; i++) {
            st8_phys(address++, s[i] & 0xff);
        }
        st8_phys(address, 0);
    }
    int ld32_phys(int address) {
        return phys_mem32[address >> 2];
    }
    void st32_phys(int address, int dword) {
        phys_mem32[address >> 2] = dword;
    }

    int __ld8_mem8_kernel_read();
    int ld8_mem8_kernel_read(); // from kernel RO memory: load (return) byte
    int __ld16_mem8_kernel_read();
    int ld16_mem8_kernel_read();  // ...word
    int __ld32_mem8_kernel_read();
    int ld32_mem8_kernel_read();  // ...dword

    int ld16_mem8_direct(); // read word at FAR from memory, bypass TLB

    int __ld8_mem8_read();
    int ld8_mem8_read(); // from user RO memory: load (return) byte
    int __ld16_mem8_read();
    int ld16_mem8_read(); // ...word
    int __ld32_mem8_read();
    int ld32_mem8_read(); // ...dword

    int __ld8_mem8_write();
    int ld8_mem8_write(); // from user WR memory: load (return) byte
    int __ld16_mem8_write();
    int ld16_mem8_write(); // ...word
    int __ld32_mem8_write();
    int ld32_mem8_write(); // ...dword

    void __st8_mem8_kernel_write(int byte);
    void st8_mem8_kernel_write(int byte); // in kernel WR memory: store byte
    void __st16_mem8_kernel_write(int word);
    void st16_mem8_kernel_write(int word); // ...word
    void __st32_mem8_kernel_write(int dword);
    void st32_mem8_kernel_write(int dword); // ...dword

    void __st8_mem8_write(int byte);
    void st8_mem8_write(int byte); // in user WR memory: store byte
    void __st16_mem8_write(int word);
    void st16_mem8_write(int word); // ...word
    void __st32_mem8_write(int dword);
    void st32_mem8_write(int dword); // ...dword

    void push_word(int word);
    void push_dword(int dword);
    void pop_word();
    void pop_dword();
    int read_stack_word();
    int read_stack_dword();

    void set_lower_byte(int reg, int byte);
    void set_lower_word(int reg, int word);

    int segment_translation(int mem8);
    int convert_offset_to_linear(bool writable);

    void update_segment_register(int sreg, int selector, uint32_t base, uint32_t limit, int flags);
    void set_segment_register(int sreg, int selector);
    void set_segment_register_real__v86(int sreg, int selector);
    void set_segment_register_protected(int sreg, int selector);
    int is_segment_accessible(int selector, bool writable);

    void load_xdt_descriptor(int *descriptor_table_entry, int selector);
    void load_tss_interlevel(int *descriptor_table_entry, int privilege_level);
    int compile_dte_base(int dte_lower_dword, int dte_upper_dword);
    int compile_dte_limit(int dte_lower_dword, int dte_upper_dword);
    void compile_segment_descriptor(SegmentDescriptor *sd, int dte_lower_dword, int dte_upper_dword);
    int compile_sizemask(int dte_upper_dword);

    int op_INC8(int data);
    int op_INC16(int data);
    int op_DEC8(int data);
    int op_DEC16(int data);
    int op_SHRD_SHLD16(int dst, int src, int count);
    int op_SHRD(int dst, int src, int count);
    int op_SHLD(int dst, int src, int count);
    void op_BT16(int bit_base, int bit_offset);
    void op_BT(int bit_base, int bit_offset);
    int op_BTS_BTR_BTC16(int bit_base, int bit_offset);
    int op_BTS_BTR_BTC(int bit_base, int bit_offset);
    int op_BSF16(int dst, int src);
    int op_BSF(int dst, int src);
    int op_BSR16(int dst, int src);
    int op_BSR(int dst, int src);
    void op_DIV8(int divisor);
    void op_IDIV8(int divisor);
    void op_DIV16(int divisor);
    void op_IDIV16(int divisor);
    int op_DIV32(uint32_t dividend_upper, uint32_t dividend_lower, uint32_t divisor);
    int op_IDIV32(int dividend_upper, int dividend_lower, int divisor);
    int op_MUL8(int multiplicand, int multiplier);
    int op_IMUL8(int multiplicand, int multiplier);
    int op_MUL16(int multiplicand, int multiplier);
    int op_IMUL16(int multiplicand, int multiplier);
    int op_MUL32(int multiplicand, int multiplier);
    int op_IMUL32(int multiplicand, int multiplier);

    int do_multiply32(int multiplicand, int multiplier);

    int do_arithmetic8(int dst, int src);
    int do_arithmetic16(int dst, int src);
    int do_arithmetic32(int dst, int src);

    int do_shift8(int src, int count);
    int do_shift16(int src, int count);
    int do_shift32(uint32_t src, int count);

    void op_LDTR(int selector);
    void op_LTR(int selector);
    void do_JMPF(int selector, int address);
    void op_JMPF_virtual_mode(int selector, int address);
    void op_JMPF(int selector, int address);
    void do_CALLF(bool is_operand_size32, int selector, int address, int return_address);
    void op_CALLF_real__v86_mode(bool is_operand_size32, int selector, int address, int return_address);
    void op_CALLF_protected_mode(bool is_operand_size32, int selector, int address, int return_address);
    void do_return_real__v86_mode(bool is_operand_size32, bool is_iret, int return_offset);
    void do_return_protected_mode(bool is_operand_size32, bool is_iret, int return_offset);
    void clear_segment_register(int sreg, int privilege_level);
    void op_IRET(bool is_operand_size32);
    void op_RETF(bool is_operand_size32, int return_offset);
    void op_LAR_LSL(bool is_operand_size32, bool is_lsl);
    int ld_descriptor_field(int selector, bool is_lsl);

    void do_interrupt(int interrupt_id, int is_sw, int error_code, int return_address, int is_hw);
    void do_interrupt_real__v86_mode(int interrupt_id, int is_sw, int error_code, int return_address, int is_hw);
    void do_interrupt_protected_mode(int interrupt_id, int is_sw, int error_code, int return_address, int is_hw);

    void op_VERR_VERW(int selector, bool is_verw);
    void op_ARPL();
    void op_CPUID();
    void op_AAM(int radix);
    void op_AAD(int radix);
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
    void ld_full_pointer16(int sreg);
    void ld_full_pointer32(int sreg);

    void op_INS16();
    void op_OUTS16();
    void op_MOVS16();
    void op_STOS16();
    void op_CMPS16();
    void op_LODS16();
    void op_SCAS16();

    void op_INSB();
    void op_OUTSB();
    void op_MOVSB();
    void op_STOSB();
    void op_CMPSB();
    void op_LODSB();
    void op_SCASB();
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

    bool is_CF(); // carry (bit 0)
    int is_PF(); // parity (bit 2)
    int is_AF(); // adjust (bit 4)
    bool is_OF(); // overflow (bit 11)
    bool is_BE(); // below or equal, signed comparison
    int is_LE(); // less or equal, unsigned comparison
    int is_LT(); // less than
    int can_jump(int condition);
    int compile_flags(bool shift = false);

    int get_EFLAGS();
    void set_EFLAGS(int bits, int mask);

    std::string hex_rep(int number, int digits) {
        std::string s;
        char h[] = "0123456789ABCDEF";
        for (int i = digits - 1; i >= 0; i--) {
            s = s + h[(number >> (i * 4)) & 15];
        }
        return s;
    }
    std::string _4_bytes(int number) {
        return hex_rep(number, 8);
    }
    std::string _2_bytes(int number) {
        return hex_rep(number, 4);
    }
    std::string _1_byte(int number) {
        return hex_rep(number, 2);
    }
};
#endif // _X86_H
