import Testing
@testable import Free86

import Foundation

@Test("segment descriptor flags positions")
func segmentDescriptorFlagsPositions() {
    #expect(SegmentDescriptorFlag.G.rawValue == 23)
    #expect(SegmentDescriptorFlag.D.rawValue == 22)
    #expect(SegmentDescriptorFlag.P.rawValue == 15)
    #expect(SegmentDescriptorFlag.S.rawValue == 12)
    #expect(SegmentDescriptorTypeFlag.A.rawValue == 8)
    #expect(SegmentDescriptorCodeTypeFlag.C.rawValue == 10)
    #expect(SegmentDescriptorCodeTypeFlag.R.rawValue == 9)
    #expect(SegmentDescriptorDataTypeFlag.E.rawValue == 10)
    #expect(SegmentDescriptorDataTypeFlag.W.rawValue == 9)
}

@Test("segment descriptor set/ check flags")
func segmentDescriptorSetCheckFlags() {
    var segmentDescriptor = SegmentDescriptor(0xC0FEBA5E, 0x00012345, .CodeExOnly, 3)
    #expect(segmentDescriptor.isFlagRaised(.G) == false)
    segmentDescriptor.setFlag(.G, 1)
    #expect(segmentDescriptor.upper == 0xC08178FE)  // 0111_1000
    /// TSS
    #expect(segmentDescriptor.upper.isBitRaised(9) == false)
    /// data segment
    #expect(segmentDescriptor.isFlagRaised(.A) == false)
    segmentDescriptor.upper.raiseBit(SegmentDescriptorTypeFlag.A.rawValue)
    #expect(segmentDescriptor.isFlagRaised(.A) == true)
    #expect(segmentDescriptor.isFlagRaised(.R) == false)
    #expect(segmentDescriptor.isFlagRaised(.C) == false)
    /// code segment
    segmentDescriptor.upper.clearBit(SegmentDescriptorTypeFlag.A.rawValue)
    #expect(segmentDescriptor.isFlagRaised(.A) == false)
    #expect(segmentDescriptor.isFlagRaised(.W) == false)
    #expect(segmentDescriptor.isFlagRaised(.E) == false)
}

@Test("segment descriptor type values")
func segmentDescriptorTypeValues() {
    #expect(SegmentDescriptorType.none.rawValue                         == 0b0_0000)
    #expect(SegmentDescriptorType.TSS16Available.rawValue               == 0b0_0001)
    #expect(SegmentDescriptorType.LDT.rawValue                          == 0b0_0010)
    #expect(SegmentDescriptorType.TSS16Busy.rawValue                    == 0b0_0011)
    #expect(SegmentDescriptorType.CallGate16.rawValue                   == 0b0_0100)
    #expect(SegmentDescriptorType.TaskGate.rawValue                     == 0b0_0101)
    #expect(SegmentDescriptorType.InterruptGate16.rawValue              == 0b0_0110)
    #expect(SegmentDescriptorType.TrapGate16.rawValue                   == 0b0_0111)
    #expect(SegmentDescriptorType.TSSAvailable.rawValue                 == 0b0_1001)
    #expect(SegmentDescriptorType.TSSBusy.rawValue                      == 0b0_1011)
    #expect(SegmentDescriptorType.CallGate.rawValue                     == 0b0_1100)
    #expect(SegmentDescriptorType.InterruptGate.rawValue                == 0b0_1110)
    #expect(SegmentDescriptorType.TrapGate.rawValue                     == 0b0_1111)
    #expect(SegmentDescriptorType.DataRO.rawValue                       == 0b1_0000)
    #expect(SegmentDescriptorType.DataROAccessed.rawValue               == 0b1_0001)
    #expect(SegmentDescriptorType.DataRW.rawValue                       == 0b1_0010)
    #expect(SegmentDescriptorType.DataRWAccessed.rawValue               == 0b1_0011)
    #expect(SegmentDescriptorType.DataROExpandDown.rawValue             == 0b1_0100)
    #expect(SegmentDescriptorType.DataROExpandDownAccessed.rawValue     == 0b1_0101)
    #expect(SegmentDescriptorType.DataRWExpandDown.rawValue             == 0b1_0110)
    #expect(SegmentDescriptorType.DataRWExpandDownAccessed.rawValue     == 0b1_0111)
    #expect(SegmentDescriptorType.CodeExOnly.rawValue                   == 0b1_1000)
    #expect(SegmentDescriptorType.CodeExOnlyAccessed.rawValue           == 0b1_1001)
    #expect(SegmentDescriptorType.CodeExRead.rawValue                   == 0b1_1010)
    #expect(SegmentDescriptorType.CodeExReadAccessed.rawValue           == 0b1_1011)
    #expect(SegmentDescriptorType.CodeExOnlyConforming.rawValue         == 0b1_1100)
    #expect(SegmentDescriptorType.CodeExOnlyConformingAccessed.rawValue == 0b1_1101)
    #expect(SegmentDescriptorType.CodeExReadConforming.rawValue         == 0b1_1110)
    #expect(SegmentDescriptorType.CodeExReadConformingAccessed.rawValue == 0b1_1111)
}

@Test("segment descriptor fields access")
func segmentDescriptorFieldsAccess() {
    var segmentDescriptor = SegmentDescriptor(0xC0FEBA5E, 0x00012345, .CodeExOnly, 3)
    #expect(segmentDescriptor.base  == 0xC0FEBA5E)
    segmentDescriptor.base = 0xDEADBEEF
    #expect(segmentDescriptor.base  == 0xDEADBEEF)
    #expect(segmentDescriptor.limit == 0x00012345)
    segmentDescriptor.limit = 0x00054321
    #expect(segmentDescriptor.limit == 0x00054321)
    #expect(segmentDescriptor.flags == 0x00007800)
    segmentDescriptor.setFlag(.G, 1)
    #expect(segmentDescriptor.flags == 0x00807800)
    segmentDescriptor.setFlag(.G, 0)
    #expect(segmentDescriptor.type == 0b0001_1000)
    segmentDescriptor.type = 0b1_0010
    #expect(segmentDescriptor.type == 0b1_0010)
    #expect(segmentDescriptor.dpl  == 3)
    segmentDescriptor.dpl = 0
    #expect(segmentDescriptor.dpl  == 0)
    #expect(segmentDescriptor.qword == 0xDE05_12AD_BEEF_4321)
    #expect(segmentDescriptor.isSystemSegment == false)
    #expect(segmentDescriptor.isDataSegment == true)
    #expect(segmentDescriptor.isCodeSegment == false)
    #expect(segmentDescriptor.segmentSizeMask == 0xFFFF)
    segmentDescriptor.setFlag(.D, 1)
    #expect(segmentDescriptor.segmentSizeMask == 0xFFFFFFFF)
}
