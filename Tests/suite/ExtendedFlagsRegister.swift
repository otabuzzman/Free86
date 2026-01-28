import Testing
@testable import Free86

@Test("extended flags register positions")
func extendedFlagsRegisterPositions() {
    #expect(EflagsFlag.CF.rawValue == 0)
    #expect(EflagsFlag.PF.rawValue == 2)
    #expect(EflagsFlag.AF.rawValue == 4)
    #expect(EflagsFlag.ZF.rawValue == 6)
    #expect(EflagsFlag.SF.rawValue == 7)
    #expect(EflagsFlag.TF.rawValue == 8)
    #expect(EflagsFlag.IF.rawValue == 9)
    #expect(EflagsFlag.DF.rawValue == 10)
    #expect(EflagsFlag.OF.rawValue == 11)
    #expect(EflagsFlag.NT.rawValue == 14)
    #expect(EflagsFlag.RF.rawValue == 16)
    #expect(EflagsFlag.VM.rawValue == 17)
}

@Test("extended flags register set/ check")
func extendedFlagsRegisterSetCheck() {
    var eflags: DWord = 0xDEADBEAF  // 1101_1110_1010_1101_1011_1110_1010_1111
    #expect(eflags.isFlagRaised(.CF) == true)
    eflags.setFlag(.CF, 0)
    #expect(eflags == 0xDEADBEAE)
}

@Test("extended flags register fields access")
func extendedFlagsRegisterFieldsAccess() {
    var eflags: DWord = 0xDEADBEAF  // 1101_1110_1010_1101_1011_1110_1010_1111
    #expect(eflags.iopl == 3)
    eflags.iopl = 2
    #expect(eflags.iopl == 2)
    eflags.iopl = 1
    #expect(eflags.iopl == 1)
}
