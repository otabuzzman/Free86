import Testing
@testable import Free86

final class Port<T: UnsignedInteger>: IOPort {
    var buffer: T = 0

    func rd() -> T { buffer }
    func wr(_ iodata: T) { buffer = iodata }
}

@Test("read/ write default/ RAM/ ROM banks at 32 bit addresses")
func readWriteRamRomBanks() {
    let empty = DefaultBank<DWord>()
    empty[0] = 0x12
    empty[42] = 0x34
    empty[DWord(DWord.bankSize - 1)] = 0xEF
    #expect(empty[0] == 0)
    #expect(empty[42] == 0)
    #expect(empty[DWord(DWord.bankSize - 1)] == 0)
    #expect(empty[DWord(DWord.bankOffsetMask)] == 0)

    let ram = RAMBank<DWord>()
    ram[0] = 0x12
    ram[42] = 0x34
    ram[DWord(DWord.bankSize - 1)] = 0xEF
    #expect(ram[0] == 0x12)
    #expect(ram[42] == 0x34)
    #expect(ram[DWord(DWord.bankSize - 1)] == 0xEF)
    #expect(ram[DWord(DWord.bankOffsetMask)] == 0xEF)

    let rom = ROMBank<DWord>(bytes: [0xDE, 0xED, 0xC0, 0xDE])
    rom[0] = 0x12
    rom[1] = 0x34
    rom[2] = 0x56
    rom[3] = 0x78
    #expect(rom[0] == 0xDE)
    #expect(rom[1] == 0xED)
    #expect(rom[2] == 0xC0)
    #expect(rom[3] == 0xDE)
}

@Test("read/ write default/ RAM/ ROM banks at 64 bit addresses")
func readWriteRamRomBanks64() {
    let empty = DefaultBank<QWord>()
    empty[0] = 0x12
    empty[42] = 0x34
    empty[QWord(QWord.bankSize - 1)] = 0xEF
    #expect(empty[0] == 0)
    #expect(empty[42] == 0)
    #expect(empty[QWord(QWord.bankSize - 1)] == 0)
    #expect(empty[QWord(QWord.bankOffsetMask)] == 0)

    let ram = RAMBank<QWord>()
    ram[0] = 0x12
    ram[42] = 0x34
    ram[QWord(QWord.bankSize - 1)] = 0xEF
    #expect(ram[0] == 0x12)
    #expect(ram[42] == 0x34)
    #expect(ram[QWord(QWord.bankSize - 1)] == 0xEF)
    #expect(ram[QWord(QWord.bankOffsetMask)] == 0xEF)

    let rom = ROMBank<QWord>(bytes: [0xDE, 0xED, 0xC0, 0xDE])
    rom[0] = 0x12
    rom[1] = 0x34
    rom[2] = 0x56
    rom[3] = 0x78
    #expect(rom[0] == 0xDE)
    #expect(rom[1] == 0xED)
    #expect(rom[2] == 0xC0)
    #expect(rom[3] == 0xDE)
}

@Test("read/ write 8/ 16/ 32 bit isolated IO ports at 32 bit addresses")
func readWriteIsolatedIoPorts() {
    let port8 = Port<Byte>()
    let device8 = IsolatedIO<DWord, Port<Byte>>()
    device8.register(port: port8, at: 42)
    device8.register(port: port8, at: DWord(DWord.bankSize - 1))
    #expect(device8[0] == 0)
    device8[42] = 0xA5
    #expect(device8[42] == 0xA5)
    device8[DWord(DWord.bankSize - 1)] = 0x5A
    #expect(device8[DWord(DWord.bankSize - 1)] == 0x5A)

    let port16 = Port<Word>()
    let device16 = IsolatedIO<DWord, Port<Word>>()
    device16.register(port: port16, at: 42)
    device16.register(port: port16, at: DWord(DWord.bankSize - 1))
    #expect(device16[0] == 0)
    device16[42] = 0xDEAD
    #expect(device16[42] == 0xDEAD)
    device16[DWord(DWord.bankSize - 1)] = 0xADDE
    #expect(device16[DWord(DWord.bankSize - 1)] == 0xADDE)

    let port = Port<DWord>()
    let device = IsolatedIO<DWord, Port<DWord>>()
    device.register(port: port, at: 42)
    device.register(port: port, at: DWord(DWord.bankSize - 1))
    #expect(device[0] == 0)
    device[42] = 0xDEADC0DE
    #expect(device[42] == 0xDEADC0DE)
    device[DWord(DWord.bankSize - 1)] = 0xC0DEDEAD
    #expect(device[DWord(DWord.bankSize - 1)] == 0xC0DEDEAD)
}

@Test("read/ write 8/ 16/ 32 bit isolated IO ports at 64 bit addresses")
func readWriteIsolatedIoPorts64() {
    let port8 = Port<Byte>()
    let device8 = IsolatedIO<QWord, Port<Byte>>()
    device8.register(port: port8, at: 42)
    device8.register(port: port8, at: QWord(QWord.bankSize - 1))
    #expect(device8[0] == 0)
    device8[42] = 0xA5
    #expect(device8[42] == 0xA5)
    device8[QWord(QWord.bankSize - 1)] = 0x5A
    #expect(device8[QWord(QWord.bankSize - 1)] == 0x5A)

    let port16 = Port<Word>()
    let device16 = IsolatedIO<QWord, Port<Word>>()
    device16.register(port: port16, at: 42)
    device16.register(port: port16, at: QWord(QWord.bankSize - 1))
    #expect(device16[0] == 0)
    device16[42] = 0xDEAD
    #expect(device16[42] == 0xDEAD)
    device16[QWord(QWord.bankSize - 1)] = 0xADDE
    #expect(device16[QWord(QWord.bankSize - 1)] == 0xADDE)

    let port = Port<DWord>()
    let device = IsolatedIO<QWord, Port<DWord>>()
    device.register(port: port, at: 42)
    device.register(port: port, at: QWord(QWord.bankSize - 1))
    #expect(device[0] == 0)
    device[42] = 0xDEADC0DE
    #expect(device[42] == 0xDEADC0DE)
    device[QWord(QWord.bankSize - 1)] = 0xC0DEDEAD
    #expect(device[QWord(QWord.bankSize - 1)] == 0xC0DEDEAD)
}

@Test("read/ write multiple 8 bit IO ports bank at 32 bit addresses")
func readWriteMultipleMemoryMappedIoPorts() {
    let portA = Port<Byte>()
    let portB = Port<Byte>()
    let portC = Port<Byte>()
    let bank = IOPortBank<DWord, Port<Byte>>()
    bank.register(port: portA, at: 66)
    bank.register(port: portB, at: 45)
    bank.register(port: portC, at: 62)
    #expect(bank[0] == 0)
    #expect(bank[DWord(DWord.bankSize - 1)] == 0)
    bank[66] = 0xAA
    bank[45] = 0x42
    bank[62] = 0x55
    #expect(bank[66] == 0xAA)
    #expect(bank[45] == 0x42)
    #expect(bank[62] == 0x55)
}

@Test("read/ write multiple 8 bit IO ports bank at 64 bit addresses")
func readWriteMultipleMemoryMappedIoPorts64() {
    let portA = Port<Byte>()
    let portB = Port<Byte>()
    let portC = Port<Byte>()
    let bank = IOPortBank<QWord, Port<Byte>>()
    bank.register(port: portA, at: 66)
    bank.register(port: portB, at: 45)
    bank.register(port: portC, at: 62)
    #expect(bank[0] == 0)
    #expect(bank[QWord(QWord.bankSize - 1)] == 0)
    bank[66] = 0xAA
    bank[45] = 0x42
    bank[62] = 0x55
    #expect(bank[66] == 0xAA)
    #expect(bank[45] == 0x42)
    #expect(bank[62] == 0x55)
}

@Test("read/ write 8 bit data from/ to RAM/ ROM/ memory mapped IO ports at 32 bit addresses")
func readWriteRamRomMemoryMappedIoPorts() {
    let memory = MemoryIO<DWord>(defaultBank: DefaultBank<DWord>())
    let ram = RAMBank<DWord>()
    let rom = ROMBank<DWord>(bytes: [0xDE, 0xED, 0xC0, 0xDE])
    let bank = IOPortBank<DWord, Port<Byte>>()
    memory.register(bank: ram, at: 0x0000_0000)
    memory.register(bank: rom, at: 0x0001_0000)
    memory.register(bank: bank, at: 0x0002_0000)
    let portA = Port<Byte>()
    let portB = Port<Byte>()
    let portC = Port<Byte>()
    bank.register(port: portA, at: 0x66)
    bank.register(port: portB, at: 0x45)
    bank.register(port: portC, at: 0x62)
    memory[0x0000_0000] = 0x12  // RAM
    memory[0x0000_0001] = 0x34
    #expect(memory[0x0000_0000] == 0x12)
    #expect(memory[0x0000_0001] == 0x34)
    memory[0x0001_0000] = 0x12  // ROM
    memory[0x0001_0001] = 0x34
    memory[0x0001_0002] = 0x56
    memory[0x0001_0003] = 0x78
    #expect(memory[0x0001_0000] == 0xDE)
    #expect(memory[0x0001_0001] == 0xED)
    #expect(memory[0x0001_0002] == 0xC0)
    #expect(memory[0x0001_0003] == 0xDE)
    memory[0x0002_0066] = 0xAA  // memory mapped IO
    memory[0x0002_0045] = 0x42
    memory[0x0002_0062] = 0x55
    #expect(memory[0x0002_0066] == 0xAA)
    #expect(memory[0x0002_0045] == 0x42)
    #expect(memory[0x0002_0062] == 0x55)
}

@Test("read/ write 8 bit data from/ to RAM/ ROM/ memory mapped IO ports at 64 bit addresses")
func readWriteRamRomMemoryMappedIoPorts64() {
    let memory = MemoryIO<QWord>(defaultBank: DefaultBank<QWord>())
    let ram = RAMBank<QWord>()
    let rom = ROMBank<QWord>(bytes: [0xDE, 0xED, 0xC0, 0xDE])
    let bank = IOPortBank<QWord, Port<Byte>>()
    memory.register(bank: ram, at: 0x0000_0000)
    memory.register(bank: rom, at: 0x0100_0000)
    memory.register(bank: bank, at: 0x0200_0000)
    let portA = Port<Byte>()
    let portB = Port<Byte>()
    let portC = Port<Byte>()
    bank.register(port: portA, at: 0x66)
    bank.register(port: portB, at: 0x45)
    bank.register(port: portC, at: 0x62)
    memory[0x0000_0000] = 0x12  // RAM
    memory[0x0000_0001] = 0x34
    #expect(memory[0x0000_0000] == 0x12)
    #expect(memory[0x0000_0001] == 0x34)
    memory[0x0100_0000] = 0x12  // ROM
    memory[0x0100_0001] = 0x34
    memory[0x0100_0002] = 0x56
    memory[0x0100_0003] = 0x78
    #expect(memory[0x0100_0000] == 0xDE)
    #expect(memory[0x0100_0001] == 0xED)
    #expect(memory[0x0100_0002] == 0xC0)
    #expect(memory[0x0100_0003] == 0xDE)
    memory[0x0200_0066] = 0xAA  // memory mapped IO
    memory[0x0200_0045] = 0x42
    memory[0x0200_0062] = 0x55
    #expect(memory[0x0200_0066] == 0xAA)
    #expect(memory[0x0200_0045] == 0x42)
    #expect(memory[0x0200_0062] == 0x55)
}
