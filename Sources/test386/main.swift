import Foundation
import Free86

var fileURL = URL(fileURLWithPath: "../test386.asm/test386.bin")
let loadAddress: DWord = 0x000f0000
let postPortAddress: DWord = 0x0190
let outPortAddress: DWord  = 0x002a

let mem = MemoryIO<DWord>(capacity: 16 * 1024 * 1024)  // 16 MB
let data = try Data(contentsOf: fileURL)
for (offset, byte) in data.enumerated() {
    mem[loadAddress + DWord(offset)] = Byte(byte)
}

let postPort = PostPort<Byte>()
let outPort = OutPort<Byte>()
let io = IsolatedIO<DWord>()
io.register(port: postPort, at: postPortAddress)
io.register(port: outPort, at: outPortAddress)

let cpu = Free86(memory: mem, io: io)

var historySkip = 0
let historySize = 16
var history = [String](repeating: "", count: historySize)

let argv = CommandLine.arguments
if argv.count == 3  {
    historySkip = Int(argv[1])!
    fileURL = URL(fileURLWithPath: argv[2])
}

while true {
    let cycles = cpu.cycles + QWord(historySkip > 0 ? 1 : 100000)
    let eip15: () -> String = {
        var linear = cpu.segs[.CS].shadow.base + cpu.eip
        let physical: DWord
        var n: Int

        if cpu.cr0.isPagingEnabled {
            do {
                physical = try cpu.tlbLookup(linear: linear, writable: false)
                n = 4096 - Int(linear & 0xfff)  // bytes left to end-of-page...
                n = min(n, 15)  // ...or up to maximum instruction length bytes
            } catch {
                print("\(error)")
                exit(EXIT_FAILURE)
            }
        } else {
            linear &= 0xfffff
            physical = linear
            n = 15
        }
        var memory = "[EIP..EIP+\(n - 1)]:"
        for i in 0..<n {
            memory += String(format: " %02X", mem.ld8(from: physical + DWord(i)))
        }
        return memory
    }

    while cycles > cpu.cycles {
        do {
            try await cpu.fetchDecodeExecute(cycles: cycles - cpu.cycles)
            // print(cpu.compactState())
            if historySkip > 0 && cycles > historySkip {
                history[Int(cpu.cycles) % historySize] = cpu.compactState()
            }
            if cpu.halted {
                if historySkip > 0 {
                    for i in 0..<historySize {
                        print("\(history[(Int(cpu.cycles) + 1 - 4 + i) % historySize])")
                    }
                }
                print("\(cpu.cycles)")
                exit(EXIT_SUCCESS)
            }
        } catch let interrupt as Interrupt {
            print("\(interrupt)")
            cpu.interrupt = interrupt
            switch interrupt.id {
            case 6:
                print(cpu.compactState() + "\n" + eip15())
                break
            default :
                break
            }
        } catch {
            print("\(error)")
            exit(EXIT_FAILURE)
        }
    }
}

extension Free86 {
    // EAX:00000000                ESP:CAFE55AA
    // ECX:00000000                EBP:CAFE55AA
    // EDX:00000000
    // EBX:00000000
    // ESI:00000000
    // EDI:00000000                EIP:CAFE55AA
    //
    //        EFLAGS:00010001_00010001_00001111
    //
    // ES:CAFE:CAFE55AA:CAFFE:00010001_00001111
    // CS:CAFE:CAFE55AA:CAFFE:00010001_00001111
    // SS:CAFE:CAFE55AA:CAFFE:00010001_00001111
    // DS:CAFE:CAFE55AA:CAFFE:00010001_00001111
    // FS:CAFE:CAFE55AA:CAFFE:00010001_00001111
    // GS:CAFE:CAFE55AA:CAFFE:00010001_00001111
    //
    // CR0:0..01111  CR2:DEADBEAF  CR3:DEADB000
    func state() -> String {
        var cr0 = bin(self.cr0, divide: true)
        let a = cr0.index(cr0.startIndex, offsetBy: 1)
        let o = cr0.index(cr0.startIndex, offsetBy: 30)
        cr0.replaceSubrange(a..<o, with: "..")
        return String(format: """
            EAX:%08X                ESP:%08X
            ECX:%08X                EBP:%08X
            EDX:%08X
            EBX:%08X
            ESI:%08X
            EDI:%08X                EIP:%08X

                   EFLAGS:%@

            ES:%04X:%08X:%05X:%@
            CS:%04X:%08X:%05X:%@
            SS:%04X:%08X:%05X:%@
            DS:%04X:%08X:%05X:%@
            FS:%04X:%08X:%05X:%@
            GS:%04X:%08X:%05X:%@

            CR0:%@  CR2:%08X  CR3:%08X
            """,
            regs[.EAX], regs[.ESP], regs[.ECX], regs[.EBP],
            regs[.EDX], regs[.EBP], regs[.ESI], regs[.EDI],
            eip, String(bin(eflags, divide: true).suffix(26)),
            segs[.ES].selector, segs[.ES].shadow.base, segs[.ES].shadow.limit, bin(Word((segs[.ES].shadow.flags >> 8) & 0xffff), divide: true),
            segs[.CS].selector, segs[.CS].shadow.base, segs[.CS].shadow.limit, bin(Word((segs[.CS].shadow.flags >> 8) & 0xffff), divide: true),
            segs[.SS].selector, segs[.SS].shadow.base, segs[.SS].shadow.limit, bin(Word((segs[.SS].shadow.flags >> 8) & 0xffff), divide: true),
            segs[.DS].selector, segs[.DS].shadow.base, segs[.DS].shadow.limit, bin(Word((segs[.DS].shadow.flags >> 8) & 0xffff), divide: true),
            segs[.FS].selector, segs[.FS].shadow.base, segs[.FS].shadow.limit, bin(Word((segs[.FS].shadow.flags >> 8) & 0xffff), divide: true),
            segs[.GS].selector, segs[.GS].shadow.base, segs[.GS].shadow.limit, bin(Word((segs[.GS].shadow.flags >> 8) & 0xffff), divide: true),
            cr0, cr2, cr3)
    }
    // A:DEADBEAF C:DEADBEAF D:DEADBEAF B:DEADBEAF SI:DEADBEAF DI:DEADBEAF I:CAFE55AA SP:CAFE55AA BP:CAFE55AA F:0001_00001111
    func compactState() -> String {
        String(format: "A:%08X C:%08X D:%08X B:%08X SI:%08X DI:%08X I:%08X SP:%08X BP:%08X F:%@", regs[.EAX], regs[.ECX], regs[.EDX], regs[.EBX], regs[.ESI], regs[.EDI], eip, regs[.ESP], regs[.EBP], String(bin(eflags, divide: true).suffix(22)))
    }

    func bin(_ bits: Byte) -> String {
        var result = ""
        result.reserveCapacity(8)
        for shift in stride(from: 7, through: 0, by: -1) {
            result.append(((bits >> shift) & 1) == 1 ? "1" : "0")
        }
        return result
    }
    func bin(_ bits: Word, divide: Bool = false) -> String {
        bin(Byte(bits >> 8)) + (divide ? "_" : "") + bin(Byte(bits & 0xff))
    }
    func bin(_ bits: DWord, divide: Bool = false) -> String {
        bin(Word(bits >> 16), divide: divide) + (divide ? "_" : "") + bin(Word(bits & 0xffff), divide: divide)
    }
    func bin(_ bits: QWord, divide: Bool = false) -> String {
        bin(DWord(bits >> 32), divide: divide) + (divide ? "_" : "") + bin(DWord(bits & 0xffffffff), divide: divide)
    }
}

extension MemoryIO<DWord> {
    convenience init(capacity: A) {
        assert(capacity % A(A.bankSize) == 0, "fatal error")
        self.init(defaultBank: DefaultBank<A>())
        for addr in stride(from: 0, to: capacity, by: A.bankSize) {
            self.register(bank: RAMBank<A>(), at: addr)
        }
    }
}

class PostPort<T: UnsignedInteger>: IOPort {
    func rd() -> T { 0xfe }
    func wr(_ iodata: T) {
        print(String(format: "%02X", iodata as! CVarArg))
    }
}

class OutPort<T: UnsignedInteger>: IOPort {
    func rd() -> T { 0xfe }
    func wr(_ iodata: T) {
        print(String(format: "%c", iodata as! CVarArg))
    }
}
