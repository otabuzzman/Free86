import Testing
@testable import Free86

@Test("reset function")
func free86ResetFunction() {
    let memory = MemoryIO<DWord>(defaultBank: DefaultBank<DWord>())
    for i in 0..<512 {  // 2MB (512 * 4kB)
        memory.register(bank: RAMBank<DWord>(), at: DWord(i * 4096))
    }
    let free86 = Free86(memory: memory)

    #expect(free86.regs.count == 8)
    #expect(free86.eflags == 0x2)

    #expect(free86.eip == 0xFFF0)

    #expect(free86.segs.count == 7)
    #expect(free86.segs[.CS] == SegmentRegister(0, .init(0xFFFF0000, 0, .CodeExOnly, 0)))

    #expect(free86.idt == SegmentRegister(0, .init(0x03FF, 0, .zero, 0)))

    #expect(free86.cr0.isFlagRaised(.ET) == true)
    #expect(free86.halted == false)
}
