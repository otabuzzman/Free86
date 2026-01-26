import Testing
@testable import Free86

@Test("free86 segments state block")
func free86SegmentsStateBlock() {
    let memory = MemoryIO<DWord>(defaultBank: DefaultBank<DWord>())
    for i in 0..<512 {  // 2MB (512 * 4kB)
        memory.register(bank: RAMBank<DWord>(), at: DWord(i * 4096))
    }
    let free86 = Free86(memory: memory)

    var cd: SegmentDescriptor = .init(0x0000FFFF, 0, .CodeExOnly, 0)
    var sd: SegmentDescriptor = .init(0x00FFFF00, 0, .CodeExOnly, 0)
    let dd: SegmentDescriptor = .init(0xFFFF0000, 0, .CodeExOnly, 0)
    let ed: SegmentDescriptor = .init(0xFF0000FF, 0, .CodeExOnly, 0)
    free86.segs[.CS] = .init(0, cd)
    free86.segs[.SS] = .init(0, sd)
    free86.segs[.DS] = .init(0, dd)
    free86.segs[.ES] = .init(0, ed)

    #expect(free86.csBase == 0x0000FFFF)
    #expect(free86.ssBase == 0x00FFFF00)

    #expect(free86.ssMask == 0xFFFF)
    sd.setFlag(.D)
    free86.segs[.SS] = SegmentRegister(0, sd)
    #expect(free86.ssMask == 0xFFFFFFFF)

    #expect(free86.iprDefault == 0)
    cd.setFlag(.D)
    free86.segs[.CS] = SegmentRegister(0, cd)
    #expect(free86.iprDefault.isFlagRaised(.addressSizeOverride) && free86.iprDefault.isFlagRaised(.operandSizeOverride))
    cd.setFlag(.D, 0)
    free86.segs[.CS] = SegmentRegister(0, cd)
    #expect(free86.iprDefault == 0)

    #expect(free86.x8664LongMode == false)  // true if all bases 0 and 32 bit stack segment
    free86.segs[.CS] = .init(0, .init(0))
    sd = .init(0)
    sd.setFlag(.D)
    free86.segs[.SS] = .init(0, sd)
    free86.segs[.DS] = .init(0, .init(0))
    free86.segs[.ES] = .init(0, .init(0))
    #expect(free86.x8664LongMode == true)
}
