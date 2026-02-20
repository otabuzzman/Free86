import Testing
@testable import Free86

@Test("CR0 setter")
func free86Cr0Setter() {
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
    }
    #expect(free86.tlbPagesCount == 512)
    var cr0: CR0 = 0
    free86.cr0 = cr0
    #expect(free86.cr0.isFlagRaised(.ET) == true)  // default
    #expect(free86.tlbPagesCount == 512)
    cr0.setFlag(.PE)  // must flush TLB
    free86.cr0 = cr0  // assign
    #expect(free86.cr0.isFlagRaised(.PE) == true)
    #expect(free86.tlbPagesCount == 0)  // TLB flushed

    /// refill TLB
    for i in 0..<512 {
        let linear = LinearAddress((i + 0x1000) << 12)
        let address = ~linear >> 12
        free86.tlbUpdate(linear: linear, with: address, writable: true, user: true)
    }
    #expect(free86.tlbPagesCount == 512)
    free86.cr0.setFlag(.WP)  // set, same as assign, must flush TLB
    #expect(free86.cr0.isFlagRaised(.PE) == true)
    #expect(free86.cr0.isFlagRaised(.WP) == true)
    #expect(free86.tlbPagesCount == 0)  // TLB flushed

    /// refill TLB
    for i in 0..<512 {
        let linear = LinearAddress((i + 0x1000) << 12)
        let address = ~linear >> 12
        free86.tlbUpdate(linear: linear, with: address, writable: true, user: true)
    }
    #expect(free86.tlbPagesCount == 512)
    free86.cr0.setFlag(.PG)  // must flush TLB
    #expect(free86.cr0.isFlagRaised(.PE) == true)
    #expect(free86.cr0.isFlagRaised(.WP) == true)
    #expect(free86.cr0.isFlagRaised(.PG) == true)
    #expect(free86.tlbPagesCount == 0)  // TLB flushed

    /// refill TLB
    for i in 0..<512 {
        let linear = LinearAddress((i + 0x1000) << 12)
        let address = ~linear >> 12
        free86.tlbUpdate(linear: linear, with: address, writable: true, user: true)
    }
    #expect(free86.tlbPagesCount == 512)
    free86.cr0.setFlag(.TS)  // must not flush TLB
    #expect(free86.cr0.isFlagRaised(.PE) == true)
    #expect(free86.cr0.isFlagRaised(.WP) == true)
    #expect(free86.cr0.isFlagRaised(.PG) == true)
    #expect(free86.tlbPagesCount == 512)  // TLB not flushed
}

@Test("CR3 setter")
func free86Cr3Setter() {
    let memory = MemoryIO<DWord>(defaultBank: DefaultBank<DWord>())
    let ramA = RAMBank<DWord>()
    let ramO = RAMBank<DWord>()
    memory.register(bank: ramA, at: 0x0000_0000)
    memory.register(bank: ramO, at: 0x0010_0000 - 4096)  // Free86 memory requirement
    let free86 = Free86(memory: memory)

    /// populate TLB...
    free86.cr0.setFlag(.PG)
    free86.cr0.setFlag(.PE)
    #expect(free86.cr0.isPagingEnabled == true)
    for i in 0..<512 {
        let linear = LinearAddress((i + 0x1000) << 12)
        let address = ~linear >> 12
        free86.tlbUpdate(linear: linear, with: address, writable: true, user: true)
    }
    #expect(free86.tlbPagesCount == 512)
    free86.cr3 = 0x47110000
    #expect(free86.tlbPagesCount == 0)  // TLB flushed

    /// refill TLB
    free86.cr0 = 0
    #expect(free86.cr0.isPagingEnabled == false)
    for i in 0..<512 {
        let linear = LinearAddress((i + 0x1000) << 12)
        let address = ~linear >> 12
        free86.tlbUpdate(linear: linear, with: address, writable: true, user: true)
    }
    #expect(free86.tlbPagesCount == 512)
    free86.cr3 = 0xBEEF0000
    #expect(free86.tlbPagesCount == 512)  // TLB not flushed
}

@Test("CPL setter")
func free86CplSetter() {
    let memory = MemoryIO<DWord>(defaultBank: DefaultBank<DWord>())
    let ramA = RAMBank<DWord>()
    let ramO = RAMBank<DWord>()
    memory.register(bank: ramA, at: 0x0000_0000)
    memory.register(bank: ramO, at: 0x0010_0000 - 4096)  // Free86 memory requirement
    let free86 = Free86(memory: memory)
    #expect(free86.tlbReadonly == free86.tlbReadonlyCplX)
    #expect(free86.tlbWritable == free86.tlbWritableCplX)

    free86.cpl = 3
    #expect(free86.tlbReadonly == free86.tlbReadonlyCpl3)
    #expect(free86.tlbWritable == free86.tlbWritableCpl3)
    free86.cpl = 0
    #expect(free86.tlbReadonly == free86.tlbReadonlyCplX)
    #expect(free86.tlbWritable == free86.tlbWritableCplX)
    free86.cpl = 1
    #expect(free86.tlbReadonly == free86.tlbReadonlyCplX)
    #expect(free86.tlbWritable == free86.tlbWritableCplX)
    free86.cpl = 2
    #expect(free86.tlbReadonly == free86.tlbReadonlyCplX)
    #expect(free86.tlbWritable == free86.tlbWritableCplX)
    free86.cpl = 3
    #expect(free86.tlbReadonly == free86.tlbReadonlyCpl3)
    #expect(free86.tlbWritable == free86.tlbWritableCpl3)
}
