#ifndef FREE86_H
#define FREE86_H

#include <fstream>
#include <vector>

typedef struct SegmentDescriptor {
    uint32_t base;
    uint32_t limit;
    int flags;
    SegmentDescriptor(uint32_t base, uint32_t limit, int flags)
        : base(base), limit(limit), flags(flags) {}
    SegmentDescriptor(uint64_t qword) {
        base = ((qword >> 16) & 0x0000ffff) |
               ((qword >> 16) & 0x00ff0000) |
               ((qword >> 32) & 0xff000000);
        flags = (qword >> 32) & 0x00f0ff00;
        limit = ((qword >> 32) & 0x000f0000) | (qword & 0xffff);
        if (flags & (1 << 23)) {
            limit = (limit << 12) | 0xfff;
        }
    }
    uint64_t qword() {
        #pragma GCC diagnostic ignored "-Wshadow"
        uint32_t limit;
        if (flags & (1 << 23)) {
            limit = this->limit >> 12;
        } else {
            limit = this->limit;
        }
        return ((static_cast<uint64_t>(base) & 0x0000ffff) << 16 |
                (static_cast<uint64_t>(base) & 0x00ff0000) << 16 |
                (static_cast<uint64_t>(base) & 0xff000000) << 32 |
                (static_cast<uint64_t>(flags) & 0x00f0ff00) << 32 |
                (static_cast<uint64_t>(limit) & 0x000f0000) << 32 | (static_cast<uint64_t>(limit) & 0xffff));
    }
    int segment_size_mask() {
        return (flags & (1 << 22)) ? -1 : 0xffff;
    }
} SegmentDescriptor;

typedef struct SegmentRegister {
    int selector;
    SegmentDescriptor shadow;
    SegmentRegister() : selector(0), shadow(SegmentDescriptor(0)) {}
    SegmentRegister(int selector, SegmentDescriptor shadow) : selector(selector), shadow(shadow) {}
} SegmentRegister;

typedef struct Interrupt {
    int id; // 0-31 termed `Exceptions'
    int error_code;
} Interrupt;

class Free86 {
  public:
    // EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    int regs[8];
    int eflags;

    uint32_t eip;

    // ES, CS, SS, DS, FS, GS, LDT, TR
    SegmentRegister segs[7];

    SegmentRegister gdt; // GDT register
    SegmentRegister ldt; // LDT register
    SegmentRegister tr;  // task register
    SegmentRegister idt; // IDT register

    int cr0;
    int cr2;
    int cr3;
    int cr4; // 80486

    int cpl; // current privilege level register

    int halted;

    uint64_t cycles;

    int tlb_lookup(uint32_t linear, int writable) {
        uint32_t lat20 = linear >> 12; // PDE and PTE indices
        if (writable) {
            tlb_hash = tlb_writable[lat20];
        } else {
            tlb_hash = tlb_readonly[lat20];
        }
        if (tlb_hash == -1) {
            page_translation(linear, writable, cpl == 3);
            if (writable) {
                tlb_hash = tlb_writable[lat20];
            } else {
                tlb_hash = tlb_readonly[lat20];
            }
        }
        return linear ^ tlb_hash;
    }

    virtual int get_irq() = 0;
    virtual int get_iid() = 0;

    virtual int io_read(int port) = 0;
    virtual void io_write(int port, int data) = 0;

    // x86.cpp
    Free86(int memory_size);
    virtual ~Free86();

    void reset();

    // fedex.cpp
    void fetch_decode_execute(uint64_t cycles, Interrupt& interrupt);

    // memory.cpp
    int ld8_direct(int address); // read/ write at physical memory address (bypass TLB)
    void st8_direct(int address, int byte);
    void st8_direct(int address, std::string data);

  private:
    uint32_t eip_linear;
/*
   Fetch address register

   The fetch address register (FAR, aka MAR) stores the physical memory
   address of the next byte to be retrieved in the current fetch cycle.
 */
    uint32_t far;       // fetch address register (FAR, aka MAR)
    uint32_t far_start; // first fetch address of current cycle

    int opcode; // sort of fetch data register (FDR, aka MDR)

    int df; // values 1/ -1 reflect EFLAGS.DF false/ true (string instructions)

    int dpl;  // descriptor privilege level
    int rpl;  // requested privilege level
    int iopl; // IO privilege level

    uint64_t cycles_requested;
    uint64_t cycles_remaining;

    int memory_size;
    uint8_t *memory;

/*
   Translation lookaside buffer

   The translation lookaside buffer (TLB) maps linear addresses (LA) to
   physical addresses (PA). The top 20 bits of an LA represent the PD
   and PT indices used by the MMU to look up the PA. These 20 bits act
   as an index into an array containing the PA of the corresponding page.
   Consequently, each mapping table has 1M entries. There are four
   such tables, one for each combination of user, supervisor, read-only
   and writable page features. The PA stored in a table is XORed with
   its LA and then XORed again on retrieval. This is likely a design
   choice rather than a necessity. Another array records up to 2024
   mappings stored in the tables, effectively representing the logical
   size of the TLB.
 */
    uint32_t tlb_pages[2048]{0};
    int tlb_pages_count = 0;
    // mapping tables
    int *tlb_readonly_cplX; // supervisor, any CPL
    int *tlb_writable_cplX;
    int *tlb_readonly_cpl3; // user, CPL == 3
    int *tlb_writable_cpl3;
    // current mapping tables (user or supervisor)
    int *tlb_readonly;
    int *tlb_writable;
    int tlb_size = 0x100000; // 1M entries per MT
    int tlb_hash; // LA ^ PA (by design, no necessity)

    void tlb_update(uint32_t linear /*data*/, uint32_t physical /*key*/, int writable, int user) {
        tlb_hash = linear ^ physical; // poor man's XOR hash function
        uint32_t lat20 = linear >> 12; // PD and PT indices (top 20 bits of linear address)
        if (tlb_readonly_cplX[lat20] == -1) {
            if (tlb_pages_count >= 2048) { // flush TLB if full
                // if present, keep PTE immediately preceding this PTE to improve performance
                tlb_flush_all(lat20 - 1);
            }
            // record LA just added to TLB
            tlb_pages[tlb_pages_count++] = lat20;
        }
        // update mapping tables
        tlb_readonly_cplX[lat20] = tlb_hash;
        if (writable) {
            tlb_writable_cplX[lat20] = tlb_hash;
        } else {
            tlb_writable_cplX[lat20] = -1;
        }
        if (user) {
            tlb_readonly_cpl3[lat20] = tlb_hash;
            if (writable) {
                tlb_writable_cpl3[lat20] = tlb_hash;
            } else {
                tlb_writable_cpl3[lat20] = -1;
            }
        } else {
            tlb_readonly_cpl3[lat20] = -1;
            tlb_writable_cpl3[lat20] = -1;
        }
    }
    void tlb_flush_page(uint32_t linear) {
        uint32_t lat20 = linear >> 12;
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
        tlb_readonly_cplX[lat20] = -1;
        tlb_writable_cplX[lat20] = -1;
        tlb_readonly_cpl3[lat20] = -1;
        tlb_writable_cpl3[lat20] = -1;
    }

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
   of the current fetch cycle, each in its own bit. IPR is specific to
   this emulator and not part of the processor architecture.

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

   Variables in the segments state block (SSB) provide shorthand
   access to frequently used segment data.
 */
    int CS_base; // shortcut for segs[1].shadow.base
    int SS_base; // shortcut for segs[2].shadow.base
    int SS_mask; // 16/ 32 bit address size mask for SS (from descriptor)

    // https://en.wikipedia.org/wiki/X86_memory_segmentation
    bool x86_64_long_mode = false;
    // end of SSB

    // auxiliary variables for inter-method exchange
    int operation; // bits 5..3 of opcode or modR/M byte
    uint32_t lax;  // linear address exchange register
    int modRM, reg, rM;   // mod field (modRM >> 6) inline
    int sib, base, index; // scale field (sib >> 6) inline
    int r;  // data from register derived from modRM
    int rm; // data from register or memory derived from modRM
    int m, m16; // any data from memory not derived from modRM
    int ind, imm, imm16, moffs; // indirect, immediate and offset
    int ind1st, ind2nd, imm1st, imm2nd; // 1st/ 2nds of above
    int x, y, XS_mask; // address size mask for effective DS

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
    const std::vector<int> shift8_LUT  = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2, 3, 4, 5, 6,
        7, 8, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2, 3, 4
    };
    const std::vector<int> shift16_LUT = {
         0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 0, 1, 2, 3, 4, 5, 6, 7, 8,  9, 10, 11, 12, 13, 14
    };
    // clang-format on

    // x86.cpp
    [[noreturn]] void abort(int /*interrupt*/ id, int error_code = 0);

    void update_SSB();
    void fetch_opcode();

    int instruction_length(int opcode);
    int modRM_bytes_number();

    void set_CR0(int bits);
    void set_CR3(int bits);
    bool is_real__v86();
    bool is_protected();
    bool is_paging(); // PG && PE set
    void set_cpl(int level);

    void set_lower_byte(int reg, int byte);
    void set_lower_word(int reg, int word);

    void page_translation(int writable, int user);
    void page_translation(uint32_t address, int writable, int user);

    void segment_translation();
    void moffs_to_linear(bool writable);

    void set_segment_register(int sreg, int selector, uint32_t base, uint32_t limit, int flags);
    void set_segment_register(int sreg, int selector);
    void set_segment_register_real__v86(int sreg, int selector);
    void set_segment_register_protected(int sreg, int selector);

    SegmentDescriptor ld_xdt_entry(int selector);
    uint64_t ld_tss_stack(int privilege_level); // seg:offset

    int aux_INC8(int data);
    int aux_INC16(int data);
    int aux_DEC8(int data);
    int aux_DEC16(int data);
    int aux_SHRD16_SHLD16(int dst, int src, int count);
    int aux_SHRD(int dst, int src, int count);
    int aux_SHLD(int dst, int src, int count);
    void aux_BT16(int base, int offset);
    void aux_BT(int base, int offset);
    int aux_BTS16_BTR16_BTC16(int base, int offset);
    int aux_BTS_BTR_BTC(int base, int offset);
    int aux_BSF16(int dst, int src);
    int aux_BSF(int dst, int src);
    int aux_BSR16(int dst, int src);
    int aux_BSR(int dst, int src);
    void aux_DIV8(int divisor);
    void aux_DIV16(int divisor);
    void aux_DIV(uint32_t dividend_upper, uint32_t dividend_lower, uint32_t divisor);
    void aux_IDIV8(int divisor);
    void aux_IDIV16(int divisor);
    void aux_IDIV(int dividend_upper, int dividend_lower, int divisor);
    void aux_MUL8(int multiplicand, int multiplier);
    void aux_MUL16(int multiplicand, int multiplier);
    void aux_MUL(int multiplicand, int multiplier);
    void aux_IMUL8(int multiplicand, int multiplier);
    void aux_IMUL16(int multiplicand, int multiplier);
    void aux_IMUL(int multiplicand, int multiplier);
    void multiply(int multiplicand, int multiplier);

    int calculate8(int dst, int src);
    int calculate16(int dst, int src);
    int calculate(int dst, int src);

    int shift8(int src, int count);
    int shift16(int src, int count);
    int shift(uint32_t src, int count);

    void aux_LDTR(int selector);
    void aux_LTR(int selector);
    void aux_JMPF(int selector, int offset);
    void aux_JMPF_real__v86_mode(int selector, int offset);
    void aux_JMPF_protected_mode(int selector, int offset);
    void aux_CALLF(bool o32, int selector, int offset, int return_address);
    void aux_CALLF_real__v86_mode(bool o32, int selector, int offset, int return_address);
    void aux_CALLF_protected_mode(bool o32, int selector, int offset, int return_address);
    void return_real__v86_mode(bool o32, bool is_iret, int return_offset);
    void return_protected_mode(bool o32, bool is_iret, int return_offset);
    void zero_segment_register(int sreg, int privilege_level);
    void aux_RETF(bool o32, int return_offset);

    void raise_interrupt(int id, int error_code, int is_hw, int is_sw, int return_address);
    void raise_interrupt_real__v86_mode(int id, int is_sw, int return_address);
    void raise_interrupt_protected_mode(int id, int error_code, int is_hw, int is_sw, int return_address);
    void aux_IRET(bool o32);

    void aux_LAR_LSL(bool o32, bool is_lsl);
    int ld_descriptor_fields(int selector, bool is_lsl);
    void aux_VERR_VERW(int selector, bool is_verw);
    bool is_segment_accessible(int selector, bool writable);
    void aux_ARPL();
    void aux_CPUID();
    void aux_AAM(int radix);
    void aux_AAD(int radix);
    void aux_AAA();
    void aux_AAS();
    void aux_DAA();
    void aux_DAS();
    void aux_BOUND16();
    void aux_BOUND();
    void aux_PUSHA16();
    void aux_PUSHA();
    void aux_POPA16();
    void aux_POPA();
    void aux_LEAVE16();
    void aux_LEAVE();
    void aux_ENTER16();
    void aux_ENTER();
    void ld_far_pointer16(int sreg);
    void ld_far_pointer(int sreg);

    // string.cpp
    void aux_INS16();
    void aux_OUTS16();
    void aux_MOVS16();
    void aux_STOS16();
    void aux_CMPS16();
    void aux_LODS16();
    void aux_SCAS16();

    void aux_INSB();
    void aux_OUTSB();
    void aux_MOVSB();
    void aux_STOSB();
    void aux_CMPSB();
    void aux_LODSB();
    void aux_SCASB();
    void aux_INSW();
    void aux_OUTSW();
    void aux_MOVSW();
    void aux_STOSW();
    void aux_CMPSW();
    void aux_LODSW();
    void aux_SCASW();
    void aux_INSD();
    void aux_OUTSD();
    void aux_MOVSD();
    void aux_STOSD();
    void aux_CMPSD();
    void aux_LODSD();
    void aux_SCASD();

    // memory.cpp
    int ld8_readonly_cplX(); // from supervisor RO memory: load (return) byte
    int ld16_readonly_cplX();  // ...word
    int ld_readonly_cplX();  // ...dword
    uint64_t ld64_readonly_cplX();  // ...qword from current linear address

    int ld8_readonly_cpl3(); // from user RO memory: load (return) byte
    int ld16_readonly_cpl3(); // ...word
    int ld_readonly_cpl3(); // ...dword from current linear address

    int ld8_writable_cpl3(); // from user WR memory: load (return) byte
    int ld16_writable_cpl3(); // ...word
    int ld_writable_cpl3(); // ...dword from current linear address

    void st8_writable_cplX(int byte); // in supervisor WR memory: store byte
    void st16_writable_cplX(int word); // ...word
    void st_writable_cplX(int dword); // ...dword
    void st64_writable_cplX(uint64_t qword); // ...qword at current linear address

    void st8_writable_cpl3(int byte); // in user WR memory: store byte
    void st16_writable_cpl3(int word); // ...word
    void st_writable_cpl3(int dword); // ...dword at current linear address

    int ld16_direct(int address); // read/ write at memory address, bypass TLB
    int ld_direct(int address);
    uint64_t ld64_direct(int address);
    void st16_direct(int address, int byte);
    void st_direct(int address, int dword);
    void st64_direct(int address, uint64_t qword);

    int fetch_data8(); // read byte...
    int fetch_data16(); // ...word...
    int fetch_data(); // ...dword at FAR, update FAR
    void push16(int word);
    void push(int dword);
    int pop16();
    int pop();
    int ld16_stack();
    int ld_stack();

    // eflags.cpp
    bool is_CF(); // carry (bit 0)
    bool is_PF(); // parity (bit 2)
    bool is_AF(); // adjust (bit 4)
    bool is_OF(); // overflow (bit 11)
    bool is_BE(); // below or equal, signed comparison
    bool is_LE(); // less or equal, unsigned comparison
    bool is_LT(); // less than
    bool can_jump(int condition);
    int compile_eflags(bool shift = false);

    int get_EFLAGS();
    void set_EFLAGS(int bits, int mask);
};
#endif // FREE86_H
