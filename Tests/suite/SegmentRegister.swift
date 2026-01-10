import Testing
@testable import Free86

@Test("segment register bank")
func segmentRegisterBank() {
    let bank = [
        SegmentRegister(selector: 0xC0DE, descriptorCache: SegmentDescriptor(upper: 0, lower: 7)),
        SegmentRegister(selector: 0xBEAF, descriptorCache: SegmentDescriptor(upper: 1, lower: 6)),
        SegmentRegister(selector: 0xC0DE, descriptorCache: SegmentDescriptor(upper: 2, lower: 5)),
        SegmentRegister(selector: 0xDEAD, descriptorCache: SegmentDescriptor(upper: 3, lower: 4)),
        SegmentRegister(selector: 0xADC0, descriptorCache: SegmentDescriptor(upper: 4, lower: 3)),
        SegmentRegister(selector: 0xDEDE, descriptorCache: SegmentDescriptor(upper: 5, lower: 2)),
        SegmentRegister(selector: 0xDEAD, descriptorCache: SegmentDescriptor(upper: 6, lower: 1)),
        SegmentRegister(selector: 0xDEC0, descriptorCache: SegmentDescriptor(upper: 7, lower: 0)),
    ]
    #expect(bank[.ES] == SegmentRegister(selector: 0xC0DE, descriptorCache: SegmentDescriptor(upper: 0, lower: 7)))
    #expect(bank[.CS] == SegmentRegister(selector: 0xBEAF, descriptorCache: SegmentDescriptor(upper: 1, lower: 6)))
    #expect(bank[.SS] == SegmentRegister(selector: 0xC0DE, descriptorCache: SegmentDescriptor(upper: 2, lower: 5)))
    #expect(bank[.DS] == SegmentRegister(selector: 0xDEAD, descriptorCache: SegmentDescriptor(upper: 3, lower: 4)))
    #expect(bank[.FS] == SegmentRegister(selector: 0xADC0, descriptorCache: SegmentDescriptor(upper: 4, lower: 3)))
    #expect(bank[.GS] == SegmentRegister(selector: 0xDEDE, descriptorCache: SegmentDescriptor(upper: 5, lower: 2)))
    #expect(bank[.LDT] == SegmentRegister(selector: 0xDEAD, descriptorCache: SegmentDescriptor(upper: 6, lower: 1)))
    #expect(bank[.TR] == SegmentRegister(selector: 0xDEC0, descriptorCache: SegmentDescriptor(upper: 7, lower: 0)))
}
