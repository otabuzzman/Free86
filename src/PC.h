#ifndef _H_PC
#define _H_PC

#include <cstddef>
#include <stdexcept>
#include <vector>

#ifndef NO_SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#endif

#include "CMOS.h"
#include "KBD.h"
#include "ringbuffer.h"
#include "x86/x86.h"

class WiredCPU;

class PC {
  public:
    PC(int mem_size);
    ~PC();

    int load(std::string path, int offset = 0);
    void setup();
    void cycle();

#ifndef NO_SDL
    void paint(SDL_Renderer *render, int widht, int height);
#else
    void print();
    void input();
#endif

  private:
    WiredCPU *cpu  = nullptr;
#ifndef NO_SDL
    TTF_Font    *font = nullptr;
#endif
};

class PIC;

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
    I8259(PIC *_pic) {
        pic = _pic;
        reset();
    }
    ~I8259() {
    }
    void init() {
    }
    void reset() {
        last_irr = 0;
        irr = 0;
        imr = 0;
        isr = 0;
        priority_add = 0;
        irq_base = 0;
        read_reg_select = 0;
        special_mask = 0;
        icwn = 0;
        auto_eoi = 0;
        rotate_on_autoeoi = 0;
        icw4 = 0;
        elcr = 0;
        elcr_mask = 0;
    }
    void set_irq(int irq, bool Qf) {
        int ir_register = 1 << irq;
        if (Qf) {
            if ((last_irr & ir_register) == 0) {
                irr |= ir_register;
            }
            last_irr |= ir_register;
        } else {
            last_irr &= ~ir_register;
        }
    }
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
    void ioport_write(int mem8_loc, int x) {
        mem8_loc &= 1; // pin A0
        int priority;
        if (mem8_loc == 0) { // (A0 == 0) and...
            if (x & 0x10) {  // ...(bit 4 == 1) is ICW1 (after reset)
                reset();
                icwn = 1; // start ICW sequence
                icw4 = x & 1;   // ICW1.IC4
                if (x & 0x02) { // ICW1.SNGL
                    throw "ICW1: SNGL == 1 not supported";
                }
                if (x & 0x08) { // ICW1.LTIM
                    throw "ICW1: LTIM == 1 not supported";
                }
            } else if (x & 0x08) { // ...(bit 3 == 1) are OCW3
                if (x & 0x02) { // OCW3.RR (read register command)
                    read_reg_select = x & 1;
                }
                if (x & 0x40) { // OCW3.ESMM (special mask mode)
                    special_mask = (x >> 5) & 1;
                }
            } else { // ...( bit 4 == 0 && bit 3 == 0) are OCW2
                switch (x) {
                case 0x00:
                case 0x80: // rotate in automatic EOI mode (clear)
                    rotate_on_autoeoi = x >> 7;
                    break;
                case 0x20: // non-specific EOI command
                case 0xa0: // rotate on non-specific EOI command
                    priority = get_priority(isr);
                    if (priority >= 0) {
                        isr &= ~(1 << ((priority + priority_add) & 7));
                    }
                    if (x == 0xa0) {
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
                    priority = x & 7;
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
                    priority_add = (x + 1) & 7;
                    break;
                case 0xe0: // rotate on specific EOI command, IR level 0
                case 0xe1: // level 1
                case 0xe2: // level 2
                case 0xe3: // level 3
                case 0xe4: // level 4
                case 0xe5: // level 5
                case 0xe6: // level 6
                case 0xe7: // level 7
                    priority = x & 7;
                    isr &= ~(1 << priority);
                    priority_add = (priority + 1) & 7;
                    break;
                }
            }
        } else { // ICW2, 3, 4, or OCW1
            switch (icwn) {
            case 0: // OCW1, set IMR
                imr = x;
                update_irq();
                break;
            case 1: // ICW2, set page starting address of service routines
                irq_base = x & 0xf8;
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
                auto_eoi = (x >> 1) & 1;
                icwn = 0;
                break;
            }
        }
    }
    int ioport_read(int Ug) {
        int mem8_loc, return_register;
        mem8_loc = Ug & 1;
        if (mem8_loc == 0) {
            if (read_reg_select) {
                return_register = isr;
            } else {
                return_register = irr;
            }
        } else {
            return_register = imr;
        }
        return return_register;
    }
    void update_irq();
};
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
    void set_irq(int irq, int Qf) {
        pics[irq >> 3]->set_irq(irq & 7, Qf);
        update_irq();
    }
    int get_hard_intno() {
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
    void update_irq();
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
    int set_irq_func = 0;
    int write_func = 0;
    PIC *pic;
    RingBuffer<int> input_fifo{RingBuffer<int>(1000)};
    RingBuffer<int> print_fifo{RingBuffer<int>(1000)};

  public:
    Serial(PIC *_pic, int kh, int lh) {
        pic = _pic;
        set_irq_func = kh;
        write_func = lh;
    }
    void store_char(int x) {
        print_fifo.push(x);
    }
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
    void ioport_write(int mem8_loc, int x) {
        mem8_loc &= 7;
        switch (mem8_loc) {
        default:
        case 0:
            if (lcr & 0x80) {
                divider = (divider & 0xff00) | x;
            } else {
                lsr &= ~0x20;
                update_irq();
                store_char(x);
                lsr |= 0x20;
                lsr |= 0x40;
                update_irq();
            }
            break;
        case 1:
            if (lcr & 0x80) {
                divider = (divider & 0x00ff) | (x << 8);
            } else {
                ier = x;
                update_irq();
            }
            break;
        case 2:
            break;
        case 3:
            lcr = x;
            break;
        case 4:
            mcr = x;
            break;
        case 5:
            break;
        case 6:
            msr = x;
            break;
        case 7:
            scr = x;
            break;
        }
    }
    int ioport_read(int mem8_loc) {
        mem8_loc &= 7;
        int Pg;
        switch (mem8_loc) {
        default:
        case 0:
            if (lcr & 0x80) {
                Pg = divider & 0xff;
            } else {
                Pg = rbr;
                lsr &= ~(0x01 | 0x10);
                update_irq();
                send_char_from_fifo();
            }
            break;
        case 1:
            if (lcr & 0x80) {
                Pg = (divider >> 8) & 0xff;
            } else {
                Pg = ier;
            }
            break;
        case 2:
            Pg = iir;
            break;
        case 3:
            Pg = lcr;
            break;
        case 4:
            Pg = mcr;
            break;
        case 5:
            Pg = lsr;
            break;
        case 6:
            Pg = msr;
            break;
        case 7:
            Pg = scr;
            break;
        }
        return Pg;
    }
    void send_break() {
        rbr = 0;
        lsr |= 0x10 | 0x01;
        update_irq();
    }
    void send_char(int mh) {
        rbr = mh;
        lsr |= 0x01;
        update_irq();
    }
    void send_char_from_fifo() {
        if (!input_fifo.isempty() && !(lsr & 0x01)) {
            send_char(input_fifo.pop());
        }
    }
    void input_fifo_push(int na) {
        input_fifo.push(na);
        send_char_from_fifo();
    }
    char print_fifo_pop() {
        return print_fifo.pop();
    }
    void set_irq(int x);
};
inline void Serial::set_irq(int x) {
    pic->set_irq(4, x);
}
class IRQCH {
    int last_irr = 0;
    int count = 0;
    int count_load_time = 0;
    float pit_time_unit = 0.596591F;
    x86 *cpu;

  public:
    int latched_count = 0;
    int rw_state = 0;
    int mode = 0;
    int bcd = 0;
    int gate = 0;
    IRQCH(x86 *_cpu) {
        cpu = _cpu;
    }
    int get_time() {
        return static_cast<int>(std::floor(cpu->cycles * pit_time_unit));
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
    int get_next_transition_time() {
        int d, fh, base, gh;
        d = get_time() - count_load_time;
        switch (mode) {
        default:
        case 0: // Interrupt on terminal count
        case 1: // One shot
            if (d < count) {
                fh = count;
            } else {
                return -1;
            }
            break;
        case 2: // Frequency divider
            base = (d / count) * count;
            if ((d - base) == 0 && d != 0) {
                fh = base + count;
            } else {
                fh = base + count + 1;
            }
            break;
        case 3: // Square wave
            base = (d / count) * count;
            gh = ((count + 1) >> 1);
            if ((d - base) < gh) {
                fh = base + gh;
            } else {
                fh = base + count;
            }
            break;
        case 4: // SW strobe
        case 5: // HW strobe
            if (d < count) {
                fh = count;
            } else if (d == count) {
                fh = count + 1;
            } else {
                return -1;
            }
            break;
        }
        fh = count_load_time + fh;
        return fh;
    }
    void pit_load_count(int x) {
        if (x == 0) {
            x = 0x10000;
        }
        count_load_time = get_time();
        count = x;
    }
};
class PIT {
    IRQCH *pit_channels[3];
    x86 *cpu;
    PIC *pic;
    int speaker_data_on = 0;

  public:
    PIT(x86 *_cpu, PIC *_pic) {
        cpu = _cpu;
        pic = _pic;
        for (int i = 0; i < 3; i++) {
            pit_channels[i] = new IRQCH(cpu);
            pit_channels[i]->mode = 3;
            pit_channels[i]->gate = (i != 2) >> 0;
            pit_channels[i]->pit_load_count(0);
        }
    }
    void ioport_write(int mem8_loc, int x) {
        mem8_loc &= 3;
        int hh, ih;
        if (mem8_loc == 3) {
            hh = x >> 6;
            if (hh == 3) {
                return;
            }
            auto s = pit_channels[hh];
            ih = (x >> 4) & 3;
            switch (ih) {
            case 0:
                s->latched_count = s->pit_get_count();
                s->rw_state = 4;
                break;
            default:
                s->mode = (x >> 1) & 7;
                s->bcd = x & 1;
                s->rw_state = ih - 1 + 0;
                break;
            }
        } else {
            auto s = pit_channels[mem8_loc];
            switch (s->rw_state) {
            case 0:
                s->pit_load_count(x);
                break;
            case 1:
                s->pit_load_count(x << 8);
                break;
            case 2:
            case 3:
                if (s->rw_state & 1) {
                    s->pit_load_count((s->latched_count & 0xff) | (x << 8));
                } else {
                    s->latched_count = x;
                }
                s->rw_state ^= 1;
                break;
            }
        }
    }
    int ioport_read(int mem8_loc) {
        mem8_loc &= 3;
        int Pg, ma;
        auto s = pit_channels[mem8_loc];
        switch (s->rw_state) {
        case 0:
        case 1:
        case 2:
        case 3:
            ma = s->pit_get_count();
            if (s->rw_state & 1) {
                Pg = (ma >> 8) & 0xff;
            } else {
                Pg = ma & 0xff;
            }
            if (s->rw_state & 2) {
                s->rw_state ^= 1;
            }
            break;
        default:
        case 4:
        case 5:
            if (s->rw_state & 1) {
                Pg = s->latched_count >> 8;
            } else {
                Pg = s->latched_count & 0xff;
            }
            s->rw_state ^= 1;
            break;
        }
        return Pg;
    }
    void speaker_ioport_write(int mem8_loc, int x) {
        speaker_data_on = (x >> 1) & 1;
        pit_channels[2]->gate = x & 1;
    }
    int speaker_ioport_read(int mem8_loc) {
        int eh, x;
        auto s = pit_channels[2];
        eh = s->pit_get_out();
        x = (speaker_data_on << 1) | s->gate | (eh << 5);
        return x;
    }
    void speaker_ioport_write() {
        set_irq(1);
        set_irq(0);
    }
    void set_irq(int x);
    void update_irq();
};
inline void PIT::set_irq(int x) {
    pic->set_irq(0, x);
}
inline void PIT::update_irq() {
    set_irq(1);
    set_irq(0);
}
class WiredCPU : public x86 {
    CMOS *cmos = nullptr;
    KBD *kbd = nullptr;

  public:
    PIC *pic = nullptr;
    PIT *pit = nullptr;
    Serial *serial = nullptr;
    WiredCPU(int mem_size) : x86(mem_size) {
        cmos = new CMOS();
        kbd = new KBD();
        pic = new PIC();
        serial = new Serial(pic, 0, 0);
        pit = new PIT(this, pic);
    }
    ~WiredCPU() override {
        delete cmos;
        delete kbd;
        delete pic;
        delete pit;
        delete serial;
    }
    int get_hard_irq() override;
    int get_hard_intno() override;
    int ioport_read(int port_num) override;
    void ioport_write(int port_num, int data) override;
};

#endif // _H_PC
