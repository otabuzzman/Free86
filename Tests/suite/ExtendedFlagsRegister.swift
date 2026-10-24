import Testing
@testable import Free86

@Test("EFLAGS flags positions")
func extendedFlagsRegisterPositions() {
    #expect(ExtendedFlagsRegisterFlag.CF.rawValue == 0)
    #expect(ExtendedFlagsRegisterFlag.PF.rawValue == 2)
    #expect(ExtendedFlagsRegisterFlag.AF.rawValue == 4)
    #expect(ExtendedFlagsRegisterFlag.ZF.rawValue == 6)
    #expect(ExtendedFlagsRegisterFlag.SF.rawValue == 7)
    #expect(ExtendedFlagsRegisterFlag.TF.rawValue == 8)
    #expect(ExtendedFlagsRegisterFlag.IF.rawValue == 9)
    #expect(ExtendedFlagsRegisterFlag.DF.rawValue == 10)
    #expect(ExtendedFlagsRegisterFlag.OF.rawValue == 11)
    #expect(ExtendedFlagsRegisterFlag.NT.rawValue == 14)
    #expect(ExtendedFlagsRegisterFlag.RF.rawValue == 16)
    #expect(ExtendedFlagsRegisterFlag.VM.rawValue == 17)
}

@Test("EFLAGS set/ check flags")
func extendedFlagsRegisterSetCheck() {
    var eflags: DWord = 0xDEADBEAF  // 1101_1110_1010_1101_1011_1110_1010_1111
    #expect(eflags.isFlagRaised(.CF) == true)
    eflags.setFlag(.CF, .zero)
    #expect(eflags == 0xDEADBEAE)
    #expect(ExtendedFlagsRegister.flagMask(for: .DF) == 1 << ExtendedFlagsRegisterFlag.DF.rawValue)
    #expect(ExtendedFlagsRegister.flagMask(for: .DF) == 0x00000400)
}

@Test("EFLAGS fields access")
func extendedFlagsRegisterFieldsAccess() {
    var eflags: DWord = 0xDEADBEAF  // 1101_1110_1010_1101_1011_1110_1010_1111
    #expect(eflags.iopl == 3)
    eflags.iopl = 2
    #expect(eflags.iopl == 2)
    eflags.iopl = 1
    #expect(eflags.iopl == 1)
}
