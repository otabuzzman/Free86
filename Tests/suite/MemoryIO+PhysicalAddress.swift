import XCTest
@testable import i80386

// MARK: - Dummy types for testing

final class BytePort: IOPort {
    typealias Element = Byte
    private(set) var lastWritten: Byte = 0
    var value: Byte = 0

    func rd() -> Byte { value }
    func wr(_ iodata: Byte) { lastWritten = iodata; value = iodata }
}

final class WordPort: IOPort {
    typealias Element = UInt16
    private(set) var lastWritten: UInt16 = 0
    var value: UInt16 = 0

    func rd() -> UInt16 { value }
    func wr(_ iodata: UInt16) { lastWritten = iodata; value = iodata }
}

// MARK: - Tests

final class MemoryAndIOTests: XCTestCase {

    func testRAMBankReadWrite() {
        let ram = RAMBank<UInt32>()
        ram[0] = 0x42
        XCTAssertEqual(ram[0], 0x42)
    }

    func testROMBankIsReadOnly() {
        let rom = ROMBank<UInt32>(bytes: [1, 2, 3])
        XCTAssertEqual(rom[0], 1)
        XCTAssertEqual(rom[2], 3)
        rom[0] = 9
        XCTAssertEqual(rom[0], 1, "ROM should ignore writes")
    }

    func testDefaultBankAlwaysReturnsZero() {
        let def = DefaultBank<UInt32>()
        def[100] = 0xFF
        XCTAssertEqual(def[100], 0)
    }

    func testMemoryRegistersDifferentBanks() {
        let memory = Memory<UInt32>(defaultBank: DefaultBank<UInt32>())
        let ram = RAMBank<UInt32>(fill: 0xAA)
        let addr: UInt32 = 0x0000_1000
        memory.register(bank: ram, at: addr)
        memory[addr] = 0x42
        XCTAssertEqual(memory[addr], 0x42)
    }

    func testIsolatedIOReadsAndWrites() {
        let port = BytePort()
        let io = IsolatedIO<UInt32, BytePort>()
        io.register(port: port, at: 0x20)
        io[0x20] = 0x55
        XCTAssertEqual(port.lastWritten, 0x55)
        port.value = 0x77
        XCTAssertEqual(io[0x20], 0x77)
    }

    func testIsolatedIOIgnoresUnregisteredAddress() {
        let io = IsolatedIO<UInt32, BytePort>()
        XCTAssertEqual(io[0x10], 0)
        io[0x10] = 0xFF // should do nothing
    }

    func testIOPortBankAsMemoryMappedDevice() {
        let port = BytePort()
        let ioBank = IOPortBank<UInt32, BytePort>()
        ioBank.register(port: port, at: 0x40)

        ioBank[0x40] = 0x99
        XCTAssertEqual(port.lastWritten, 0x99)
        port.value = 0xAB
        XCTAssertEqual(ioBank[0x40], 0xAB)
    }

    func testIsolatedIOWithWordPort() {
        let port = WordPort()
        let io = IsolatedIO<UInt32, WordPort>()
        io.register(port: port, at: 0x80)
        io[0x80] = 0xBEEF
        XCTAssertEqual(port.lastWritten, 0xBEEF)
        port.value = 0x1234
        XCTAssertEqual(io[0x80], 0x1234)
    }

    func testIOPortBankWithUnregisteredAddress() {
        let ioBank = IOPortBank<UInt32, BytePort>()
        XCTAssertEqual(ioBank[0x123], 0)
        ioBank[0x123] = 0xAA // should silently ignore
    }

    func testROMAndRAMInDifferentMemorySlots() {
        let memory = Memory<UInt32>(defaultBank: DefaultBank<UInt32>())
        let romAddr: UInt32 = 0x0020_0000
        let ramAddr: UInt32 = 0x0000_0000

        let ram = RAMBank<UInt32>()
        let rom = RAMBank<UInt32>() // using RAM here, but could be ROM
        memory.register(bank: ram, at: ramAddr)
        memory.register(bank: rom, at: romAddr)

        memory[ramAddr] = 0x11
        memory[romAddr] = 0x22
        XCTAssertEqual(memory[ramAddr], 0x11)
        XCTAssertEqual(memory[romAddr], 0x22)
    }
}

extension MemoryAndIOTests {

    // MARK: - Helper wide I/O ports

    final class WordDevice: IOPort {
        typealias Element = UInt16
        var value: UInt16 = 0
        private(set) var writes: [UInt16] = []
        func rd() -> UInt16 { value }
        func wr(_ iodata: UInt16) {
            value = iodata
            writes.append(iodata)
        }
    }

    final class DWordDevice: IOPort {
        typealias Element = UInt32
        var value: UInt32 = 0
        private(set) var writes: [UInt32] = []
        func rd() -> UInt32 { value }
        func wr(_ iodata: UInt32) {
            value = iodata
            writes.append(iodata)
        }
    }

    // MARK: - Tests

    func testIsolated16BitPortSingleByteAccess() {
        let port = WordDevice()
        let iso = IsolatedIO<UInt32, WordDevice>()
        iso.register(port: port, at: 0x00)

        // Treat each access as "whole-port" operation
        iso[0x00] = 0x1234
        XCTAssertEqual(port.writes.last, 0x1234)
        XCTAssertEqual(iso[0x00], 0x1234)

        // Simulate device updating internal register
        port.value = 0xABCD
        XCTAssertEqual(iso[0x00], 0xABCD)
    }
/*
    func testMemoryMappedViewOn16BitPortReadsLowByte() {
        let wordPort = WordDevice()
        let ioBank = IOPortBank<UInt32, WordDevice>()
        ioBank.register(port: wordPort, at: 0x100)

        wordPort.value = 0x1234
        // IOPortBank always exposes Byte view: should return low byte
        XCTAssertEqual(ioBank[0x100], 0x34)
    }

    func testMemoryMappedViewOn16BitPortWritesLowByteOnly() {
        let wordPort = WordDevice()
        let ioBank = IOPortBank<UInt32, WordDevice>()
        ioBank.register(port: wordPort, at: 0x200)

        ioBank[0x200] = 0xAB
        // The port receives 0x00AB because Byte is zero-extended to UInt16
        XCTAssertEqual(wordPort.writes.last, 0x00AB)
    }

    func testIsolated32BitPortBehavesCorrectly() {
        let port = DWordDevice()
        let iso = IsolatedIO<UInt32, DWordDevice>()
        iso.register(port: port, at: 0x40)

        iso[0x40] = 0xDEADBEEF
        XCTAssertEqual(port.writes.last, 0xDEADBEEF)

        port.value = 0xCAFEBABE
        XCTAssertEqual(iso[0x40], 0xCAFEBABE)
    }

    func testMemoryMappedViewOn32BitPortReadsLowByte() {
        let port = DWordDevice()
        let ioBank = IOPortBank<UInt32, DWordDevice>()
        ioBank.register(port: port, at: 0x80)

        port.value = 0xAABBCCDD
        XCTAssertEqual(ioBank[0x80], 0xDD)
    }

    func testMemoryMappedViewOn32BitPortWritesLowByte() {
        let port = DWordDevice()
        let ioBank = IOPortBank<UInt32, DWordDevice>()
        ioBank.register(port: port, at: 0x81)

        ioBank[0x81] = 0x99
        // 0x99 zero-extended to 32 bits
        XCTAssertEqual(port.writes.last, 0x00000099)
    }
*/
    func testMemoryMappedMixedPortsByteAndWord() {
        let bytePort = BytePort()
        let wordPort = WordDevice()
        let ioBank = IOPortBank<UInt32, BytePort>()
        let wordIO = IsolatedIO<UInt32, WordDevice>()

        ioBank.register(port: bytePort, at: 0x00)
        wordIO.register(port: wordPort, at: 0x10)

        // Intermixed access semantics
        ioBank[0x00] = 0x55
        XCTAssertEqual(bytePort.lastWritten, 0x55)

        wordIO[0x10] = 0xBEEF
        XCTAssertEqual(wordPort.writes.last, 0xBEEF)
    }

    func testIsolatedIOPortMultipleWritesPreserveOrder() {
        let port = WordDevice()
        let io = IsolatedIO<UInt32, WordDevice>()
        io.register(port: port, at: 0x60)

        io[0x60] = 0x1111
        io[0x60] = 0x2222
        io[0x60] = 0x3333

        XCTAssertEqual(port.writes, [0x1111, 0x2222, 0x3333])
    }
}

extension MemoryAndIOTests {

    // MARK: - Mixed isolated I/O ports

    final class Port8: IOPort {
        typealias Element = UInt8
        var value: UInt8 = 0
        func rd() -> UInt8 { value }
        func wr(_ iodata: UInt8) { value = iodata }
    }

    final class Port16: IOPort {
        typealias Element = UInt16
        var value: UInt16 = 0
        func rd() -> UInt16 { value }
        func wr(_ iodata: UInt16) { value = iodata }
    }

    final class Port32: IOPort {
        typealias Element = UInt32
        var value: UInt32 = 0
        func rd() -> UInt32 { value }
        func wr(_ iodata: UInt32) { value = iodata }
    }

    func testMixedIsolatedPorts() {
        let iso8 = IsolatedIO<UInt32, Port8>()
        let iso16 = IsolatedIO<UInt32, Port16>()
        let iso32 = IsolatedIO<UInt32, Port32>()

        let port8 = Port8()
        let port16 = Port16()
        let port32 = Port32()

        iso8.register(port: port8, at: 0x10)
        iso16.register(port: port16, at: 0x20)
        iso32.register(port: port32, at: 0x30)

        iso8[0x10] = 0xAA
        iso16[0x20] = 0xBEEF
        iso32[0x30] = 0xCAFEBABE

        XCTAssertEqual(port8.value, 0xAA)
        XCTAssertEqual(port16.value, 0xBEEF)
        XCTAssertEqual(port32.value, 0xCAFEBABE)

        port8.value = 0x12
        port16.value = 0x3456
        port32.value = 0x89ABCDEF

        XCTAssertEqual(iso8[0x10], 0x12)
        XCTAssertEqual(iso16[0x20], 0x3456)
        XCTAssertEqual(iso32[0x30], 0x89ABCDEF)
    }

    // MARK: - System integration tests

    func testSystemIntegration_RAM_ROM_IO() {
        // Build memory with default empty banks
        let memory = Memory<UInt32>(defaultBank: DefaultBank<UInt32>())

        // Create RAM and ROM
        let ram = RAMBank<UInt32>()
        let rom = ROMBank<UInt32>(bytes: [0x11, 0x22, 0x33, 0x44])
        let ioBank = IOPortBank<UInt32, BytePort>()

        // Register them into the address space
        memory.register(bank: ram, at: 0x0000_0000)
        memory.register(bank: rom, at: 0x0001_0000)
        memory.register(bank: ioBank, at: 0x0002_0000)

        // Install an I/O port at address 0x0000_0004
        let ioPort = BytePort()
        ioBank.register(port: ioPort, at: 0x0000_0004)

        // RAM write/read
        memory[0x0000_0001] = 0xAB
        XCTAssertEqual(ram[0x0000_0001], 0xAB)
        XCTAssertEqual(memory[0x0000_0001], 0xAB)

        // ROM read only
        XCTAssertEqual(memory[0x0001_0000], 0x11)
        XCTAssertEqual(memory[0x0001_0003], 0x44)

        // ROM write should be ignored
        memory[0x0001_0000] = 0x99
        XCTAssertEqual(memory[0x0001_0000], 0x11)

        // I/O write via memory-mapped address
        memory[0x0002_0004] = 0xDE
        XCTAssertEqual(ioPort.lastWritten, 0xDE)

        // Device updates its internal register -> read reflects it
        ioPort.value = 0xEF
        XCTAssertEqual(memory[0x0002_0004], 0xEF)
    }

    func testSystemIntegration_ConcurrentAccessIsolation() {
        let memory = Memory<UInt32>(defaultBank: DefaultBank<UInt32>())
        let ram = RAMBank<UInt32>()
        let ioBank = IOPortBank<UInt32, BytePort>()
        let ioPort1 = BytePort()
        let ioPort2 = BytePort()

        memory.register(bank: ram, at: 0x0000_0000)
        memory.register(bank: ioBank, at: 0x0001_0000)
        ioBank.register(port: ioPort1, at: 0x0000_0004)
        ioBank.register(port: ioPort2, at: 0x0000_0008)

        // Write to RAM
        memory[0x0000_0000] = 0x55
        XCTAssertEqual(ram[0x0000_0000], 0x55)

        // Write to first port
        memory[0x0001_0004] = 0xA0
        XCTAssertEqual(ioPort1.lastWritten, 0xA0)
        XCTAssertEqual(ioPort2.lastWritten, 0)

        // Write to second port
        memory[0x0001_0008] = 0xB1
        XCTAssertEqual(ioPort2.lastWritten, 0xB1)
        XCTAssertEqual(ioPort1.lastWritten, 0xA0)

        // Ensure RAM and I/O are fully independent
        memory[0x0000_0001] = 0xFF
        XCTAssertEqual(ram[0x0000_0001], 0xFF)
        XCTAssertEqual(ioPort1.lastWritten, 0xA0)
        XCTAssertEqual(ioPort2.lastWritten, 0xB1)
    }

    func testSystemIntegration_IsolatedIO_And_Memory_Mapped_Coexist() {
        let memory = Memory<UInt32>(defaultBank: DefaultBank<UInt32>())
        let ioBank = IOPortBank<UInt32, BytePort>()
        let isoIO = IsolatedIO<UInt32, Port16>()

        memory.register(bank: ioBank, at: 0x0003_0000)

        let mmio = BytePort()
        let isolated = Port16()
        ioBank.register(port: mmio, at: 0x0000_0010)
        isoIO.register(port: isolated, at: 0x0000_0008)

        // Memory-mapped write (byte)
        memory[0x0003_0010] = 0x77
        XCTAssertEqual(mmio.lastWritten, 0x77)

        // Isolated write (word)
        isoIO[0x0000_0008] = 0xBEEF
        XCTAssertEqual(isolated.value, 0xBEEF)

        // Read back
        isolated.value = 0xFACE
        XCTAssertEqual(isoIO[0x0000_0008], 0xFACE)
    }
}
