import Testing
@testable import Free86

@Test("segment descriptor base/ limit values")
func segmentDescriptorBaseLimit() {
    var segmentDescriptor = SegmentDescriptor(upper: 0x5505_00AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.base == 0x55AA_55AA)
    #expect(segmentDescriptor.limit == 0x5_AA55)

    segmentDescriptor.upper.toggleBit(SegmentDescriptor.UpperBits.g)
    #expect(segmentDescriptor.limit == 0x5AA5_5FFF)
}

@Test("segment descriptor type/ dpl masks")
func segmentDescriptorTypeDpl() {
    let segmentDescriptor = SegmentDescriptor(upper: 0x5505_65AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.type == 5)
    #expect(segmentDescriptor.dpl == 3)
}
