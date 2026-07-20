import Foundation
import Free86

// var fileURL = URL(fileURLWithPath: "bin/testINTs.bin")
var fileURL = URL(fileURLWithPath: "bin/testINTr.bin")
let loadAddress: DWord = 0x000f0000
let debugPortAddress: DWord  = 0x2a

let mem = MemoryIO<DWord>(capacity: 16 * 1024 * 1024)  // 16 MB
let data = try Data(contentsOf: fileURL)
for (offset, byte) in data.enumerated() {
    mem[loadAddress + DWord(offset)] = Byte(byte)
}

let io = IsolatedIO<DWord>()
let cpu = Free86(memory: mem, io: io)

let debugPort = DebugPort<Byte>()
io.register(port: debugPort, at: debugPortAddress)

let cycles = cpu.cycles + 4711

/// test 1: divide by 0 exception (#DE)
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.regs[.ESP] == 0x7c00)

/// test 2: HW interrupt on INTR
try await cpu.INTR.trigger(32)
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.regs[.ESP] == 0x7c00)

/// test 3: HW interrupt on NMI
try await cpu.NMI.trigger(true)
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.regs[.ESP] == 0x7c00)

/// test 4: HW interrupt on INTR
try await cpu.INTR.trigger(32)
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.cycles == 76)  // for later inspection
assert(cpu.regs[.ESP] == 0x7bfa)
/// test 4: nested INTR blocked
try await cpu.INTR.trigger(32)   // nested INTR...
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.cycles == 76)  // ...blocked
assert(cpu.regs[.ESP] == 0x7bfa)
try await cpu.INTR.trigger(32)   // nested INTR...
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.cycles == 76)  // ...blocked
assert(cpu.regs[.ESP] == 0x7bfa)
/// test 4: RESET to continue
try await cpu.RESET.trigger(true)

/// test 5: divide by 0 exception (#DE)
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.regs[.ESP] == 0x7c00)

/// test 6: HW interrupt on INTR (sti in ISR)
try await cpu.INTR.trigger(32)
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.regs[.ESP] == 0x7bfa)

/// test 7: nested INTR allowed
try await cpu.INTR.trigger(32)
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.regs[.ESP] == 0x7c00)

/// test 8: HW interrupt on NMI
try await cpu.NMI.trigger(true)
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.cycles == 150)  // for later inspection
assert(cpu.regs[.ESP] == 0x7bfa)
/// test 8: nested NMI blocked
try await cpu.NMI.trigger(true)   // nested NMI...
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.cycles == 150)  // ...blocked
assert(cpu.regs[.ESP] == 0x7bfa)
try await cpu.NMI.trigger(true)   // nested NMI...
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.cycles == 150)  // ...blocked
assert(cpu.regs[.ESP] == 0x7bfa)
/// test 8: nested INTR blocked
try await cpu.INTR.trigger(32)   // nested INTR...
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.cycles == 150)  // ...blocked
assert(cpu.regs[.ESP] == 0x7bfa)
try await cpu.INTR.trigger(32)   // nested INTR...
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.cycles == 150)  // ...blocked
assert(cpu.regs[.ESP] == 0x7bfa)
/// test 8: RESET to continue
try await cpu.RESET.trigger(true)

/// test 9: divide by 0 exception (#DE)
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.cycles == 182)  // for later inspection
assert(cpu.regs[.ESP] == 0x7bfa)
/// test 9: nested NMI blocked
try await cpu.NMI.trigger(true)   // nested NMI...
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.cycles == 182)  // ...blocked
assert(cpu.regs[.ESP] == 0x7bfa)
try await cpu.NMI.trigger(true)   // nested NMI...
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.cycles == 182)  // ...blocked
assert(cpu.regs[.ESP] == 0x7bfa)
/// test 9: nested INTR blocked
try await cpu.INTR.trigger(32)   // nested INTR...
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.cycles == 182)  // ...blocked
assert(cpu.regs[.ESP] == 0x7bfa)
try await cpu.INTR.trigger(32)   // nested INTR...
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.cycles == 182)  // ...blocked
assert(cpu.regs[.ESP] == 0x7bfa)
/// test 9: RESET to continue
try await cpu.RESET.trigger(true)

/// test 10: divide by 0 exception (#DE)
try await cpu.INTR.trigger(32)
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.regs[.ESP] == 0x7c00)

/// test 11: HW interrupt on INTR
/// test 12: nested #DE in INTR ISR
try await cpu.INTR.trigger(32)
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.regs[.ESP] == 0x7c00)

/// test 13: HW interrupt on NMI
/// test 14: nested #DE in NMI ISR
/// test 15: nested #DE in #DE ISR (double fault)
/// test 16: triple fault
try await cpu.NMI.trigger(true)
await cpu.fetchDecodeExecuteLoop(cycles: cycles)
assert(cpu.halted == true)  // shutdown state
assert(cpu.regs[.ESP] == 0x7be8)



/*
Task.detached {
    while true {
        let c = Int(getchar())
        switch c {
        case 105, 73:  // 'i', 'I'
            let intr = Byte(Int.random(in: 0..<256))
            print("INTR \(intr) ", terminator: "")
            Task { @MainActor in
                try await cpu.INTR.trigger(intr)
            }
        case 110, 78:  // 'n', 'N'
            print("NMI ", terminator: "")
            Task { @MainActor in
                try await cpu.NMI.trigger(true)
            }
        case 114, 82:  // 'r', 'R'
            print("RESET")
            Task { @MainActor in
                try await cpu.RESET.trigger(true)
            }
        default:
            break
        }
        if Task.isCancelled { break }
    }
}

while true {
    let cycles = cpu.cycles + 100000
    await cpu.fetchDecodeExecuteLoop(cycles: cycles)
}
*/

extension Free86 {
    func fetchDecodeExecuteLoop(cycles: QWord) async {
        while cycles > self.cycles {
            do {
                try await self.fetchDecodeExecute(cycles: cycles - self.cycles)
                if self.halted {
                    print("\(self.cycles) cycles")
                    break
                }
            } catch let interrupt as Interrupt {
                self.interrupt = interrupt
            } catch {
                print("\(error)")
                exit(EXIT_FAILURE)
            }
        }
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

class DebugPort<T: FixedWidthInteger & UnsignedInteger>: IOPort {
    func rd() -> T { 0xff }
    func wr(_ iodata: T) {
        print(String(format: "%d", iodata as! CVarArg))
    }
}
