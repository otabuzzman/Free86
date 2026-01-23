import Testing
@testable import Free86

@Test("free86 paged memory access")
func free86PagedMemoryAccess() {
    let memory = MemoryIO<DWord>(defaultBank: DefaultBank<DWord>())
    for i in 0..<512 {  // 2MB (512 * 4kB)
        memory.register(bank: RAMBank<DWord>(), at: DWord(i * 4096))
    }
    let free86 = Free86(memory: memory)

    let pageDirAddr: DWord = 0x1000
    let pageTblAddr = pageDirAddr + 0x1000
    /// PD
    var pde = pageTblAddr  // PDE for PT
    pde.setFlag(.P)
    pde.setFlag(.W)
    pde.setFlag(.U)
    free86.st(at: pageDirAddr, dword: pde)
    for e: DWord in 1..<1024 {  // set remaining PDEs to 0
        free86.st(at: pageDirAddr + e * 4, dword: 0)
    }
    /// enable paging
    free86.cr3 = pageDirAddr
    free86.cr0.setFlag(.PE)
    free86.cr0.setFlag(.PG)
    /// trigger page fault
    let linear: LinearAddress = 0x4711  // no matching PTE for linear 0...0xFFFFF, thus PF
    #expect(throws: Interrupt(14, errorCode: 7)) {
        try free86.translate(linear, writable: true, user: true)
    }
    #expect(free86.cr2 == linear)
    /// add missing PTE
    var pte: PageTableEntry = 0x100000
    pte.setFlag(.P)
    pte.setFlag(.W)
    pte.setFlag(.U)
    let offset: DWord = linear.pageTableOffset
    let index = offset >> 2
    pte += index * 0x1000
    free86.st(at: pde.pageFrameAddress + offset, dword: pte)
    /// repeat access
    #expect(throws: Never.self) {
        try free86.translate(linear, writable: true, user: true)
    }
    free86.lax = linear
    try! free86.stWritableCpl3(dword: 0xDEADBEEF)
    #expect(free86.ld(from: 0x104711) == 0xDEADBEEF)
}
