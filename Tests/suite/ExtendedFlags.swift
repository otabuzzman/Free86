import Testing
@testable import Free86

@Test("extended flags")
func extendedFlags() {
    let eflags: DWord = 0xDEADBEAF  // 1101_1110_1010_1101_1011_1110_1010_1111
    #expect(eflags.isFlagRaised(.CF) == true)
    #expect(eflags.isFlagRaised(.PF) == true)
    #expect(eflags.isFlagRaised(.AF) == false)
    #expect(eflags.isFlagRaised(.ZF) == false)
    #expect(eflags.isFlagRaised(.SF) == true)
    #expect(eflags.isFlagRaised(.TF) == false)
    #expect(eflags.isFlagRaised(.IF) == true)
    #expect(eflags.isFlagRaised(.DF) == true)
    #expect(eflags.isFlagRaised(.OF) == true)
    #expect(eflags.isFlagRaised(.NT) == false)
    #expect(eflags.isFlagRaised(.RF) == true)
    #expect(eflags.isFlagRaised(.VM) == false)
}

@Test("extended flags IOPL")
func extendedFlagsIopl() {
    var eflags: DWord = 0xDEADBEAF  // 1101_1110_1010_1101_1011_1110_1010_1111
    #expect(eflags.iopl == 3)
    eflags.iopl = 2
    #expect(eflags.iopl == 2)
    eflags.iopl = 1
    #expect(eflags.iopl == 1)

    #expect(eflags.isFlagRaised(.CF) == true)
    #expect(eflags.isFlagRaised(.PF) == true)
    #expect(eflags.isFlagRaised(.AF) == false)
    #expect(eflags.isFlagRaised(.ZF) == false)
    #expect(eflags.isFlagRaised(.SF) == true)
    #expect(eflags.isFlagRaised(.TF) == false)
    #expect(eflags.isFlagRaised(.IF) == true)
    #expect(eflags.isFlagRaised(.DF) == true)
    #expect(eflags.isFlagRaised(.OF) == true)
    #expect(eflags.isFlagRaised(.NT) == false)
    #expect(eflags.isFlagRaised(.RF) == true)
    #expect(eflags.isFlagRaised(.VM) == false)
}
