import Foundation
import Free86

let mem = MemoryIO<DWord>(capacity: 16 * 1024 * 1024)  // 16 MB

let bootloaderFileURL = URL(fileURLWithPath: "bin/linuxstart.bin")
let bootloaderAddress: DWord = 0x0001_0000
try mem.load(file: bootloaderFileURL, storeAt: bootloaderAddress)
let linosKernelFileURL = URL(fileURLWithPath: "bin/vmlinux-2.6.20.bin")
let linosKernelAddress: DWord = 0x0010_0000
try mem.load(file: linosKernelFileURL, storeAt: linosKernelAddress)
let initRamDiskFileURL = URL(fileURLWithPath: "bin/root.bin")
let initRamDiskAddress: DWord = 0x0040_0000
try mem.load(file: initRamDiskFileURL, storeAt: initRamDiskAddress)

let cmdLine = "console=ttyS0 root=/dev/ram0 rw init=/sbin/init notsc=1"
let cmdLineAddress: DWord = 0x0000_f800
mem.load(utf8: cmdLine, storeAt: cmdLineAddress)

let cpuStateInitFileURL = URL(fileURLWithPath: "bin/bootstrap.bin")
let cpuStateInitAddress: DWord = 0x000f_0000
try mem.load(file: cpuStateInitFileURL, storeAt: cpuStateInitAddress)

extension MemoryIO<DWord> {
    convenience init(capacity: A) {
        assert(capacity % A(A.bankSize) == 0)
        self.init(defaultBank: DefaultBank<A>())
        for address in stride(from: 0, to: capacity, by: A.bankSize) {
            self.register(bank: RAMBank<A>(), at: address)
        }
    }
}

extension MemoryIO {
    func load(utf8: String, storeAt address: A) {
        let data = utf8.data(using: .utf8)!
        for (offset, byte) in data.enumerated() {
            st8Direct(at: address + A(offset), byte: Byte(byte))
        }
    }

    func load(file: URL, storeAt address: A) throws {
        let data = try Data(contentsOf: file)
        for (offset, byte) in data.enumerated() {
            st8Direct(at: address + A(offset), byte: Byte(byte))
        }
    }
}
