import Testing
@testable import Free86

@Test("segment selector flags positions")
func segmentSelectorFlagsPositions() {
    #expect(SegmentSelectorFlag.TI.rawValue == 2)
}

@Test("segment selector set/ check flags")
func segmentSelectorSetCheckFlags() {
    var segmentSelector: SegmentSelector = 0xBEAF  // 0b100_1011_1010_1111
    #expect(segmentSelector.isFlagRaised(.TI) == true)
    segmentSelector.setFlag(.TI, 0)
    #expect(segmentSelector == 0xBEAB)
}

@Test("segment selector fields access")
func segmentSelectorFieldsAccess() {
    var segmentSelector: Word
    segmentSelector = 0b1100_1010_1111_1110  // 0xCAFE

    #expect(segmentSelector.index == 0b0001_1001_0101_1111)

    #expect(segmentSelector.isGDT == false)
    #expect(segmentSelector.isLDT == true)

    segmentSelector = 0b1100_1010_1111_1111
    #expect(segmentSelector.rpl == 3)
    segmentSelector = 0b1100_1010_1111_1110
    #expect(segmentSelector.rpl == 2)
    segmentSelector = 0b1100_1010_1111_1101
    #expect(segmentSelector.rpl == 1)
    segmentSelector = 0b1100_1010_1111_1100
    #expect(segmentSelector.rpl == 0)
}
