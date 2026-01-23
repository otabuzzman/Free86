import Testing
@testable import Free86

@Test("free86 reset function")
func free86ResetFunction() {
    let memory = MemoryIO<DWord>(defaultBank: DefaultBank<DWord>())
    for i in 0..<512 {  // 2MB (512 * 4kB)
        memory.register(bank: RAMBank<DWord>(), at: DWord(i * 4096))
    }
    let free86 = Free86(memory: memory)

    /// state properties set by reset() intentionally declared as implicitly-unwrapped optionals
    #expect(free86.regs.count == 8)
    #expect(free86.eflags == 0x2)

    #expect(free86.eip == 0xFFF0)

    #expect(free86.segs.count == 7)
    #expect(free86.segs[.CS].descriptorCache.base == 0xFFFF_0000)

    #expect(free86.idt.descriptorCache.base == 0x03FF)

    #expect(free86.cr0.isFlagRaised(.ET) == true)
    #expect(free86.halted == false)
}
