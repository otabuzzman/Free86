import Testing
@testable import Free86

@Test("direct memory access")
func directMemoryAccess() {
    let memory = MemoryIO<DWord>(defaultBank: DefaultBank<DWord>())
    let ram = RAMBank<DWord>()
    memory.register(bank: ram, at: 0x0000_0000)

    memory[0x0123] = 0xDE
    memory[0x0124] = 0xC0
    memory[0x0125] = 0xAD
    memory[0x0126] = 0xDE
    /// byte access
    #expect(memory.ld8Direct(from: 0x0123) == 0xDE)
    #expect(memory.ld8Direct(from: 0x0124) == 0xC0)
    #expect(memory.ld8Direct(from: 0x0125) == 0xAD)
    #expect(memory.ld8Direct(from: 0x0126) == 0xDE)
    /// word access
    #expect(memory.ld16Direct(from: 0x0123) == 0xC0DE)
    #expect(memory.ld16Direct(from: 0x0125) == 0xDEAD)
    /// dword access
    #expect(memory.ldDirect(from: 0x0123) == 0xDEADC0DE)

    memory.st8Direct(at: 0x0143, byte: 0xFE)
    memory.st8Direct(at: 0x0144, byte: 0xCA)
    memory.st8Direct(at: 0x0145, byte: 0xEF)
    memory.st8Direct(at: 0x0146, byte: 0xBE)
    /// byte access
    #expect(memory[0x0143] == 0xFE)
    #expect(memory[0x0144] == 0xCA)
    #expect(memory[0x0145] == 0xEF)
    #expect(memory[0x0146] == 0xBE)
    /// word access
    memory.st16Direct(at: 0x0143, word: 0xCAFE)
    memory.st16Direct(at: 0x0145, word: 0xBEEF)
    #expect(memory[0x0143] == 0xFE)
    #expect(memory[0x0144] == 0xCA)
    #expect(memory[0x0145] == 0xEF)
    #expect(memory[0x0146] == 0xBE)
    /// dword access
    memory.stDirect(at: 0x0143, dword: 0xBEEFCAFE)
    #expect(memory[0x0143] == 0xFE)
    #expect(memory[0x0144] == 0xCA)
    #expect(memory[0x0145] == 0xEF)
    #expect(memory[0x0146] == 0xBE)
}
