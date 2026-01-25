import Testing
@testable import Free86

import Foundation

@Test("segment descriptor init")
func segmentDescriptorInit() {
    let segmentDescriptor = SegmentDescriptor(0xC0FEBA5E, 0x00012345, .CodeExOnly, 3)
    #expect(segmentDescriptor.base  == 0xC0FEBA5E)
    #expect(segmentDescriptor.limit == 0x00012345)
    #expect(segmentDescriptor.type == 0b00011000)
    #expect(segmentDescriptor.dpl  == 3)
    #expect(segmentDescriptor.upper == 0xC001_78FE)
    #expect(segmentDescriptor.lower == 0xBA5E_2345)
    #expect(segmentDescriptor.bytes == 0xC001_78FE_BA5E_2345)
}

@Test("segment descriptor base/ limit values")
func segmentDescriptorBaseLimit() {
    var segmentDescriptor: SegmentDescriptor
    segmentDescriptor = SegmentDescriptor(0x5505_00AA_55AA_AA55)
    #expect(segmentDescriptor.base  == 0x55AA_55AA)
    #expect(segmentDescriptor.limit == 0x0005_AA55)

    segmentDescriptor.setFlag(.G, 1)
    #expect(segmentDescriptor.limit == 0x5AA5_5FFF)

    segmentDescriptor.base = 0xDEADBEEF
    #expect(segmentDescriptor.base == 0xDEADBEEF)
    #expect(segmentDescriptor.limit == 0x5AA5_5FFF)
}

@Test("segment descriptor types")
func segmentDescriptorTypes() {
    var segmentDescriptor: SegmentDescriptor
    segmentDescriptor = SegmentDescriptor(0x5505_61AA_55AA_AA55)
    #expect(segmentDescriptor.isType(.TSS16Available) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_62AA_55AA_AA55)
    #expect(segmentDescriptor.isType(.LDT) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_63AA_55AA_AA55)
    #expect(segmentDescriptor.isType(.TSS16Busy) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_64AA_55AA_AA55)
    #expect(segmentDescriptor.isType(.CallGate16) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_65AA_55AA_AA55)
    #expect(segmentDescriptor.isType(.TaskGate) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_66AA_55AA_AA55)
    #expect(segmentDescriptor.isType(.InterruptGate16) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_67AA_55AA_AA55)
    #expect(segmentDescriptor.isType(.TrapGate16) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_69AA_55AA_AA55)
    #expect(segmentDescriptor.isType(.TSSAvailable) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_6BAA_55AA_AA55)
    #expect(segmentDescriptor.isType(.TSSBusy) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_6CAA_55AA_AA55)
    #expect(segmentDescriptor.isType(.CallGate) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_6EAA_55AA_AA55)
    #expect(segmentDescriptor.isType(.InterruptGate) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_6FAA_55AA_AA55)
    #expect(segmentDescriptor.isType(.TrapGate) == true)

    segmentDescriptor = SegmentDescriptor(0x5505_70AA_55AA_AA55)
    #expect(segmentDescriptor.isType(.DataRO) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_71AA_55AA_AA55)
    #expect(segmentDescriptor.isType(.DataROAccessed) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_72AA_55AA_AA55)
    #expect(segmentDescriptor.isType(.DataRW) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_73AA_55AA_AA55)
    #expect(segmentDescriptor.isType(.DataRWAccessed) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_74AA_55AA_AA55)
    #expect(segmentDescriptor.isType(.DataROExpandDown) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_75AA_55AA_AA55)
    #expect(segmentDescriptor.isType(.DataROExpandDownAccessed) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_76AA_55AA_AA55)
    #expect(segmentDescriptor.isType(.DataRWExpandDown) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_77AA_55AA_AA55)
    #expect(segmentDescriptor.isType(.DataRWExpandDownAccessed) == true)

    segmentDescriptor = SegmentDescriptor(0x5505_78AA_55AA_AA55)
    #expect(segmentDescriptor.isType(.CodeExOnly) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_79AA_55AA_AA55)
    #expect(segmentDescriptor.isType(.CodeExOnlyAccessed) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_7AAA_55AA_AA55)
    #expect(segmentDescriptor.isType(.CodeExRead) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_7BAA_55AA_AA55)
    #expect(segmentDescriptor.isType(.CodeExReadAccessed) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_7CAA_55AA_AA55)
    #expect(segmentDescriptor.isType(.CodeExOnlyConforming) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_7DAA_55AA_AA55)
    #expect(segmentDescriptor.isType(.CodeExOnlyConformingAccessed) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_7EAA_55AA_AA55)
    #expect(segmentDescriptor.isType(.CodeExReadConforming) == true)
    segmentDescriptor = SegmentDescriptor(0x5505_7FAA_55AA_AA55)
    #expect(segmentDescriptor.isType(.CodeExReadConformingAccessed) == true)
}

@Test("segment descriptor DPL masks")
func segmentDescriptorDplMasks() {
    var segmentDescriptor: SegmentDescriptor
    segmentDescriptor = SegmentDescriptor(0x5505_60AA_55AA_AA55)
    #expect(segmentDescriptor.dpl == 3)
    segmentDescriptor = SegmentDescriptor(0x5505_40AA_55AA_AA55)
    #expect(segmentDescriptor.dpl == 2)
    segmentDescriptor = SegmentDescriptor(0x5505_20AA_55AA_AA55)
    #expect(segmentDescriptor.dpl == 1)
}

@Test("segment descriptor flags")
func segmentDescriptorFlags() {
    var segmentDescriptor: SegmentDescriptor
    segmentDescriptor = SegmentDescriptor(0x5500_0055_55AA_AA55)
    segmentDescriptor.setFlag(.G, 1)
    #expect(segmentDescriptor.upper.isBitRaised(23) == true)
    segmentDescriptor.setFlag(.D, 1)
    #expect(segmentDescriptor.upper.isBitRaised(22) == true)
    segmentDescriptor.setFlag(.P, 1)
    #expect(segmentDescriptor.upper.isBitRaised(15) == true)
    segmentDescriptor.setFlag(.S, 1)
    #expect(segmentDescriptor.upper.isBitRaised(12) == true)
    #expect(segmentDescriptor.upper == 0x55C0_9055)
    #expect(segmentDescriptor.lower == 0x55AA_AA55)
    #expect(segmentDescriptor.bytes == 0x55C0_9055_55AA_AA55)

    segmentDescriptor.setFlag(.G, 0)
    #expect(segmentDescriptor.upper.isBitRaised(23) == false)
    segmentDescriptor.setFlag(.D, 0)
    #expect(segmentDescriptor.upper.isBitRaised(22) == false)
    segmentDescriptor.setFlag(.P, 0)
    #expect(segmentDescriptor.upper.isBitRaised(15) == false)
    segmentDescriptor.setFlag(.S, 0)
    #expect(segmentDescriptor.upper.isBitRaised(12) == false)
    #expect(segmentDescriptor.upper == 0x5500_0055)
    #expect(segmentDescriptor.lower == 0x55AA_AA55)
    #expect(segmentDescriptor.bytes == 0x5500_0055_55AA_AA55)

    #expect(segmentDescriptor.isSystemSegment == true)
    #expect(segmentDescriptor.isDataSegment == false)
    #expect(segmentDescriptor.isCodeSegment == false)

    segmentDescriptor.setFlag(.S, 1)
    #expect(segmentDescriptor.isSystemSegment == false)
    #expect(segmentDescriptor.isDataSegment == true)
    #expect(segmentDescriptor.isCodeSegment == false)

    segmentDescriptor = SegmentDescriptor(0x5500_1855_55AA_AA55)
    #expect(segmentDescriptor.isSystemSegment == false)
    #expect(segmentDescriptor.isDataSegment == false)
    #expect(segmentDescriptor.isCodeSegment == true)
}
