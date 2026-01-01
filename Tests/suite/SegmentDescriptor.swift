import Testing
@testable import Free86

@Test("segment descriptor base/ limit values")
func segmentDescriptorBaseLimit() {
    var segmentDescriptor = SegmentDescriptor(
        upper: 0x5505_00AA,
        lower: 0x55AA_AA55)
    #expect(segmentDescriptor.base  == 0x55AA_55AA)
    #expect(segmentDescriptor.limit == 0x0005_AA55)

    segmentDescriptor.setFlag(.G, 1)
    #expect(segmentDescriptor.limit == 0x5AA5_5FFF)
}

@Test("segment descriptor types")
func segmentDescriptorTypes() {
    var segmentDescriptor = SegmentDescriptor(upper: 0x5505_61AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.TSS16Available) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_62AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.LDT) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_63AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.TSS16Busy) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_64AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.CallGate16) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_65AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.TaskGate) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_66AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.InterruptGate16) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_67AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.TrapGate16) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_69AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.TSSAvailable) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_6BAA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.TSSBusy) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_6CAA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.CallGate) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_6EAA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.InterruptGate) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_6FAA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.TrapGate) == true)

    segmentDescriptor = SegmentDescriptor(upper: 0x5505_70AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.DataRO) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_71AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.DataROAccessed) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_72AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.DataRW) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_73AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.DataRWAccessed) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_74AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.DataROExpandDown) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_75AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.DataROExpandDownAccessed) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_76AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.DataRWExpandDown) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_77AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.DataRWExpandDownAccessed) == true)

    segmentDescriptor = SegmentDescriptor(upper: 0x5505_78AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.CodeExOnly) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_79AA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.CodeExOnlyAccessed) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_7AAA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.CodeExRead) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_7BAA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.CodeExReadAccessed) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_7CAA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.CodeExOnlyConforming) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_7DAA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.CodeExOnlyConformingAccessed) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_7EAA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.CodeExReadConforming) == true)
    segmentDescriptor = SegmentDescriptor(upper: 0x5505_7FAA, lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isType(.CodeExReadConformingAccessed) == true)
}

@Test("segment descriptor DPL masks")
func segmentDescriptorDplMasks() {
    var segmentDescriptor = SegmentDescriptor(
        upper: 0x5505_60AA,
        lower: 0x55AA_AA55)
    #expect(segmentDescriptor.dpl == 3)
    segmentDescriptor = SegmentDescriptor(
        upper: 0x5505_40AA,
        lower: 0x55AA_AA55)
    #expect(segmentDescriptor.dpl == 2)
    segmentDescriptor = SegmentDescriptor(
        upper: 0x5505_20AA,
        lower: 0x55AA_AA55)
    #expect(segmentDescriptor.dpl == 1)
}

@Test("segment descriptor flags")
func segmentDescriptorFlags() {
    var segmentDescriptor = SegmentDescriptor(
        upper: 0x5500_0055,
        lower: 0x55AA_AA55)
    segmentDescriptor.setFlag(.G, 1)
    segmentDescriptor.setFlag(.D, 1)
    segmentDescriptor.setFlag(.P, 1)
    segmentDescriptor.setFlag(.S, 1)
    #expect(segmentDescriptor.upper == 0x55C0_9055)
    #expect(segmentDescriptor.lower == 0x55AA_AA55)

    segmentDescriptor.setFlag(.G, 0)
    segmentDescriptor.setFlag(.D, 0)
    segmentDescriptor.setFlag(.P, 0)
    segmentDescriptor.setFlag(.S, 0)
    #expect(segmentDescriptor.upper == 0x5500_0055)
    #expect(segmentDescriptor.lower == 0x55AA_AA55)

    #expect(segmentDescriptor.isSystemSegment == true)
    #expect(segmentDescriptor.isDataSegment == false)
    #expect(segmentDescriptor.isCodeSegment == false)

    segmentDescriptor.setFlag(.S, 1)
    #expect(segmentDescriptor.isSystemSegment == false)
    #expect(segmentDescriptor.isDataSegment == true)
    #expect(segmentDescriptor.isCodeSegment == false)

    segmentDescriptor = SegmentDescriptor(
        upper: 0x5500_1855,
        lower: 0x55AA_AA55)
    #expect(segmentDescriptor.isSystemSegment == false)
    #expect(segmentDescriptor.isDataSegment == false)
    #expect(segmentDescriptor.isCodeSegment == true)
}
