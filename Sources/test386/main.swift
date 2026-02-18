import Foundation
import Free86

let testSuiteFileURL = URL(fileURLWithPath: "../test386.asm/test386.bin")
let testSuiteAddress: DWord = 0x000f0000

let mem = MemoryIO<DWord>(capacity: 16 * 1024 * 1024)  // 16 MB
try mem.load(file: testSuiteFileURL, storeAt: testSuiteAddress)

extension MemoryIO<DWord> {
    convenience init(capacity: A) {
        assert(capacity % A(A.bankSize) == 0, "fatal error")
        self.init(defaultBank: DefaultBank<A>())
        for addr in stride(from: 0, to: capacity, by: A.bankSize) {
            self.register(bank: RAMBank<A>(), at: addr)
        }
    }
}

@MainActor
extension MemoryIO<DWord> {
    func load(file: URL, storeAt address: A) throws {
        let data = try Data(contentsOf: file)
        for (offset, byte) in data.enumerated() {
            mem[address + A(offset)] = Byte(byte)
        }
    }
}
