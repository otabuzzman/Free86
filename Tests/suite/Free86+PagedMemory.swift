import Testing
@testable import Free86

@Test("free86 paged memory access")
func free86PagedMemoryAccess() {
    let memory = MemoryIO<DWord>(defaultBank: DefaultBank<DWord>())
    for i in 0..<512 {  // 2MB (512 * 4kB)
        memory.register(bank: RAMBank<DWord>(), at: DWord(i * 4096))
    }
    let free86 = Free86(memory: memory)

    /// set up the PD with all entries 0
    let pageDirAddr: DWord = 0x1000
    let pageTblAddr = pageDirAddr + 0x1000
    /// PD setup
    var pde = pageTblAddr  // PDE for PT
    pde.setFlag(.P)
    pde.setFlag(.W)
    pde.setFlag(.U)
    memory.st(at: pageDirAddr, dword: pde)
    for e: DWord in 1..<1024 {  // set remaining PDEs to 0
        memory.st(at: pageDirAddr + e * 4, dword: 0)
    }
    /// start paging
    free86.cr3 = pageDirAddr
    free86.cr0.setFlag(.PE)
    free86.cr0.setFlag(.PG)

    /// test case: access arbitrary linear (0x4711)
    let linear: LinearAddress = 0x4711  // no matching PTE for linear 0...0xFFFFF, thus PF
    #expect(throws: Interrupt(14, errorCode: 7)) {
        try free86.translate(linear, writable: true, user: true)
    }
    #expect(free86.cr2 == linear)  // linear cousing PF in CR2
    /// PTE for missing linear
    var pte: PageTableEntry = 0x100000  // map linear 0...0xFFFFF to physical 0x100000...0x1FFFFF
    pte.setFlag(.P)
    pte.setFlag(.W)
    pte.setFlag(.U)
    let o: DWord = linear.pageTableOffset
    let i: DWord = linear.pageTableOffset >> 2
    memory.st(at: pde.pageFrameAddress + o, dword: pte + i * 0x1000)
    /// repeat test case
    #expect(throws: Never.self) {  // now mapped, thus no exception
        try free86.translate(linear, writable: true, user: true)
    }
    free86.lax = linear  // paged memory functions expect linear in `lax`
    try! free86.stWritableCpl3(dword: 0xDEADBEEF)     // store linear
    #expect(memory.ld(from: 0x104711) == 0xDEADBEEF)  // load from physical
    #expect(free86.lax == linear)

    /// store physical
    memory[0x104711] = 0xDE
    memory[0x104712] = 0xC0
    memory[0x104713] = 0xAD
    memory[0x104714] = 0xDE
    memory[0x104715] = 0xFE
    memory[0x104716] = 0xCA
    memory[0x104717] = 0xEF
    memory[0x104718] = 0xBE
    /// load byte linear
    #expect(try! free86.ld8WritableCpl3() == 0xDE)
    free86.lax += 1
    #expect(try! free86.ld8WritableCpl3() == 0xC0)
    free86.lax += 1
    #expect(try! free86.ld8WritableCpl3() == 0xAD)
    free86.lax += 1
    #expect(try! free86.ld8WritableCpl3() == 0xDE)
    free86.lax += 1
    #expect(try! free86.ld8WritableCpl3() == 0xFE)
    free86.lax += 1
    #expect(try! free86.ld8WritableCpl3() == 0xCA)
    free86.lax += 1
    #expect(try! free86.ld8WritableCpl3() == 0xEF)
    free86.lax += 1
    #expect(try! free86.ld8WritableCpl3() == 0xBE)
    free86.lax -= 7
    /// load word linear
    #expect(try! free86.ld16WritableCpl3() == 0xC0DE)
    free86.lax += 2
    #expect(try! free86.ld16WritableCpl3() == 0xDEAD)
    free86.lax += 2
    #expect(try! free86.ld16WritableCpl3() == 0xCAFE)
    free86.lax += 2
    #expect(try! free86.ld16WritableCpl3() == 0xBEEF)
    free86.lax -= 6
    /// load dword linear
    #expect(try! free86.ldWritableCpl3() == 0xDEADC0DE)
    free86.lax += 4
    #expect(try! free86.ldWritableCpl3() == 0xBEEFCAFE)
    free86.lax -= 4
    /// load qword linear
    #expect(try! free86.ld64ReadonlyCplX() == 0xBEEFCAFE_DEADC0DE)
    #expect(free86.lax == linear)

    /// store byte linear/ load physical
    free86.lax += 0x10
    try! free86.st8WritableCpl3(byte: 0xDE)
    free86.lax += 1
    try! free86.st8WritableCpl3(byte: 0xC0)
    free86.lax += 1
    try! free86.st8WritableCpl3(byte: 0xAD)
    free86.lax += 1
    try! free86.st8WritableCpl3(byte: 0xDE)
    free86.lax += 1
    try! free86.st8WritableCpl3(byte: 0xFE)
    free86.lax += 1
    try! free86.st8WritableCpl3(byte: 0xCA)
    free86.lax += 1
    try! free86.st8WritableCpl3(byte: 0xEF)
    free86.lax += 1
    try! free86.st8WritableCpl3(byte: 0xBE)
    free86.lax -= 7
    #expect(memory.ld8(from: 0x104721) == 0xDE)
    #expect(memory.ld8(from: 0x104722) == 0xC0)
    #expect(memory.ld8(from: 0x104723) == 0xAD)
    #expect(memory.ld8(from: 0x104724) == 0xDE)
    #expect(memory.ld8(from: 0x104725) == 0xFE)
    #expect(memory.ld8(from: 0x104726) == 0xCA)
    #expect(memory.ld8(from: 0x104727) == 0xEF)
    #expect(memory.ld8(from: 0x104728) == 0xBE)
    /// store word linear/ load physical
    free86.lax += 0x10
    try! free86.st16WritableCpl3(word: 0xC0DE)
    free86.lax += 2
    try! free86.st16WritableCpl3(word: 0xDEAD)
    free86.lax += 2
    try! free86.st16WritableCpl3(word: 0xCAFE)
    free86.lax += 2
    try! free86.st16WritableCpl3(word: 0xBEEF)
    free86.lax -= 6
    #expect(memory.ld16(from: 0x104731) == 0xC0DE)
    #expect(memory.ld16(from: 0x104733) == 0xDEAD)
    #expect(memory.ld16(from: 0x104735) == 0xCAFE)
    #expect(memory.ld16(from: 0x104737) == 0xBEEF)
    /// store dword linear/ load physical
    free86.lax += 0x10
    try! free86.stWritableCpl3(dword: 0xDEADC0DE)
    free86.lax += 4
    try! free86.stWritableCpl3(dword: 0xBEEFCAFE)
    free86.lax -= 4
    #expect(memory.ld(from: 0x104741) == 0xDEADC0DE)
    #expect(memory.ld(from: 0x104745) == 0xBEEFCAFE)
    /// store qword linear/ load physical
    free86.lax += 0x10
    try! free86.st64WritableCplX(qword: 0xBEEFCAFE_DEADC0DE)
    #expect(try! free86.ld64ReadonlyCplX() == 0xBEEFCAFE_DEADC0DE)
    #expect(free86.lax == linear + 0x40)
}
