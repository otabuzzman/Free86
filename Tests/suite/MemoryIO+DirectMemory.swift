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
    memory[0x0127] = 0xFE
    memory[0x0128] = 0xCA
    memory[0x0129] = 0xEF
    memory[0x012A] = 0xBE
    /// load byte
    #expect(memory.ld8(from: 0x0123) == 0xDE)
    #expect(memory.ld8(from: 0x0124) == 0xC0)
    #expect(memory.ld8(from: 0x0125) == 0xAD)
    #expect(memory.ld8(from: 0x0126) == 0xDE)
    /// load word
    #expect(memory.ld16(from: 0x0123) == 0xC0DE)
    #expect(memory.ld16(from: 0x0125) == 0xDEAD)
    /// load dword
    #expect(memory.ld(from: 0x0123) == 0xDEADC0DE)
    /// load qword
    #expect(memory.ld64(from: 0x0123) == 0xBEEFCAFE_DEADC0DE)

    memory.st8(at: 0x0143, byte: 0xFE)
    memory.st8(at: 0x0144, byte: 0xCA)
    memory.st8(at: 0x0145, byte: 0xEF)
    memory.st8(at: 0x0146, byte: 0xBE)
    /// store byte
    #expect(memory[0x0143] == 0xFE)
    #expect(memory[0x0144] == 0xCA)
    #expect(memory[0x0145] == 0xEF)
    #expect(memory[0x0146] == 0xBE)
    /// store word
    memory.st16(at: 0x0143, word: 0xCAFE)
    memory.st16(at: 0x0145, word: 0xBEEF)
    #expect(memory[0x0143] == 0xFE)
    #expect(memory[0x0144] == 0xCA)
    #expect(memory[0x0145] == 0xEF)
    #expect(memory[0x0146] == 0xBE)
    /// store dword
    memory.st(at: 0x0143, dword: 0xBEEFCAFE)
    #expect(memory[0x0143] == 0xFE)
    #expect(memory[0x0144] == 0xCA)
    #expect(memory[0x0145] == 0xEF)
    #expect(memory[0x0146] == 0xBE)
    memory.st(at: 0x0143, dword: 0xBEEFCAFE)
    #expect(memory[0x0143] == 0xFE)
    #expect(memory[0x0144] == 0xCA)
    #expect(memory[0x0145] == 0xEF)
    #expect(memory[0x0146] == 0xBE)
    /// store qword
    memory.st64(at: 0x0143, qword: 0xDEADC0DE_BEEFCAFE)
    #expect(memory[0x0143] == 0xFE)
    #expect(memory[0x0144] == 0xCA)
    #expect(memory[0x0145] == 0xEF)
    #expect(memory[0x0146] == 0xBE)
    #expect(memory[0x0147] == 0xDE)
    #expect(memory[0x0148] == 0xC0)
    #expect(memory[0x0149] == 0xAD)
    #expect(memory[0x014A] == 0xDE)
}
