import Testing
@testable import Free86

@Test("TLB update/ flush")
func free86TLBUpdateFlush() {
    let memory = MemoryIO<DWord>(defaultBank: DefaultBank<DWord>())
    let ramA = RAMBank<DWord>()
    let ramO = RAMBank<DWord>()
    memory.register(bank: ramA, at: 0x0000_0000)
    memory.register(bank: ramO, at: 0x0010_0000 - 4096)  // Free86 memory requirement
    let free86 = Free86(memory: memory)
    
    /// populate TLB...
    for i in 0..<512 {
        let linear = LinearAddress((i + 0x1000) << 12)
        let address = ~linear >> 12
        free86.tlbUpdate(linear: linear, with: address, writable: true, user: true)
        
        var hash = free86.tlbReadonlyCplX[linear.pageTablesIndices]  // always
        #expect(linear ^ DWord(hash) == address)
        hash = free86.tlbWritableCplX[linear.pageTablesIndices]  // writable user/ Supervisor
        #expect(linear ^ DWord(hash) == address)
        hash = free86.tlbReadonlyCpl3[linear.pageTablesIndices]  // user
        #expect(linear ^ DWord(hash) == address)
        hash = free86.tlbWritableCpl3[linear.pageTablesIndices]  // writable user
        #expect(linear ^ DWord(hash) == address)
    }
    #expect(free86.tlbPagesCount == 512)
    /// more entries with different writable/ user arguments...
    for i in 512..<1024 {
        let linear = LinearAddress((i + 0x1000) << 12)
        let address = ~linear >> 12
        free86.tlbUpdate(linear: linear, with: address, writable: true, user: false)
        
        var hash = free86.tlbReadonlyCplX[linear.pageTablesIndices]  // always
        #expect(linear ^ DWord(hash) == address)
        hash = free86.tlbWritableCplX[linear.pageTablesIndices]  // writable user/ Supervisor
        #expect(linear ^ DWord(hash) == address)
        hash = free86.tlbReadonlyCpl3[linear.pageTablesIndices]  // user
        #expect(hash == -1)
        hash = free86.tlbWritableCpl3[linear.pageTablesIndices]  // writable user
        #expect(hash == -1)
    }
    #expect(free86.tlbPagesCount == 1024)
    /// more entries with different writable/ user arguments...
    for i in 1024..<1536 {
        let linear = LinearAddress((i + 0x1000) << 12)
        let address = ~linear >> 12
        free86.tlbUpdate(linear: linear, with: address, writable: false, user: true)
        
        var hash = free86.tlbReadonlyCplX[linear.pageTablesIndices]  // always
        #expect(linear ^ DWord(hash) == address)
        hash = free86.tlbWritableCplX[linear.pageTablesIndices]  // writable user/ Supervisor
        #expect(hash == -1)
        hash = free86.tlbReadonlyCpl3[linear.pageTablesIndices]  // user
        #expect(linear ^ DWord(hash) == address)
        hash = free86.tlbWritableCpl3[linear.pageTablesIndices]  // writable user
        #expect(hash == -1)
    }
    #expect(free86.tlbPagesCount == 1536)
    /// more entries with different writable/ user arguments...
    for i in 1536..<2048 {
        let linear = LinearAddress((i + 0x1000) << 12)
        let address = ~linear >> 12
        free86.tlbUpdate(linear: linear, with: address, writable: false, user: false)
        
        var hash = free86.tlbReadonlyCplX[linear.pageTablesIndices]  // always
        #expect(linear ^ DWord(hash) == address)
        hash = free86.tlbWritableCplX[linear.pageTablesIndices]  // writable user/ Supervisor
        #expect(hash == -1)
        hash = free86.tlbReadonlyCpl3[linear.pageTablesIndices]  // user
        #expect(hash == -1)
        hash = free86.tlbWritableCpl3[linear.pageTablesIndices]  // writable user
        #expect(hash == -1)
    }
    #expect(free86.tlbPagesCount == 2048)
    
    /// adding one more entry forces TLB flush, preserving last entry at index 0 because consecutive new entry
    let last = free86.tlbPages.last
    let linear = LinearAddress((2048 + 0x1000) << 12)
    let address = ~linear >> 12
    free86.tlbUpdate(linear: linear, with: address, writable: false, user: false)
    #expect(free86.tlbPagesCount == 2)
    #expect(free86.tlbPages[0] == last)
    
    free86.tlbFlush()
    #expect(free86.tlbPagesCount == 0)
}

@Test("page translation")
func free86PageTranslation() {
    let memory = MemoryIO<DWord>(defaultBank: DefaultBank<DWord>())
    for i in 0..<512 {  // 2MB (512 * 4kB)
        memory.register(bank: RAMBank<DWord>(), at: DWord(i * 4096))
    }
    let free86 = Free86(memory: memory)

    /// set up the PD with a single entry at index 0 refering to a PT
    /// which maps 1 MB starting at linear 0 to physical 0x100000
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
    /// PT setup
    var pte: PageTableEntry = 0x100000  // PTE for 1st memory page at 0x100000
    pte.setFlag(.P)
    pte.setFlag(.W)
    pte.setFlag(.U)
    for e: DWord in 0..<256 {  // 256 PTEs map linear 0...0xFFFFF to physical 0x100000...0x1FFFFF
        memory.st(at: pageTblAddr + e * 4, dword: pte + e * 0x1000)
    }
    for e: DWord in 256..<1024 {  // set remaining PTEs to 0
        memory.st(at: pageTblAddr + e * 4, dword: 0)
    }
    /// start paging
    free86.cr3 = pageDirAddr
    free86.cr0.setFlag(.PE)
    free86.cr0.setFlag(.PG)

    /// test case: access 1st linear (0)
    var linear: LinearAddress = 0
    #expect(throws: Never.self) {  // mapped, thus no exception
        try free86.translate(linear, writable: true, user: true)
    }
    /// get hash for linear from TLB, has been added by translate
    var hash = free86.tlbReadonlyCplX[linear.pageTablesIndices]
    /// unhash yields 1st physical (0x100000)
    #expect(linear ^ DWord(hash) == 0x100000)

    /// test case: access arbitrary linear (0x4711)
    linear = 0x4711
    #expect(throws: Never.self) {
        try free86.translate(linear, writable: true, user: true)
    }
    hash = free86.tlbReadonlyCplX[linear.pageTablesIndices]
    #expect(linear ^ DWord(hash) == 0x4711 + 0x100000)

    /// test case: access last linear (0xFFFFF)
    linear = 0xFFFFF
    #expect(throws: Never.self) {
        try free86.translate(linear, writable: true, user: true)
    }
    hash = free86.tlbReadonlyCplX[linear.pageTablesIndices]
    #expect(linear ^ DWord(hash) == 0xFFFFF + 0x100000)
}
