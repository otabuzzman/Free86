import Foundation
import Free86

/// https://elixir.bootlin.com/qemu/v7.0.0/source/hw/rtc/mc146818rtc.c
class CMOS {
    var bytes: [Byte] = .init(repeating: .zero, count: 128)
    var index = 0
    init() {
        bytes[10]   = 0x26  // RTC status register A: 32768 Hz time base, 976562 msec INT rate
        bytes[11]   = 0x02  // RTC status register B: 24 hours
        bytes[12]   = 0x00  // RTC status register C
        bytes[13]   = 0x80  // RTC status register D: battery power good
        bytes[0x14] = 0x02  // IBM equipment byte: math coprocessor installed
    }
}
/// https://elixir.bootlin.com/qemu/v7.0.0/source/hw/intc/i8259.c
/// https://pdos.csail.mit.edu/6.828/2017/readings/hardware/8259A.pdf
class I8259 {
    var last_irr = 0
    var irr = 0  // interrupt request register
    var imr = 0  // interrupt mask register
    var isr = 0  // in-service register
    var priority_add = 0
    var read_reg_select = 0
    var special_mask = 0
    var icwn = 0  // ICW sequence 1..4
    var auto_eoi = 0
    var rotate_on_autoeoi = 0
    var icw4 = 0
    var elcr = 0  // edge/level Control Register
    var rotate_on_auto_eoi = false
    let pic: PIC
    var elcr_mask = 0
    var irq_base = 0
    init(_ pic: PIC) {
        self.pic = pic
        reset()
    }
    func reset() {
        last_irr = 0
        irr = 0
        imr = 0
        isr = 0
        priority_add = 0
        read_reg_select = 0
        special_mask = 0
        icwn = 0
        auto_eoi = 0
        rotate_on_autoeoi = 0
        icw4 = 0
        elcr = 0
        elcr_mask = 0
        irq_base = 0
    }
    func set_irq(_ irq: Int, _ val: Bool) {
        let ir_register = 1 << irq
        if val {
            if (last_irr & ir_register) == 0 {
                irr |= ir_register
            }
            last_irr |= ir_register
        } else {
            last_irr &= ~ir_register
        }

    }
    func update_irq() {
        pic.update_irq()
    }
    func get_irq() -> Int {
        let ir_register = irr & ~imr
        let priority = get_priority(ir_register)
        if priority < 0 {
            return -1
        }
        let in_service_priority = get_priority(isr)
        if priority > in_service_priority {
            return priority
        } else {
            return -1
        }
    }
    func get_priority(_ ir_register: Int) -> Int {
        var priority = 7
        if ir_register == 0 {
            return -1
        }
        while (ir_register & (1 << ((priority + priority_add) & 7))) == 0 {
            priority -= 1
        }
        return priority
    }
    func intack(_ irq: Int) {
        if auto_eoi != 0 {
            if rotate_on_auto_eoi {
                priority_add = (irq + 1) & 7
            }
        } else {
            isr |= 1 << irq
        }
        if (elcr & (1 << irq)) == 0 {
            irr &= ~(1 << irq)
        }
    }
}
/// the programmable interrupt controller (two cascaded 8059)
class PIC {
    private(set) var irq = 0
    lazy var pics = [I8259(self), I8259(self)]
    init() {
        pics[0].elcr_mask = 0xf8
        pics[1].elcr_mask = 0xde
    }
    func set_irq(_ irq: Int, _ val: Bool) {
        pics[irq >> 3].set_irq(irq & 7, val)
        update_irq()
    }
    func update_irq() {
        let slave_irq = pics[1].get_irq()
        let irq = pics[0].get_irq()
         if slave_irq >= 0 {
             pics[0].set_irq(2, true)
             pics[0].set_irq(2, false)
         }
         if irq >= 0 {
             self.irq = 1
         } else {
             self.irq = 0
         }
    }
    var iid: Int {
        var iid = 0
        var irq = pics[0].get_irq()
        if irq >= 0 {
            pics[0].intack(irq)
            if irq == 2 {
                var slave_irq = pics[1].get_irq()
                if slave_irq >= 0 {
                    pics[1].intack(slave_irq)
                } else {
                    slave_irq = 7
                }
                iid = pics[1].irq_base + slave_irq
            } else {
                iid = pics[0].irq_base + irq
            }
        } else {
            irq = 7
            iid = pics[0].irq_base + irq
        }
        update_irq()
        return iid
    }
}
/// https://elixir.bootlin.com/qemu/v7.0.0/source/hw/timer/i8254.c
/// https://elixir.bootlin.com/qemu/v7.0.0/source/hw/timer/i8254_common.c
class PITChannel {
    var last_irr = 0
    var count = 0
    var count_load_time = 0
    var pit_time_unit: Float = 0.596591
    var cpu: Free86
    var latched_count = 0
    var rw_state = 0
    var mode = 0
    var bcd = 0
    var gate = 0
    init(_ cpu: Free86) {
        self.cpu = cpu
    }
    func get_time() -> Int {
        Int((Float(cpu.cycles) * pit_time_unit))
    }
    func pit_load_count(_ data: Int) {
        if data == 0 {
            count = 0x10000
        } else {
            count = data
        }
        count_load_time = get_time()
    }
    func pit_get_count() -> Int {
        var dh = 0
        let d = get_time() - count_load_time
        switch mode {
        case 0, 1, 4, 5:
            dh = (count - d) & 0xffff
            break
        default:
            dh = count - (d % count)
            break
        }
        return dh
    }
    func pit_get_out() -> Int {
        var eh = false
        let d = get_time() - count_load_time
        switch mode {
        case 0:  // interrupt on terminal count
            eh = d >= count
            break
        case 1:  // one shot
            eh = d < count
            break
        case 2:  // frequency divider
            if (d % count) == 0 && d != 0 {
                eh = true
            } else {
                eh = false
            }
            break
        case 3:  // square wave
            eh = (d % count) < (count >> 1)
            break
        case 4,  // SW strobe
             5:  // HW strobe
            eh = d == count
            break
        default:
            break
        }
        return eh ? 1 : 0
    }
}
class PIT {
    let pit_channels: [PITChannel]
    var speaker_data_on = 0
    let pic: PIC
    init(_ cpu: Free86, _ pic: PIC) {
        self.pic = pic
        self.pit_channels = [PITChannel(cpu), PITChannel(cpu), PITChannel(cpu)]
        for slot in 0..<3 {
            pit_channels[slot].mode = 3
            pit_channels[slot].gate = (slot != 2) ? 1 : 0
            pit_channels[slot].pit_load_count(0)
        }
    }
    func set_irq(_ val: Bool) {
        pic.set_irq(0, val)
    }
    func update_irq() {
        set_irq(true)
        set_irq(false)
    }
}
/// https://elixir.bootlin.com/qemu/v7.0.0/source/hw/char/serial.c
/// https://wiki.osdev.org/Serial_Ports
class Serial {
    var divider = 0
    var rbr = 0  // receive buffer
    var ier = 0  // interrupt enable register
    var iir = 0x01  // interrupt identification register
    var lcr = 0  // line control register
    var mcr = 0  // modem control register
    var lsr = 0x40 | 0x20  // line status register
    var msr = 0  // modem status register
    var scr = 0  // scratch register
    let pic: PIC
    init(_ pic: PIC) {
        self.pic = pic
    }
    func set_irq(_ val: Bool) {
        pic.set_irq(4, val)
    }
    func update_irq() {
        if (lsr & 0x01) != 0 && (ier & 0x01) != 0 {
            iir = 0x04
        } else if (lsr & 0x20) != 0 && (ier & 0x02) != 0 {
            iir = 0x02
        } else {
            iir = 0x01
        }
        if iir != 0x01 {
            set_irq(true)
        } else {
            set_irq(false)
        }
    }
}
