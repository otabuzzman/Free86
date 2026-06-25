import Foundation
import Free86

var fileURL = URL(fileURLWithPath: "bin/testINT.bin")
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
    while cycles > cpu.cycles {
        do {
            try await cpu.fetchDecodeExecute(cycles: cycles - cpu.cycles)
            if cpu.halted {
                print("\(cpu.cycles)")
                exit(EXIT_SUCCESS)
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
