import Testing
@testable import Free86

@Test("free86 direct memory access")
func free86DirectMemoryAccess() {
    let memory = MemoryIO<DWord>(defaultBank: DefaultBank<DWord>())
    let ramA = RAMBank<DWord>()
    let ramO = RAMBank<DWord>()
    memory.register(bank: ramA, at: 0x0000_0000)
    memory.register(bank: ramO, at: 0x0010_0000 - 4096)  // Free86 memory requirement
    let free86 = Free86(memory: memory)

    memory[0x0123] = 0xDE
    memory[0x0124] = 0xC0
    memory[0x0125] = 0xAD
    memory[0x0126] = 0xDE
    /// load byte
    #expect(free86.ld8(from: 0x0123) == 0xDE)
    #expect(free86.ld8(from: 0x0124) == 0xC0)
    #expect(free86.ld8(from: 0x0125) == 0xAD)
    #expect(free86.ld8(from: 0x0126) == 0xDE)
    /// load word
    #expect(free86.ld16(from: 0x0123) == 0xC0DE)
    #expect(free86.ld16(from: 0x0125) == 0xDEAD)
    /// load dword
    #expect(free86.ld(from: 0x0123) == 0xDEADC0DE)

    free86.st8(at: 0x0143, byte: 0xFE)
    free86.st8(at: 0x0144, byte: 0xCA)
    free86.st8(at: 0x0145, byte: 0xEF)
    free86.st8(at: 0x0146, byte: 0xBE)
    /// store byte
    #expect(memory[0x0143] == 0xFE)
    #expect(memory[0x0144] == 0xCA)
    #expect(memory[0x0145] == 0xEF)
    #expect(memory[0x0146] == 0xBE)
    /// store word
    free86.st16(at: 0x0143, word: 0xCAFE)
    free86.st16(at: 0x0145, word: 0xBEEF)
    #expect(memory[0x0143] == 0xFE)
    #expect(memory[0x0144] == 0xCA)
    #expect(memory[0x0145] == 0xEF)
    #expect(memory[0x0146] == 0xBE)
    /// store dword
    free86.st(at: 0x0143, dword: 0xBEEFCAFE)
    #expect(memory[0x0143] == 0xFE)
    #expect(memory[0x0144] == 0xCA)
    #expect(memory[0x0145] == 0xEF)
    #expect(memory[0x0146] == 0xBE)
}
