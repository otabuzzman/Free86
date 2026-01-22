import Testing
@testable import Free86

@Test("free86 translation lookaside buffer")
func free86TranslationLookasideBuffer() {
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
