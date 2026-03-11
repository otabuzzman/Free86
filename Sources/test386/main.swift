import Foundation
import Free86

let fileURL = URL(fileURLWithPath: "../test386.asm/test386.bin")
let loadAddress: DWord = 0x000f0000
let outPortAddress: DWord  = 0x002a
let postPortAddress: DWord = 0x0190

let mem = MemoryIO<DWord>(capacity: 16 * 1024 * 1024)  // 16 MB
let data = try Data(contentsOf: fileURL)
for (offset, byte) in data.enumerated() {
    mem[loadAddress + DWord(offset)] = Byte(byte)
}

let outPort = OutPort<Byte>()
let io = IsolatedIO<DWord>()
io.register(port: outPort, at: postPortAddress)
io.register(port: outPort, at: outPortAddress)

let cpu = Free86(memory: mem, io: io)

let historySkip = 0
let historySize = 512
var history = [String](repeating: "", count: historySize)

while true {
    let number: QWord = 100000
    let cycles = cpu.cycles + number
    while cpu.cycles < cycles {
        do {
            try await cpu.fetchDecodeExecute(cycles: cycles)
            // print(cpu.status())
            if number == 1 && cycles > historySkip {
                history[Int(cpu.cycles) % historySize] = cpu.status()
            }
            if cpu.halted {
                if number == 1 {
                    for i in 0..<historySize {
                        print("\(history[(Int(cpu.cycles) + 1 + i) % historySize])")
                    }
                }
                print("\(cpu.cycles)")
                exit(EXIT_SUCCESS)
            }
        } catch let interrupt as Interrupt {
            print("\(interrupt)")
            cpu.interrupt = interrupt
        } catch {
            print("\(error)")
            exit(EXIT_FAILURE)
        }
    }
}

extension Free86 {
    func status() -> String {
        ""
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

class OutPort<T: UnsignedInteger>: IOPort {
    func rd() -> T { 0 }
    func wr(_ iodata: T) {
        print(String(format: "%02x", iodata as! CVarArg))
    }
}
