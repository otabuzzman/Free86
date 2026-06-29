import Foundation
import Free86

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

var cycles = cpu.cycles + 100000
try await cpu.fetchDecodeExecuteLoop(cycles: cycles)

try await cpu.INTR.trigger(0x20)  // test 1
try await cpu.fetchDecodeExecuteLoop(cycles: cycles)

try await cpu.NMI.trigger(true)   // test 2
try await cpu.fetchDecodeExecuteLoop(cycles: cycles)

try await cpu.INTR.trigger(0x20)  // test 4
try await cpu.fetchDecodeExecuteLoop(cycles: cycles)

try await cpu.INTR.trigger(0x20)  // test 4 (blocked)
try await cpu.fetchDecodeExecuteLoop(cycles: cycles)

try await cpu.INTR.trigger(0x20)  // test 4 (blocked)
try await cpu.fetchDecodeExecuteLoop(cycles: cycles)

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
        print(String(format: "0x%02X", iodata as! CVarArg))
    }
}

extension Free86 {
    func fetchDecodeExecuteLoop(cycles: QWord, stopOnHalt: Bool = true) async throws {
        while cycles > self.cycles {
            do {
                try await fetchDecodeExecute(cycles: cycles - self.cycles)
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
