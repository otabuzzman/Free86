import Testing
@testable import Free86

@Test("control register flags")
func conrolRegister() {
    var cr0: DWord = 0xDEADBEAF  // 0b1101_1110_1010_1101_1100_1010_1111_1110
    #expect(cr0.isFlagRaised(.PE) == true)
    #expect(cr0.isFlagRaised(.MP) == true)
    #expect(cr0.isFlagRaised(.EM) == true)
    #expect(cr0.isFlagRaised(.TS) == true)
    #expect(cr0.isFlagRaised(.ET) == false)
    #expect(cr0.isFlagRaised(.WP) == true)
    #expect(cr0.isFlagRaised(.PG) == true)
    #expect(cr0.pageDirectoryBase == 0xDEADB000)

    cr0 &= 0x7FFEFFF0
    cr0.setBit(4)
    #expect(cr0.isFlagRaised(.PE) == false)
    #expect(cr0.isFlagRaised(.MP) == false)
    #expect(cr0.isFlagRaised(.EM) == false)
    #expect(cr0.isFlagRaised(.TS) == false)
    #expect(cr0.isFlagRaised(.ET) == true)
    #expect(cr0.isFlagRaised(.WP) == false)
    #expect(cr0.isFlagRaised(.PG) == false)
    #expect(cr0.pageDirectoryBase == 0x5EACB000)
}
