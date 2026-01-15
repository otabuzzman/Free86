import Foundation
import Free86

let mem = MemoryIO<DWord>(capacity: 16 * 1024 * 1024)  // 16 MB

let bootloaderFileURL = URL(fileURLWithPath: "bin/linuxstart.bin")
let bootloaderAddress: DWord = 0x0001_0000
try mem.load(file: bootloaderFileURL, at: bootloaderAddress)
let linosKernelFileURL = URL(fileURLWithPath: "bin/vmlinux-2.6.20.bin")
let linosKernelAddress: DWord = 0x0010_0000
try mem.load(file: linosKernelFileURL, at: linosKernelAddress)
let initRamDiskFileURL = URL(fileURLWithPath: "bin/root.bin")
let initRamDiskAddress: DWord = 0x0040_0000
try mem.load(file: initRamDiskFileURL, at: initRamDiskAddress)

let cmdLine = "console=ttyS0 root=/dev/ram0 rw init=/sbin/init notsc=1"
let cmdLineAddress: DWord = 0x0000_f800
mem.load(utf8: cmdLine, at: cmdLineAddress)

let cpuStateInitFileURL = URL(fileURLWithPath: "bin/bootstrap.bin")
let cpuStateInitAddress: DWord = 0x000f_0000
try mem.load(file: cpuStateInitFileURL, at: cpuStateInitAddress)

extension MemoryIO<DWord> {
    convenience init(capacity: A) {
        assert(capacity % A(A.bankSize) == 0)
        self.init(defaultBank: DefaultBank<A>())
        for address in stride(from: 0, to: capacity, by: A.bankSize) {
            self.register(bank: RAMBank<A>(), at: address)
        }
    }

    func load(utf8: String, at: A) {
        let data = utf8.data(using: .utf8)!
        load(bytes: data, at: at)
    }

    func load(file: URL, at: A) throws {
        let data = try Data(contentsOf: file)
        load(bytes: data, at: at)
    }

    func load(bytes: Data, at: A) {
        for (offset, byte) in bytes.enumerated() {
            self[at + A(offset)] = Byte(byte)
        }
    }
}
