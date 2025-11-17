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

typedef struct DescriptorTable {
    int selector = 0;
    uint32_t base = 0;
    uint32_t limit = 0;
    int flags = 0;
} DescriptorTable;

typedef struct ErrorInfo {
    int intno = -1;
    int error_code = 0;
} ErrorInfo;

class x86Internal {
  public:
    int tlb_pages[2048]{0};
    int tlb_pages_count = 0;
    int tlb_size = 0x100000; // 1M
    int *tlb_read_kernel = nullptr;
    int *tlb_write_kernel = nullptr;
    int *tlb_read_user = nullptr;
    int *tlb_write_user = nullptr;
    int *tlb_read = nullptr; // current (user or kernel)
    int *tlb_write = nullptr;
    int last_tlb_val; // tlb_hash_value

    uint8_t *phys_mem = nullptr;
    uint8_t *phys_mem8 = nullptr;
    uint16_t *phys_mem16 = nullptr;
    uint32_t *phys_mem32 = nullptr;

    // EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    int regs[8]{0};
    int eflags = 0x2;

    int eip = 0xfff0;
    int eip_offset = 0;

    // clang-format off
    // ES, CS, SS, DS, FS, GS, LDT, TR
    DescriptorTable segs[7] = {
        {0, 0, 0, 0}, 
        {0, 0xffff0000, 0, 0}, // CS
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0}};
    // clang-format on
    int df = 1; // direction Flag

    int cpl = 0;  // current privilege level
    int dpl = 0;  // descriptor privilege level
    int iopl = 0; // IO privilege level

    DescriptorTable gdt; // GDT register
    DescriptorTable ldt; // LDT register
    DescriptorTable tr;  // task register
    DescriptorTable idt = {0, 0, 0x03ff, 0}; // IDT register

    int cr0 = 0;
    int cr2 = 0;
    int cr3 = 0;
    int cr4 = 0;

    int OPbyte = 0;

    int cc_src = 0;
    int cc_dst = 0;
    int cc_op = 0;
    int cc_op2 = 0;
    int cc_dst2 = 0;
    int cccc_op = 0;   // current op
    int cccc_dst = 0;  // current dest
    int cccc_src = 0;  // current src
    int cccc_op2 = 0;  // current op, byte2
    int cccc_dst2 = 0; // current dest, byte2

    // 0x0001 ES segment override prefix    (0x26)
    // 0x0002 CS segment override prefix    (0x2e)
    // 0x0003 SS segment override prefix    (0x36)
    // 0x0004 DS segment override prefix    (0x3e)
    // 0x0005 FS segment override prefix    (0x64)
    // 0x0006 GS segment override prefix    (0x65)
    // 0x0010 REPZ  string operation prefix (0xf3)
    // 0x0020 REPNZ string operation prefix (0xf2)
    // 0x0040 LOCK  signal prefix           (0xf0)
    // 0x0080 address-size override prefix  (0x67)
    // 0x0100 operand-size override prefix  (0x66)
    int CS_flags = 0;
    int init_CS_flags = 0; // reflects D flag (PM (1986), 16.1)

    int CS_base;
    int SS_base;
    int SS_mask = -1; // 16 or 32 bit SS size

    // https://en.wikipedia.org/wiki/X86_memory_segmentation
    bool x86_64_long_mode = false;

    int conditional_var = 0; // opcode_543 bits 5, 4, and 3 of opcode or modR/M byte
    int mem8; // byte_value
    int reg_idx0, reg_idx1;  // register indices (0-7)
    int x, y, z, v;          // intermediate values

    int physmem8_ptr = 0;    // fetch_address
    int initial_mem_ptr = 0; // fetch_address_byte0
    uint32_t mem8_loc;       // byte_address

    int cycles_requested = 0;
    int cycles_remaining = 0;
    int cycles_processed = 0;

    int halted = 0;

    ErrorInfo interrupt;

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

    int mem_size = 16 * 1024 * 1024;
    int new_mem_size = mem_size + ((15 + 3) & ~3);

    x86Internal(int _mem_size);
    ~x86Internal();

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
        uint32_t mem8_locu = mem8_loc;
        phys_mem32[mem8_locu >> 2] = x;
    }
    void tlb_set_page(int mem8_loc, int page_val, int set_write_tlb, int set_user_tlb) {
        mem8_loc &= -4096; // top 20 bits matter
        page_val &= -4096;
        uint32_t mem8_locu = mem8_loc;
        int x = mem8_locu ^ page_val; // poor man's XOR hashing
        int i = mem8_locu >> 12;
        if (tlb_read_kernel[i] == -1) {
            if (tlb_pages_count >= 2048) {
                tlb_flush_all1((i - 1) & 0xfffff);
            }
            tlb_pages[tlb_pages_count++] = i;
        }
        tlb_read_kernel[i] = x;
        if (set_write_tlb) {
            tlb_write_kernel[i] = x;
        } else {
            tlb_write_kernel[i] = -1;
        }
        if (set_user_tlb) {
            tlb_read_user[i] = x;
            if (set_write_tlb) {
                tlb_write_user[i] = x;
            } else {
                tlb_write_user[i] = -1;
            }
        } else {
            tlb_read_user[i] = -1;
            tlb_write_user[i] = -1;
        }
    }
    void tlb_flush_page(int mem8_loc) {
        uint32_t mem8_locu = mem8_loc;
        int i = mem8_locu >> 12;
        tlb_clear(i);
    }
    void tlb_flush_all() {
        int n = tlb_pages_count;
        for (int j = 0; j < n; j++) {
            int i = tlb_pages[j];
            tlb_clear(i);
        }
        tlb_pages_count = 0;
    }
    void tlb_flush_all1(int la) {
        int n = tlb_pages_count;
        int new_n = 0;
        for (int j = 0; j < n; j++) {
            int i = tlb_pages[j];
            if (i == la) {
                tlb_pages[new_n++] = i;
            } else {
                tlb_clear(i);
            }
        }
        tlb_pages_count = new_n;
    }
    void tlb_clear(int i) {
        tlb_read_kernel[i] = -1;
        tlb_write_kernel[i] = -1;
        tlb_read_user[i] = -1;
        tlb_write_user[i] = -1;
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

    void instruction(int cycles);
    int init(int cycles);
    int check_halted();
    void check_interrupt();
    void init_segment_local_vars();

    int operation_size_function(int eip_offset, int OPbyte);

    void set_CR0(int Qd);
    void set_CR3(int new_pdb);
    void set_CR4(int newval);
    bool check_real_mode();
    bool check_protected();

    void check_opbyte();

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
    int __ld_8bits_mem8_read();
    int ld_8bits_mem8_read();
    int __ld_16bits_mem8_read();
    int ld_16bits_mem8_read();
    int __ld_32bits_mem8_read();
    int ld_32bits_mem8_read();
    int __ld_8bits_mem8_write();
    int ld_8bits_mem8_write();
    int __ld_16bits_mem8_write();
    int ld_16bits_mem8_write();
    int __ld_32bits_mem8_write();
    int ld_32bits_mem8_write();
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
    int segmented_mem8_loc_for_MOV(bool is_verw);
    void set_segment_vars(int ee, int selector, uint32_t base, uint32_t limit, int flags);
    void init_segment_vars_with_selector(int Sb, int selector);
    void set_protected_mode_segment_register(int reg, int selector);
    void set_segment_register(int reg, int selector);
    int segment_isnt_accessible(int selector, bool is_verw);

    void set_word_in_register(int reg_idx1, int x);
    void set_lower_word_in_register(int reg_idx1, int x);

    int do_32bit_math(int conditional_var, int Yb, int Zb);
    int do_16bit_math(int conditional_var, int Yb, int Zb);
    int do_8bit_math(int conditional_var, int Yb, int Zb);
    int increment_16bit(int x);
    int decrement_16bit(int x);
    int increment_8bit(int x);
    int decrement_8bit(int x);
    int shift8(int conditional_var, int Yb, int Zb);
    int shift16(int conditional_var, int Yb, int Zb);
    int shift32(int conditional_var, uint32_t Yb, int Zb);

    int op_16_SHRD_SHLD(int conditional_var, int Yb, int Zb, int pc);
    int op_SHLD(int Yb, int Zb, int pc);
    int op_SHRD(int Yb, int Zb, int pc);
    void op_16_BT(int Yb, int Zb);
    void op_BT(int Yb, int Zb);
    int op_16_BTS_BTR_BTC(int conditional_var, int Yb, int Zb);
    int op_BTS_BTR_BTC(int conditional_var, int Yb, int Zb);
    int op_16_BSF(int Yb, int Zb);
    int op_BSF(int Yb, int Zb);
    int op_16_BSR(int Yb, int Zb);
    int op_BSR(int Yb, int Zb);
    void op_DIV(int OPbyte);
    void op_IDIV(int OPbyte);
    void op_16_DIV(int OPbyte);
    void op_16_IDIV(int OPbyte);
    int op_DIV32(uint32_t Ic, uint32_t Jc, uint32_t OPbyte);
    int op_IDIV32(int Ic, int Jc, int OPbyte);
    int op_MUL(int a, int OPbyte);
    int op_IMUL(int a, int OPbyte);
    int op_16_MUL(int a, int OPbyte);
    int op_16_IMUL(int a, int OPbyte);
    int do_multiply32(int _a, int cc_opbyte);
    int op_MUL32(int a, int OPbyte);
    int op_IMUL32(int a, int OPbyte);

    bool check_carry();
    bool check_overflow();
    bool check_below_or_equal();
    int check_parity();
    int check_less_than();
    int check_less_or_equal();
    int check_adjust_flag();
    int check_status_bits_for_jump(int gd);
    int conditional_flags_for_rot_shiftcc_ops();
    int get_conditional_flags();

    int get_FLAGS();
    void set_FLAGS(int flag_bits, int ld);

    void abort_with_error_code(int intno, int error_code);
    void abort(int intno);

    void change_permission_level(int sd);

    void push_word_to_stack(int x);
    void push_dword_to_stack(int x);
    int pop_word_from_stack_read();
    void pop_word_from_stack_incr_ptr();
    int pop_dword_from_stack_read();
    void pop_dword_from_stack_incr_ptr();

    int SS_mask_from_flags(int desp_high4);

    void load_from_descriptor_table(int selector, int *desary);
    void load_from_TR(int he, int *desary);
    int calc_desp_limit(int desp_low4, int desp_high4);
    int calc_desp_base(int desp_low4, int desp_high4);
    void set_descriptor_register(DescriptorTable *descriptor_table, int desp_low4, int desp_high4);

    void do_interrupt_protected_mode(int intno, int ne, int error_code, int oe, int pe);
    void do_interrupt_not_protected_mode(int intno, int ne, int error_code, int oe, int pe);
    void do_interrupt(int intno, int ne, int error_code, int oe, int pe);

    void op_LDTR(int selector);
    void op_LTR(int selector);
    void do_JMPF_virtual_mode(int selector, int Le);
    void do_JMPF(int selector, int Le);
    void op_JMPF(int selector, int Le);
    void op_CALLF_not_protected_mode(bool is_32_bit, int selector, int Le, int oe);
    void op_CALLF_protected_mode(bool is_32_bit, int selector, int Le, int oe);
    void op_CALLF(bool is_32_bit, int selector, int Le, int oe);
    void do_return_not_protected_mode(bool is_32_bit, bool is_iret, int imm16);
    void do_return_protected_mode(bool is_32_bit, bool is_iret, int imm16);
    void Pe(int reg, int cpl_var);
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
    void checkOp_BOUND();
    void op_16_BOUND();
    void op_16_PUSHA();
    void op_PUSHA();
    void op_16_POPA();
    void op_POPA();
    void op_16_LEAVE();
    void op_LEAVE();
    void op_16_ENTER();
    void op_ENTER();
    void op_16_load_far_pointer32(int Sb);
    void op_16_load_far_pointer16(int Sb);
    void stringOp_INSB();
    void stringOp_OUTSB();
    void stringOp_MOVSB();
    void stringOp_STOSB();
    void stringOp_CMPSB();
    void stringOp_LODSB();
    void stringOp_SCASB();
    void op_16_INS();
    void op_16_OUTS();
    void op_16_MOVS();
    void op_16_STOS();
    void op_16_CMPS();
    void op_16_LODS();
    void op_16_SCAS();
    void stringOp_INSW();
    void stringOp_OUTSW();
    void stringOp_MOVSW();
    void stringOp_STOSW();
    void stringOp_CMPSW();
    void stringOp_LODSW();
    void stringOp_SCASW();
    void stringOp_INSD();
    void stringOp_OUTSD();
    void stringOp_MOVSD();
    void stringOp_STOSD();
    void stringOp_CMPSD();
    void stringOp_LODSD();
    void stringOp_SCASD();
};
#endif // _X86_H
