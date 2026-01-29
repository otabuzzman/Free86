import Testing
@testable import Free86

@Test("PTE flags positions")
func pageTableEntryFlagsPositions() {
    #expect(PageTableEntryFlag.P.rawValue == 0)
    #expect(PageTableEntryFlag.W.rawValue == 1)
    #expect(PageTableEntryFlag.U.rawValue == 2)
    #expect(PageTableEntryFlag.A.rawValue == 5)
    #expect(PageTableEntryFlag.D.rawValue == 6)
}

@Test("PTE fields access")
func pageTableEntryFieldsAccess() {
    var pageTableEntry: DWord
    pageTableEntry = 0xDEADCAFE  // 0b1101_1110_1010_1101_1100_1010_1111_1110
    #expect(pageTableEntry.pageFrameAddress == 0xDEADC000)
    pageTableEntry = 0
    #expect(pageTableEntry.isPresent == false)
    pageTableEntry.setFlag(.P)
    #expect(pageTableEntry.isPresent == true)
    #expect(pageTableEntry.isWritable == false)
    pageTableEntry.setFlag(.W)
    #expect(pageTableEntry.isWritable == true)
    #expect(pageTableEntry.isUser == false)
    pageTableEntry.setFlag(.U)
    #expect(pageTableEntry.isUser == true)
    #expect(pageTableEntry.accessed == false)
    pageTableEntry.setFlag(.A)
    #expect(pageTableEntry.accessed == true)
    #expect(pageTableEntry.isDirty == false)
    pageTableEntry.setFlag(.D)
    #expect(pageTableEntry.isDirty == true)
}
