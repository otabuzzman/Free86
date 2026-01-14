import Foundation
import Free86

let testSuiteFileURL = URL(fileURLWithPath: "../test386.asm/test386.bin")
let testSuiteAddress: DWord = 0x000f0000

let mem = Memory<DWord>(capacity: 16 * 1024 * 1024)  // 16 MB
try mem.load(file: testSuiteFileURL, at: testSuiteAddress)

extension Memory<DWord> {
    convenience init(capacity: A) {
        assert(capacity % A(A.bankSize) == 0)
        self.init(defaultBank: DefaultBank<A>())
        for addr in stride(from: 0, to: capacity, by: A.bankSize) {
            self.register(bank: RAMBank<A>(), at: addr)
        }
    }
    
    func load(file: URL, at: A) throws {
        let data = try Data(contentsOf: file)
        for (offset, byte) in data.enumerated() {
            self[at + A(offset)] = Byte(byte)
        }
    }
}
