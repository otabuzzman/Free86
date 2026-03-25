import Foundation
import Free86

let mem = MemoryIO<DWord>(capacity: 16 * 1024 * 1024)  // 16 MB

let bootloaderFileURL = URL(fileURLWithPath: "bin/linuxstart.bin")
let bootloaderAddress: DWord = 0x00010000
var data = try Data(contentsOf: bootloaderFileURL)
for (offset, byte) in data.enumerated() {
    mem[bootloaderAddress + DWord(offset)] = Byte(byte)
}

let linosKernelFileURL = URL(fileURLWithPath: "bin/vmlinux-2.6.20.bin")
let linosKernelAddress: DWord = 0x00100000
data = try Data(contentsOf: linosKernelFileURL)
for (offset, byte) in data.enumerated() {
    mem[linosKernelAddress + DWord(offset)] = Byte(byte)
}

let initRamDiskFileURL = URL(fileURLWithPath: "bin/root.bin")
let initRamDiskAddress: DWord = 0x00400000
data = try Data(contentsOf: initRamDiskFileURL)
for (offset, byte) in data.enumerated() {
    mem[initRamDiskAddress + DWord(offset)] = Byte(byte)
}

let cmdLine = "console=ttyS0 root=/dev/ram0 rw init=/sbin/init notsc=1"  // lpj=101131 no-hlt
let cmdLineAddress: DWord = 0x0000f800
for (offset, byte) in cmdLine.data(using: .utf8)!.enumerated() {
    mem[cmdLineAddress + DWord(offset)] = Byte(byte)
}

let cpuStateInitFileURL = URL(fileURLWithPath: "bin/bootstrap.bin")
let cpuStateInitAddress: DWord = 0x000f0000
data = try Data(contentsOf: cpuStateInitFileURL)
for (offset, byte) in data.enumerated() {
    mem[cpuStateInitAddress + DWord(offset)] = Byte(byte)
}

let cmos = CMOS()
let pic = PIC()
let pit = PIT(pic)
let serial = Serial(pic)
let port70 = Port70<Byte>(cmos)
let port71 = Port71<Byte>(cmos)
let port20 = PortA0<Byte>(pic, 0)
let port21 = PortA1<Byte>(pic, 0)
let portA0 = PortA0<Byte>(pic, 1)
let portA1 = PortA1<Byte>(pic, 1)
let port40 = Port42<Byte>(pit, 0)
let port41 = Port42<Byte>(pit, 1)
let port42 = Port42<Byte>(pit, 2)
let port43 = Port43<Byte>(pit)
let port61 = Port61<Byte>(pit)
let port3F8 = Port3F8<Byte>(serial)
let port3F9 = Port3F9<Byte>(serial)
let port3FA = Port3FA<Byte>(serial)
let port3FB = Port3FB<Byte>(serial)
let port3FC = Port3FC<Byte>(serial)
let port3FD = Port3FD<Byte>(serial)
let port3FE = Port3FE<Byte>(serial)
let port3FF = Port3FF<Byte>(serial)
let io = IsolatedIO<DWord>()
io.register(port: port70, at: 0x70)
io.register(port: port71, at: 0x71)
io.register(port: port20, at: 0x20)
io.register(port: port21, at: 0x21)
io.register(port: portA0, at: 0xA0)
io.register(port: portA1, at: 0xA1)
io.register(port: port40, at: 0x40)
io.register(port: port41, at: 0x41)
io.register(port: port42, at: 0x42)
io.register(port: port43, at: 0x43)
io.register(port: port61, at: 0x61)
io.register(port: port3F8, at: 0x3F8)
io.register(port: port3F9, at: 0x3F9)
io.register(port: port3FA, at: 0x3FA)
io.register(port: port3FB, at: 0x3FB)
io.register(port: port3FC, at: 0x3FC)
io.register(port: port3FD, at: 0x3FD)
io.register(port: port3FE, at: 0x3FE)
io.register(port: port3FF, at: 0x3FF)
let cpu = Free86(memory: mem, io: io)

while true {
    let cycles = cpu.cycles + 100000
    while cycles > cpu.cycles {
        pit.update_irq()
        if pic.irq > 0 && pic.read_irq() >= 0 {
            try await cpu.INTR.trigger(Byte(pic.read_irq()))
        }
        do {
            try await cpu.fetchDecodeExecute(cycles: cycles - cpu.cycles)
            if cpu.halted {
                break
            }
        } catch let interrupt as Interrupt {
            cpu.interrupt = interrupt
        } catch {
            print("\(error)")
            exit(EXIT_FAILURE)
        }
    }
}

extension MemoryIO<DWord> {
    convenience init(capacity: A) {
        assert(capacity % A(A.bankSize) == 0, "fatal error")
        self.init(defaultBank: DefaultBank<A>())
        for address in stride(from: 0, to: capacity, by: A.bankSize) {
            self.register(bank: RAMBank<A>(), at: address)
        }
    }
}

class Port70<T: FixedWidthInteger & UnsignedInteger>: IOPort {
    let circuit: CMOS
    init(_ circuit: CMOS) {
        self.circuit = circuit
    }
    func rd() -> T { 0xff }
    func wr(_ iodata: T) {
        circuit.index = Int(iodata & 0x7f)
    }
}
class Port71<T: FixedWidthInteger & UnsignedInteger>: IOPort {
    let circuit: CMOS
    init(_ circuit: CMOS) {
        self.circuit = circuit
    }
    func rd() -> T {
        let iodata = circuit.bytes[circuit.index]
        if circuit.index == 10 {
            circuit.bytes[10] ^= 0x80  // XOR emulates data update cycle
        }
        return T(iodata)
    }
    func wr(_ iodata: T) { }
}
class PortA0<T: FixedWidthInteger & UnsignedInteger>: IOPort {
    var priority = 0
    let circuit: PIC
    let chip: Int
    init(_ circuit: PIC, _ chip: Int) {
        self.circuit = circuit
        self.chip = chip
    }
    func rd() -> T { 0xff }
    func wr(_ iodata: T) {
        if (iodata & 0x10) != 0 {  // ...(bit 4 == 1) is ICW1 (after reset)
            circuit.pics[chip].reset()
            circuit.pics[chip].icwn = 1  // start ICW sequence
            circuit.pics[chip].icw4 = Int(iodata & 1)  // ICW1.IC4
            if (iodata & 0x02) != 0 {  // ICW1.SNGL
                assert(false, "ICW1: SNGL == 1 not supported")
            }
            if (iodata & 0x08) != 0 {  // ICW1.LTIM
                assert(false, "ICW1: LTIM == 1 not supported")
            }
        } else if (iodata & 0x08) != 0 {  // ...(bit 3 == 1) are OCW3
            if (iodata & 0x02) != 0 {  // OCW3.RR (read register command)
                circuit.pics[chip].read_reg_select = Int(iodata & 1)
            }
            if (iodata & 0x40) != 0 {  // OCW3.ESMM (special mask mode)
                circuit.pics[chip].special_mask = Int((iodata >> 5) & 1)
            }
        } else { // ...( bit 4 == 0 && bit 3 == 0) are OCW2
            switch iodata {
            case 0x00,
                 0x80: // rotate in automatic EOI mode (clear)
                circuit.pics[chip].rotate_on_autoeoi = Int(iodata >> 7)
                break
            case 0x20, // non-specific EOI command
                 0xa0: // rotate on non-specific EOI command
                priority = circuit.pics[chip].get_priority(circuit.pics[chip].isr)
                if priority >= 0 {
                    circuit.pics[chip].isr &= ~(1 << ((priority + circuit.pics[chip].priority_add) & 7))
                }
                if iodata == 0xa0 {
                    circuit.pics[chip].priority_add = (circuit.pics[chip].priority_add + 1) & 7
                }
                break
            case 0x60, // specific EOI command, IR priority level 0
                 0x61, // level 1
                 0x62, // level 2
                 0x63, // level 3
                 0x64, // level 4
                 0x65, // level 5
                 0x66, // level 6
                 0x67: // level 7
                priority = Int(iodata & 7)
                circuit.pics[chip].isr &= ~(1 << priority)
                break;
            case 0xc0, // set priority command, IR priority level 0
                 0xc1, // level 1
                 0xc2, // level 2
                 0xc3, // level 3
                 0xc4, // level 4
                 0xc5, // level 5
                 0xc6, // level 6
                 0xc7: // level 7
                circuit.pics[chip].priority_add = Int((iodata + 1) & 7)
                break
            case 0xe0,  // rotate on specific EOI command, IR level 0
                 0xe1,  // level 1
                 0xe2,  // level 2
                 0xe3,  // level 3
                 0xe4,  // level 4
                 0xe5,  // level 5
                 0xe6,  // level 6
                 0xe7:  // level 7
                priority = Int(iodata & 7)
                circuit.pics[chip].isr &= ~(1 << priority)
                circuit.pics[chip].priority_add = (priority + 1) & 7
                break
            default:
                break
            }
        }
    }
}
class PortA1<T: FixedWidthInteger & UnsignedInteger>: IOPort {
    let circuit: PIC
    let chip: Int
    init(_ circuit: PIC, _ chip: Int) {
        self.circuit = circuit
        self.chip = chip
    }
    func rd() -> T { 0xff }
    func wr(_ iodata: T) {
        switch circuit.pics[chip].icwn {
        case 0:  // OCW1, set IMR
            circuit.pics[chip].imr = Int(iodata)
            circuit.pics[chip].update_irq()
            break
        case 1:  // ICW2, set page starting address of service routines
            circuit.pics[chip].irq_base = Int(iodata & 0xf8)
            circuit.pics[chip].icwn = 2
            break
        case 2:  // ICW3, load slave register if ICW1.SNGL == 0
            if circuit.pics[chip].icw4 != 0 {
                circuit.pics[chip].icwn = 3
            } else {
                circuit.pics[chip].icwn = 0
            }
            break
        case 3:  // ICW4, program SFNM, BUF, M/S, AEOI and uPM if set
            circuit.pics[chip].auto_eoi = Int((iodata >> 1) & 1)
            circuit.pics[chip].icwn = 0
            break
        default:
            break
        }
    }
}
class Port42<T: FixedWidthInteger & UnsignedInteger>: IOPort {
    let circuit: PIT
    let port: Int
    init(_ circuit: PIT, _ port: Int) {
        self.circuit = circuit
        self.port = port
    }
    func rd() -> T {
        var res = 0
        switch circuit.pit_channels[port].rw_state {
        case 0, 1, 2, 3:
            let ma = circuit.pit_channels[port].pit_get_count();
            if (circuit.pit_channels[port].rw_state & 1) != 0 {
                res = (ma >> 8) & 0xff
            } else {
                res = ma & 0xff
            }
            if (circuit.pit_channels[port].rw_state & 2) != 0 {
                circuit.pit_channels[port].rw_state ^= 1
            }
            break
        case 4, 5:
            if (circuit.pit_channels[port].rw_state & 1) != 0 {
                res = circuit.pit_channels[port].latched_count >> 8
            } else {
                res = circuit.pit_channels[port].latched_count & 0xff
            }
            circuit.pit_channels[port].rw_state ^= 1
            break
        default:
            break
        }
        return T(res)
    }
    func wr(_ iodata: T) {
        switch circuit.pit_channels[port].rw_state {
        case 0:
            circuit.pit_channels[port].pit_load_count(Int(iodata))
            break
        case 1:
            circuit.pit_channels[port].pit_load_count(Int(iodata) << 8)
            break
        case 2, 3:
            if (circuit.pit_channels[port].rw_state & 1) != 0 {
                circuit.pit_channels[port].pit_load_count((circuit.pit_channels[port].latched_count & 0xff) | (Int(iodata) << 8))
            } else {
                circuit.pit_channels[port].latched_count = Int(iodata)
            }
            circuit.pit_channels[port].rw_state ^= 1
            break
        default:
            break
        }
    }
}
class Port43<T: FixedWidthInteger & UnsignedInteger>: IOPort {
    var hh = 0
    let circuit: PIT
    init(_ circuit: PIT) {
        self.circuit = circuit
    }
    func rd() -> T { 0xff }
    func wr(_ iodata: T) {
        hh = Int(iodata >> 6)
        if hh == 3 {
            return
        }
        let ih = (iodata >> 4) & 3
        switch ih {
        case 0:
            circuit.pit_channels[hh].latched_count = circuit.pit_channels[hh].pit_get_count()
            circuit.pit_channels[hh].rw_state = 4
            break
        default:
            circuit.pit_channels[hh].mode = Int((iodata >> 1) & 7)
            circuit.pit_channels[hh].bcd = Int(iodata & 1)
            circuit.pit_channels[hh].rw_state = Int(ih - 1 + 0)
            break
        }
    }
}
class Port61<T: FixedWidthInteger & UnsignedInteger>: IOPort {
    let circuit: PIT
    init(_ circuit: PIT) {
        self.circuit = circuit
    }
    func rd() -> T {
        let eh = circuit.pit_channels[2].pit_get_out()
        let iodata = (circuit.speaker_data_on << 1) | circuit.pit_channels[2].gate | (eh << 5)
        return T(iodata)
    }
    func wr(_ iodata: T) {
        circuit.speaker_data_on = Int((iodata >> 1) & 1)
        circuit.pit_channels[2].gate = Int(iodata & 1)
    }
}
class Port3F8<T: FixedWidthInteger & UnsignedInteger>: IOPort {
    let circuit: Serial
    init(_ circuit: Serial) {
        self.circuit = circuit
    }
    func rd() -> T {
        if (circuit.lcr & 0x80) != 0 {
            return T(circuit.divider & 0xff)
        } else {
            let reg = circuit.rbr
            circuit.lsr &= ~(0x01 | 0x10)
            circuit.update_irq()
            // circuit.input_fifo_pop()
            return T(reg)
        }
}
    func wr(_ iodata: T) {
        if (circuit.lcr & 0x80) != 0 {
            circuit.divider = (circuit.divider & 0xff00) | Int(iodata)
        } else {
            circuit.lsr &= ~0x20
            circuit.update_irq()
            // circuit.print_fifo_push(iodata)
            print(String(format: "%c", iodata as! CVarArg), terminator: "")
            circuit.lsr |= 0x20
            circuit.lsr |= 0x40
            circuit.update_irq()
        }
    }
}
class Port3F9<T: FixedWidthInteger & UnsignedInteger>: IOPort {
    let circuit: Serial
    init(_ circuit: Serial) {
        self.circuit = circuit
    }
    func rd() -> T {
        if (circuit.lcr & 0x80) != 0 {
            return T((circuit.divider >> 8) & 0xff)
        } else {
            return T(circuit.ier)
        }
    }
    func wr(_ iodata: T) {
        if (circuit.lcr & 0x80) != 0 {
            circuit.divider = (circuit.divider & 0x00ff) | (Int(iodata) << 8)
        } else {
            circuit.ier = Int(iodata)
            circuit.update_irq()
        }
    }
}
class Port3FA<T: FixedWidthInteger & UnsignedInteger>: IOPort {
    let circuit: Serial
    init(_ circuit: Serial) {
        self.circuit = circuit
    }
    func rd() -> T { T(circuit.iir) }
    func wr(_ iodata: T) { }
}
class Port3FB<T: FixedWidthInteger & UnsignedInteger>: IOPort {
    let circuit: Serial
    init(_ circuit: Serial) {
        self.circuit = circuit
    }
    func rd() -> T { T(circuit.lcr) }
    func wr(_ iodata: T) {
        circuit.lcr = Int(iodata)
    }
}
class Port3FC<T: FixedWidthInteger & UnsignedInteger>: IOPort {
    let circuit: Serial
    init(_ circuit: Serial) {
        self.circuit = circuit
    }
    func rd() -> T { T(circuit.mcr) }
    func wr(_ iodata: T) {
        circuit.mcr = Int(iodata)
    }
}
class Port3FD<T: FixedWidthInteger & UnsignedInteger>: IOPort {
    let circuit: Serial
    init(_ circuit: Serial) {
        self.circuit = circuit
    }
    func rd() -> T { T(circuit.lsr) }
    func wr(_ iodata: T) { }
}
class Port3FE<T: FixedWidthInteger & UnsignedInteger>: IOPort {
    let circuit: Serial
    init(_ circuit: Serial) {
        self.circuit = circuit
    }
    func rd() -> T { T(circuit.msr) }
    func wr(_ iodata: T) {
        circuit.msr = Int(iodata)
    }
}
class Port3FF<T: FixedWidthInteger & UnsignedInteger>: IOPort {
    let circuit: Serial
    init(_ circuit: Serial) {
        self.circuit = circuit
    }
    func rd() -> T { T(circuit.scr) }
    func wr(_ iodata: T) {
        circuit.scr = Int(iodata)
    }
}
