import Foundation
import Free86

let bootloaderFileURL = URL(fileURLWithPath: "bin/linuxstart.bin")
let bootloaderAddress: DWord = 0x00010000
let linosKernelFileURL = URL(fileURLWithPath: "bin/vmlinux-2.6.20.bin")
let linosKernelAddress: DWord = 0x00100000
let cpuStateInitFileURL = URL(fileURLWithPath: "bin/bootstrap.bin")
let cpuStateInitAddress: DWord = 0x000f0000

let mem = Memory<DWord>(capacity: 16 * 1024 * 1024)  // 16 MB
try mem.load(file: bootloaderFileURL, at: bootloaderAddress)
try mem.load(file: linosKernelFileURL, at: linosKernelAddress)
try mem.load(file: cpuStateInitFileURL, at: cpuStateInitAddress)

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
