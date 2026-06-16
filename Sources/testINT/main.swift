import Foundation
import Free86

var fileURL = URL(fileURLWithPath: "testINT.bin")
let loadAddress: DWord = 0x000f0000
let debugPortAddress: DWord  = 0x002a

let mem = MemoryIO<DWord>(capacity: 16 * 1024 * 1024)  // 16 MB
let data = try Data(contentsOf: fileURL)
for (offset, byte) in data.enumerated() {
    mem[loadAddress + DWord(offset)] = Byte(byte)
}

let io = IsolatedIO<DWord>()
let cpu = Free86(memory: mem, io: io)

let outPort = OutPort<Byte>()
io.register(port: outPort, at: debugPortAddress)

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

class OutPort<T: FixedWidthInteger & UnsignedInteger>: IOPort {
    func rd() -> T { 0xff }
    func wr(_ iodata: T) {
        print(String(format: "%c", iodata as! CVarArg))
    }
}
