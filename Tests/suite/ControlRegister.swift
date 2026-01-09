import Testing
@testable import Free86

@Test("control register flags")
func conrolRegister() {
    var cr0: DWord = 0xDEADBEAF
    #expect(cr0.isFlagRaised(.PE) == true)
    #expect(cr0.isFlagRaised(.MP) == true)
    #expect(cr0.isFlagRaised(.EM) == true)
    #expect(cr0.isFlagRaised(.TS) == true)
    #expect(cr0.isFlagRaised(.ET) == false)
    #expect(cr0.isFlagRaised(.PG) == true)
    cr0 &= 0x7FFFFFF0
    cr0.setBit(4)
    #expect(cr0.isFlagRaised(.PE) == false)
    #expect(cr0.isFlagRaised(.MP) == false)
    #expect(cr0.isFlagRaised(.EM) == false)
    #expect(cr0.isFlagRaised(.TS) == false)
    #expect(cr0.isFlagRaised(.ET) == true)
    #expect(cr0.isFlagRaised(.PG) == false)
}
