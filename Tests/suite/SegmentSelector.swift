import Testing
@testable import Free86

@Test("segment selector indices")
func segmentSelectorIndices() {
    var segmentSelector: Word
    segmentSelector = 0b1100_1010_1111_1110 // 0xCAFE
    #expect(segmentSelector.index == 0b0001_1001_0101_1111)
    segmentSelector = 0b1100_1110_1010_1100 // 0xDEAD
    #expect(segmentSelector.index == 0b0001_1001_1101_0101)
}

@Test("segment selector tables")
func segmentSelectorTables() {
    var segmentSelector: Word
    segmentSelector = 0b1100_1010_1111_1110
    #expect(segmentSelector.isGDT == false)
    #expect(segmentSelector.isLDT == true)
    segmentSelector = 0b1100_1110_1010_1000
    #expect(segmentSelector.isGDT == true)
    #expect(segmentSelector.isLDT == false)
}

@Test("segment selector RPL")
func segmentSelectorRpl() {
    var segmentSelector: Word
    segmentSelector = 0b1100_1010_1111_1111
    #expect(segmentSelector.rpl == 3)
    segmentSelector = 0b1100_1010_1111_1110
    #expect(segmentSelector.rpl == 2)
    segmentSelector = 0b1100_1010_1111_1101
    #expect(segmentSelector.rpl == 1)
    segmentSelector = 0b1100_1010_1111_1100
    #expect(segmentSelector.rpl == 0)
}
