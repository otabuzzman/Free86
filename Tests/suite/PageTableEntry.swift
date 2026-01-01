import Testing
@testable import Free86

@Test("page table entry frame address")
func pageTableEntryFrameAddress() {
    var pageTableEntry: DWord
    pageTableEntry = 0xDEADCAFE // 0b1101_1110_1010_1101_1100_1010_1111_1110
    #expect(pageTableEntry.pageFrameAddress == 0b1101_1110_1010_1101_1100)
    pageTableEntry = 0xBEAFC0DE // 0b1011_1110_1010_1111_1100_0000_1101_1110
    #expect(pageTableEntry.pageFrameAddress == 0b1011_1110_1010_1111_1100)
}

@Test("page table entry flags")
func pageTableEntryFlags() {
    var pageTableEntry: DWord
    pageTableEntry = 0b1111_1110
    #expect(pageTableEntry.isPresent == false)
    #expect(pageTableEntry.isWritable == true)
    #expect(pageTableEntry.isUser     == true)
    #expect(pageTableEntry.isDirty    == true)
    pageTableEntry = 0b1011_1111
    #expect(pageTableEntry.isPresent == true)
    #expect(pageTableEntry.isWritable == true)
    #expect(pageTableEntry.isUser     == true)
    #expect(pageTableEntry.isDirty    == false)
    pageTableEntry = 0b1011_1001
    #expect(pageTableEntry.isPresent == true)
    #expect(pageTableEntry.isWritable == false)
    #expect(pageTableEntry.isUser     == false)
    #expect(pageTableEntry.isDirty    == false)
}
