#ifndef PC_H
#define PC_H

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>
#include <time.h>

#ifndef NO_SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#endif

#include "ringbuffer.h"
#include "x86/free86.h"

class WiredCPU;

class PC {
  public:
    PC(uint32_t memory_size);
    ~PC();

    size_t load(std::string path, uint32_t offset = 0);
    void setup();
    void cycle();

#ifndef NO_SDL
    void paint(SDL_Renderer *render, int widht, int height);
#else
    void print();
    void input();
#endif

  private:
    std::string state() {
        // EAX:00000000                ESP:CAFE55AA
        // ECX:00000000                EBP:CAFE55AA
        // EDX:00000000
        // EBX:00000000
        // ESI:00000000
        // EDI:00000000                EIP:CAFE55AA
        //
        //        EFLAGS:00010001_00010001_00001111
        //
        // CS:CAFE:CAFE55AA:CAFFE:00010001_00001111
        // SS:CAFE:CAFE55AA:CAFFE:00010001_00001111
        // DS:CAFE:CAFE55AA:CAFFE:00010001_00001111
        // ES:CAFE:CAFE55AA:CAFFE:00010001_00001111
        // FS:CAFE:CAFE55AA:CAFFE:00010001_00001111
        // GS:CAFE:CAFE55AA:CAFFE:00010001_00001111
        //
        // CR0:0..01111  CR2:DEADBEAF  CR3:DEADB000
        a = "EAX:" + hex(cpu->regs[0]);
        c = "ECX:" + hex(cpu->regs[1]);
        d = "EDX:" + hex(cpu->regs[2]);
        b = "EBX:" + hex(cpu->regs[3]);
        si = "ESI:" + hex(cpu->regs[6]);
        di = "EDI:" + hex(cpu->regs[7]);
        i = "EIP:" + hex((int) cpu->eip);
        sp = "ESP:" + hex(cpu->regs[4]);
        bp = "EBP:" + hex(cpu->regs[5]);
        flags = "EFLAGS:" + bin(cpu->eflags, true).substr(9);
        cs = "CS:" + hex((short) cpu->segs[1].selector) + ":" + hex(cpu->segs[1].shadow.base) + ":" + hex(cpu->segs[1].shadow.limit).substr(3, std::string::npos) + ":" + bin((short) cpu->segs[1].shadow.flags, true);
        ss = "SS:" + hex((short) cpu->segs[2].selector) + ":" + hex(cpu->segs[2].shadow.base) + ":" + hex(cpu->segs[2].shadow.limit).substr(3, std::string::npos) + ":" + bin((short) cpu->segs[2].shadow.flags, true);
        ds = "DS:" + hex((short) cpu->segs[3].selector) + ":" + hex(cpu->segs[3].shadow.base) + ":" + hex(cpu->segs[3].shadow.limit).substr(3, std::string::npos) + ":" + bin((short) cpu->segs[3].shadow.flags, true);
        es = "ES:" + hex((short) cpu->segs[0].selector) + ":" + hex(cpu->segs[0].shadow.base) + ":" + hex(cpu->segs[0].shadow.limit).substr(3, std::string::npos) + ":" + bin((short) cpu->segs[0].shadow.flags, true);
        fs = "FS:" + hex((short) cpu->segs[4].selector) + ":" + hex(cpu->segs[4].shadow.base) + ":" + hex(cpu->segs[4].shadow.limit).substr(3, std::string::npos) + ":" + bin((short) cpu->segs[4].shadow.flags, true);
        gs = "GS:" + hex((short) cpu->segs[5].selector) + ":" + hex(cpu->segs[5].shadow.base) + ":" + hex(cpu->segs[5].shadow.limit).substr(3, std::string::npos) + ":" + bin((short) cpu->segs[5].shadow.flags, true);
        cr0 = "CR0:" + bin(cpu->cr0).replace(1, 26, "..");
        cr2 = "CR2:" + hex(cpu->cr2);
        cr3 = "CR3:" + hex(cpu->cr3);
        return std::string(
            a + "                " + sp + "\n" +
            c + "                " + bp + "\n" +
            d + "\n" +
            b + "\n" +
            si + "\n" +
            di + "                " + i + "\n" +
            "\n" + "       " + flags + "\n" +
            "\n" + cs + "\n" +
            ss + "\n" +
            ds + "\n" +
            es + "\n" +
            fs + "\n" +
            gs + "\n" +
            "\n" + cr0 + "  " + cr2 + "  " + cr3
        );
    }
    std::string compact_state() {
        // A:DEADBEAF C:DEADBEAF D:DEADBEAF B:DEADBEAF SI:DEADBEAF DI:DEADBEAF I:CAFE55AA SP:CAFE55AA BP:CAFE55AA F:0001_00001111
        std::string a, c, d, b, si, di, i, sp, bp, flags;
        std::string cs, ss, ds, es, fs, gs, cr0, cr2, cr3;
        a = "A:" + hex(cpu->regs[0]);
        c = " C:" + hex(cpu->regs[1]);
        d = " D:" + hex(cpu->regs[2]);
        b = " B:" + hex(cpu->regs[3]);
        si = " SI:" + hex(cpu->regs[6]);
        di = " DI:" + hex(cpu->regs[7]);
        i = " I:" + hex((int) cpu->eip);
        sp = " SP:" + hex(cpu->regs[4]);
        bp = " BP:" + hex(cpu->regs[5]);
        flags = " F:" + bin(cpu->eflags, true).substr(22);
        if (compact) {
            return std::string(a + c + d + b + si + di + i + sp + bp + flags);
        }
    }
    std::string eip_15() {
        uint32_t linear, physical;
        int n;
        linear = cpu->segs[1].shadow.base + cpu->eip; // EIP is offset of linear address
        if (cpu->cr0 & 0x80000001) { // protected mode and paging enabled
            physical = cpu->tlb_lookup(linear, 0); // physical address
            n = 4096 - (linear & 0xfff); // print bytes left to end-of-page...
            n = std::min(n, 15); // ...or up to maximum instruction length bytes
        } else {
            linear &= 0xfffff;
            physical = linear;
            n = 15;
        }
        std::string memory = "[EIP..EIP+" + hex((char) (n - 1)) + "]:";
        for (int i = 0; i < n; i++) {
            memory += " " + hex((char) cpu->ld8_direct(physical + i));
        }
        return memory;
    }
    std::string bin(char bits) {
    #define V(byte) \
        ((byte) & 0x80 ? "1" : "0") + \
        ((byte) & 0x40 ? "1" : "0") + \
        ((byte) & 0x20 ? "1" : "0") + \
        ((byte) & 0x10 ? "1" : "0") + \
        ((byte) &  8 ? "1" : "0") + \
        ((byte) &  4 ? "1" : "0") + \
        ((byte) &  2 ? "1" : "0") + \
        ((byte) &  1 ? "1" : "0")
        std::string result = "";
        return result + V(bits);
    }
    std::string bin(short bits, bool divide = false) {
        return bin((char)(bits >> 8)) + (divide ? "_" : "") + bin((char)(bits & 0xff));
    }
    std::string bin(int bits, bool divide = false) {
        return bin((short)(bits >> 16), divide) + (divide ? "_" : "") + bin((short)(bits & 0xffff), divide);
    }
    std::string bin(uint32_t bits, bool divide = false) {
        return bin((short)(bits >> 16), divide) + (divide ? "_" : "") + bin((short)(bits & 0xffff), divide);
    }
    std::string hex(char bits) {
        std::string result = ""; char numerals[] = "0123456789ABCDEF";
        return result + numerals[((bits >> 4) & 0xf)] + numerals[bits & 0xf];
    }
    std::string hex(short bits) {
        return hex((char)(bits >> 8)) + hex((char)(bits & 0xff));
    }
    std::string hex(int bits) {
        return hex((short)(bits >> 16)) + hex((short)(bits & 0xffff));
    }
    std::string hex(uint32_t bits) {
        return hex((short)(bits >> 16)) + hex((short)(bits & 0xffff));
    }
    WiredCPU *cpu  = nullptr;
#ifndef NO_SDL
    TTF_Font *font = nullptr;
#endif
};

// https://elixir.bootlin.com/qemu/v7.0.0/source/hw/rtc/mc146818rtc.c
class CMOS {
    uint8_t bytes[128]{0};
    int     index = 0;

  public:
    CMOS() {
        bytes[10]   = 0x26; // RTC status register A: 32768 Hz time base, 976562 msec INT rate
        bytes[11]   = 0x02; // RTC status register B: 24 hours
        bytes[12]   = 0x00; // RTC status register C
        bytes[13]   = 0x80; // RTC status register D: battery power good
        bytes[0x14] = 0x02; // IBM equipment byte: math coprocessor installed
    }

    void ioport_write(int port, int data) {
        if (port == 0x70) {
            index = data & 0x7f;
        }
    }

    int ioport_read(int port) {
        int data;
        time_t clock;
        struct tm *utc;

        if (port == 0x70) {
            return 0xff;
        }

        time(&clock);
        utc = gmtime(&clock);

        bytes[0] = bin_to_bcd(utc->tm_sec);
        bytes[2] = bin_to_bcd(utc->tm_min);
        bytes[4] = bin_to_bcd(utc->tm_hour);
        bytes[6] = bin_to_bcd(utc->tm_wday);
        bytes[7] = bin_to_bcd(utc->tm_mday);
        bytes[8] = bin_to_bcd(utc->tm_mon + 1);
        bytes[9] = bin_to_bcd(utc->tm_year);

        data = bytes[index];
        if (index == 10) {
            bytes[10] ^= 0x80; // XOR emulates data update cycle
        }
        return data;
    }

  private:
    uint8_t bin_to_bcd(int a) {
        return static_cast<uint8_t>(((a / 10) << 4) | (a % 10));
    }
};
class PIC;
// https://elixir.bootlin.com/qemu/v7.0.0/source/hw/intc/i8259.c
// https://pdos.csail.mit.edu/6.828/2017/readings/hardware/8259A.pdf
class I8259 {
    int last_irr = 0;
    int irr = 0; // interrupt request register
    int imr = 0; // interrupt mask register
    int isr = 0; // in-service register
    int priority_add = 0;
    int read_reg_select = 0;
    int special_mask = 0;
    int icwn = 0; // ICW sequence 1..4
    int auto_eoi = 0;
    int rotate_on_autoeoi = 0;
    int icw4 = 0;
    int elcr = 0; // Edge/Level Control Register
    bool rotate_on_auto_eoi = false;
    PIC *pic = nullptr;

  public:
    int elcr_mask = 0;
    int irq_base = 0;
    I8259(PIC *pic) {
        this->pic = pic;
        reset();
    }
    ~I8259() {
    }
    void reset() {
        last_irr = 0;
        irr = 0;
        imr = 0;
        isr = 0;
        priority_add = 0;
        read_reg_select = 0;
        special_mask = 0;
        icwn = 0;
        auto_eoi = 0;
        rotate_on_autoeoi = 0;
        icw4 = 0;
        elcr = 0;
        elcr_mask = 0;
        irq_base = 0;
    }
    void set_irq(int irq, int val) {
        int ir_register = 1 << irq;
        if (val) {
            if ((last_irr & ir_register) == 0) {
                irr |= ir_register;
            }
            last_irr |= ir_register;
        } else {
            last_irr &= ~ir_register;
        }
    }
    int get_irq() {
        int ir_register, in_service_priority, priority;
        ir_register = irr & ~imr;
        priority = get_priority(ir_register);
        if (priority < 0) {
            return -1;
        }
        in_service_priority = get_priority(isr);
        if (priority > in_service_priority) {
            return priority;
        } else {
            return -1;
        }
    }
    void update_irq();
    int get_priority(int ir_register) {
        int priority = 7;
        if (ir_register == 0) {
            return -1;
        }
        while ((ir_register & (1 << ((priority + priority_add) & 7))) == 0) {
            priority--;
        }
        return priority;
    }
    void intack(int irq) {
        if (auto_eoi) {
            if (rotate_on_auto_eoi) {
                priority_add = (irq + 1) & 7;
            }
        } else {
            isr |= (1 << irq);
        }
        if (!(elcr & (1 << irq))) {
            irr &= ~(1 << irq);
        }
    }
    void ioport_write(int port, int data) {
        int priority;
        if ((port & 1) == 0) { // (A0 == 0) and...
            if (data & 0x10) {  // ...(bit 4 == 1) is ICW1 (after reset)
                reset();
                icwn = 1; // start ICW sequence
                icw4 = data & 1;   // ICW1.IC4
                if (data & 0x02) { // ICW1.SNGL
                    throw "ICW1: SNGL == 1 not supported";
                }
                if (data & 0x08) { // ICW1.LTIM
                    throw "ICW1: LTIM == 1 not supported";
                }
            } else if (data & 0x08) { // ...(bit 3 == 1) are OCW3
                if (data & 0x02) { // OCW3.RR (read register command)
                    read_reg_select = data & 1;
                }
                if (data & 0x40) { // OCW3.ESMM (special mask mode)
                    special_mask = (data >> 5) & 1;
                }
            } else { // ...( bit 4 == 0 && bit 3 == 0) are OCW2
                switch (data) {
                case 0x00:
                case 0x80: // rotate in automatic EOI mode (clear)
                    rotate_on_autoeoi = data >> 7;
                    break;
                case 0x20: // non-specific EOI command
                case 0xa0: // rotate on non-specific EOI command
                    priority = get_priority(isr);
                    if (priority >= 0) {
                        isr &= ~(1 << ((priority + priority_add) & 7));
                    }
                    if (data == 0xa0) {
                        priority_add = (priority_add + 1) & 7;
                    }
                    break;
                case 0x60: // specific EOI command, IR priority level 0
                case 0x61: // level 1
                case 0x62: // level 2
                case 0x63: // level 3
                case 0x64: // level 4
                case 0x65: // level 5
                case 0x66: // level 6
                case 0x67: // level 7
                    priority = data & 7;
                    isr &= ~(1 << priority);
                    break;
                case 0xc0: // set priority command, IR priority level 0
                case 0xc1: // level 1
                case 0xc2: // level 2
                case 0xc3: // level 3
                case 0xc4: // level 4
                case 0xc5: // level 5
                case 0xc6: // level 6
                case 0xc7: // level 7
                    priority_add = (data + 1) & 7;
                    break;
                case 0xe0: // rotate on specific EOI command, IR level 0
                case 0xe1: // level 1
                case 0xe2: // level 2
                case 0xe3: // level 3
                case 0xe4: // level 4
                case 0xe5: // level 5
                case 0xe6: // level 6
                case 0xe7: // level 7
                    priority = data & 7;
                    isr &= ~(1 << priority);
                    priority_add = (priority + 1) & 7;
                    break;
                }
            }
        } else { // ICW2, 3, 4, or OCW1
            switch (icwn) {
            case 0: // OCW1, set IMR
                imr = data;
                update_irq();
                break;
            case 1: // ICW2, set page starting address of service routines
                irq_base = data & 0xf8;
                icwn = 2;
                break;
            case 2: // ICW3, load slave register if ICW1.SNGL == 0
                if (icw4) {
                    icwn = 3;
                } else {
                    icwn = 0;
                }
                break;
            case 3: // ICW4, program SFNM, BUF, M/S, AEOI and uPM if set
                auto_eoi = (data >> 1) & 1;
                icwn = 0;
                break;
            }
        }
    }
    int ioport_read(int port) {
        int reg;
        if ((port & 1) == 0) {
            if (read_reg_select) {
                reg = isr;
            } else {
                reg = irr;
            }
        } else {
            reg = imr;
        }
        return reg;
    }
};
// the programmable interrupt controller (two cascaded 8259)
class PIC {
  public:
    int irq = 0;
    I8259 *pics[2];
    PIC() {
        pics[0] = new I8259(this);
        pics[1] = new I8259(this);
        pics[0]->elcr_mask = 0xf8;
        pics[1]->elcr_mask = 0xde;
    }
    ~PIC() {
        delete pics[0];
        delete pics[1];
    }
    void set_irq(int irq, int val) {
        pics[irq >> 3]->set_irq(irq & 7, val);
        update_irq();
    }
    void update_irq();
    int read_irq() {
        int intno = 0;
        int irq = pics[0]->get_irq();
        if (irq >= 0) {
            pics[0]->intack(irq);
            if (irq == 2) {
                int slave_irq = pics[1]->get_irq();
                if (slave_irq >= 0) {
                    pics[1]->intack(slave_irq);
                } else {
                    slave_irq = 7;
                }
                intno = pics[1]->irq_base + slave_irq;
                irq = slave_irq + 8;
            } else {
                intno = pics[0]->irq_base + irq;
            }
        } else {
            irq = 7;
            intno = pics[0]->irq_base + irq;
        }
        update_irq();
        return intno;
    }
};
inline void I8259::update_irq() {
    pic->update_irq();
}
inline void PIC::update_irq() {
    int slave_irq = pics[1]->get_irq();
    int irq = pics[0]->get_irq();
    if (slave_irq >= 0) {
        pics[0]->set_irq(2, 1);
        pics[0]->set_irq(2, 0);
    }
    if (irq >= 0) {
        this->irq = 1;
    } else {
        this->irq = 0;
    }
}
// https://elixir.bootlin.com/qemu/v7.0.0/source/hw/char/serial.c
// https://wiki.osdev.org/Serial_Ports
class Serial {
    int divider = 0;
    int rbr = 0;    // receive buffer
    int ier = 0;    // interrupt enable register
    int iir = 0x01; // interrupt identification register
    int lcr = 0;    // line control register
    int mcr;        // modem control register
    int lsr = 0x40 | 0x20; // line status register
    int msr = 0;    // modem status register
    int scr = 0;    // scratch register
    RingBuffer<int> input_fifo{RingBuffer<int>(1000)};
    RingBuffer<int> print_fifo{RingBuffer<int>(1000)};
    PIC *pic;

  public:
    Serial(PIC *pic) {
        this->pic = pic;
    }
    void set_irq(int val);
    void update_irq() {
        if ((lsr & 0x01) && (ier & 0x01)) {
            iir = 0x04;
        } else if ((lsr & 0x20) && (ier & 0x02)) {
            iir = 0x02;
        } else {
            iir = 0x01;
        }
        if (iir != 0x01) {
            set_irq(1);
        } else {
            set_irq(0);
        }
    }
    void ioport_write(int port, int data) {
        switch (port & 7) {
        default:
        case 0:
            if (lcr & 0x80) {
                divider = (divider & 0xff00) | data;
            } else {
                lsr &= ~0x20;
                update_irq();
                print_fifo_push(data);
                lsr |= 0x20;
                lsr |= 0x40;
                update_irq();
            }
            break;
        case 1:
            if (lcr & 0x80) {
                divider = (divider & 0x00ff) | (data << 8);
            } else {
                ier = data;
                update_irq();
            }
            break;
        case 2:
            break;
        case 3:
            lcr = data;
            break;
        case 4:
            mcr = data;
            break;
        case 5:
            break;
        case 6:
            msr = data;
            break;
        case 7:
            scr = data;
            break;
        }
    }
    int ioport_read(int port) {
        int reg;
        switch (port & 7) {
        default:
        case 0:
            if (lcr & 0x80) {
                reg = divider & 0xff;
            } else {
                reg = rbr;
                lsr &= ~(0x01 | 0x10);
                update_irq();
                input_fifo_pop();
            }
            break;
        case 1:
            if (lcr & 0x80) {
                reg = (divider >> 8) & 0xff;
            } else {
                reg = ier;
            }
            break;
        case 2:
            reg = iir;
            break;
        case 3:
            reg = lcr;
            break;
        case 4:
            reg = mcr;
            break;
        case 5:
            reg = lsr;
            break;
        case 6:
            reg = msr;
            break;
        case 7:
            reg = scr;
            break;
        }
        return reg;
    }
    void recv_char(int chr) {
        rbr = chr;
        lsr |= 0x01;
        update_irq();
    }
    void input_fifo_pop() {
        if (!input_fifo.isempty() && !(lsr & 0x01)) {
            recv_char(input_fifo.pop());
        }
    }
    void input_fifo_push(int chr) {
        input_fifo.push(chr);
        recv_char(chr);
    }
    void print_fifo_push(int chr) {
        print_fifo.push(chr);
    }
    char print_fifo_pop() {
        return static_cast<char>(print_fifo.pop());
    }
};
inline void Serial::set_irq(int val) {
    pic->set_irq(4, val);
}
// https://elixir.bootlin.com/qemu/v7.0.0/source/hw/timer/i8254.c
// https://elixir.bootlin.com/qemu/v7.0.0/source/hw/timer/i8254_common.c
class PITChannel {
    int last_irr = 0;
    int count = 0;
    int count_load_time = 0;
    float pit_time_unit = 0.596591f;
    Free86 *cpu;

  public:
    int latched_count = 0;
    int rw_state = 0;
    int mode = 0;
    int bcd = 0;
    int gate = 0;
    PITChannel(Free86 *cpu) {
        this->cpu = cpu;
    }
    int get_time() {
        return static_cast<int>(floor(cpu->cycles * pit_time_unit));
    }
    void pit_load_count(int data) {
        if (data == 0) {
            count = 0x10000;
        } else {
            count = data;
        }
        count_load_time = get_time();
    }
    int pit_get_count() {
        int d, dh;
        d = get_time() - count_load_time;
        switch (mode) {
        case 0:
        case 1:
        case 4:
        case 5:
            dh = (count - d) & 0xffff;
            break;
        default:
            dh = count - (d % count);
            break;
        }
        return dh;
    }
    int pit_get_out() {
        int d, eh;
        d = get_time() - count_load_time;
        switch (mode) {
        default:
        case 0: // Interrupt on terminal count
            eh = (d >= count) >> 0;
            break;
        case 1: // One shot
            eh = (d < count) >> 0;
            break;
        case 2: // Frequency divider
            if ((d % count) == 0 && d != 0) {
                eh = 1;
            } else {
                eh = 0;
            }
            break;
        case 3: // Square wave
            eh = ((d % count) < (count >> 1)) >> 0;
            break;
        case 4: // SW strobe
        case 5: // HW strobe
            eh = (d == count) >> 0;
            break;
        }
        return eh;
    }
};
class PIT {
    PITChannel *pit_channels[3];
    int speaker_data_on = 0;
    PIC *pic;

  public:
    PIT(Free86 *cpu, PIC *pic) {
        this->pic = pic;
        for (int i = 0; i < 3; i++) {
            pit_channels[i] = new PITChannel(cpu);
            pit_channels[i]->mode = 3;
            pit_channels[i]->gate = (i != 2) >> 0;
            pit_channels[i]->pit_load_count(0);
        }
    }
    void set_irq(int val);
    void update_irq();
    void ioport_write(int port, int data) {
        int hh, ih;
        if ((port & 3) == 3) {
            hh = data >> 6;
            if (hh == 3) {
                return;
            }
            auto s = pit_channels[hh];
            ih = (data >> 4) & 3;
            switch (ih) {
            case 0:
                s->latched_count = s->pit_get_count();
                s->rw_state = 4;
                break;
            default:
                s->mode = (data >> 1) & 7;
                s->bcd = data & 1;
                s->rw_state = ih - 1 + 0;
                break;
            }
        } else {
            auto s = pit_channels[port & 3];
            switch (s->rw_state) {
            case 0:
                s->pit_load_count(data);
                break;
            case 1:
                s->pit_load_count(data << 8);
                break;
            case 2:
            case 3:
                if (s->rw_state & 1) {
                    s->pit_load_count((s->latched_count & 0xff) | (data << 8));
                } else {
                    s->latched_count = data;
                }
                s->rw_state ^= 1;
                break;
            }
        }
    }
    int ioport_read(int port) {
        int res, ma;
        auto s = pit_channels[port & 3];
        switch (s->rw_state) {
        case 0:
        case 1:
        case 2:
        case 3:
            ma = s->pit_get_count();
            if (s->rw_state & 1) {
                res = (ma >> 8) & 0xff;
            } else {
                res = ma & 0xff;
            }
            if (s->rw_state & 2) {
                s->rw_state ^= 1;
            }
            break;
        default:
        case 4:
        case 5:
            if (s->rw_state & 1) {
                res = s->latched_count >> 8;
            } else {
                res = s->latched_count & 0xff;
            }
            s->rw_state ^= 1;
            break;
        }
        return res;
    }
    void speaker_ioport_write(int port, int data) {
        speaker_data_on = (data >> 1) & 1;
        pit_channels[2]->gate = data & 1;
    }
    int speaker_ioport_read(int port) {
        int eh, data;
        auto s = pit_channels[2];
        eh = s->pit_get_out();
        data = (speaker_data_on << 1) | s->gate | (eh << 5);
        return data;
    }
};
inline void PIT::set_irq(int val) {
    pic->set_irq(0, val);
}
inline void PIT::update_irq() {
    set_irq(1);
    set_irq(0);
}
class WiredCPU : public Free86 {
    CMOS *cmos = nullptr;

  public:
    PIC *pic = nullptr;
    PIT *pit = nullptr;
    Serial *serial = nullptr;
    WiredCPU(uint32_t memory_size) : Free86(memory_size) {
        cmos = new CMOS();
        pic = new PIC();
        pit = new PIT(this, pic);
        serial = new Serial(pic);
    }
    ~WiredCPU() override {
        delete cmos;
        delete pic;
        delete pit;
        delete serial;
    }
    int get_irq() override;
    int get_iid() override;
    uint32_t io_read(uint32_t port) override;
    void io_write(uint32_t port, uint32_t data) override;
};

#endif // PC_H
