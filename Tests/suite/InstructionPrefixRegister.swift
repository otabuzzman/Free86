import Testing
@testable import Free86

@Test("IPR flags positions")
func instructionPrefixRegisterFlagsPositions() {
    #expect(InstructionPrefixRegisterFlag.repzStringOperation.rawValue == 4)
    #expect(InstructionPrefixRegisterFlag.repnzStringOperation.rawValue == 5)
    #expect(InstructionPrefixRegisterFlag.lockSignal.rawValue == 6)
    #expect(InstructionPrefixRegisterFlag.operandSizeOverride.rawValue == 7)
    #expect(InstructionPrefixRegisterFlag.addressSizeOverride.rawValue == 8)
}

@Test("IPR set/ check flags")
func instructionPrefixRegisterSetGetFlags() {
    var ipr: DWord = 0xDEADBEAF  // 1101_1110_1010_1101_1011_1110_1010_1111
    #expect(ipr.isFlagRaised(.repzStringOperation) == false)
    ipr.setFlag(.repzStringOperation)
    #expect(ipr.isFlagRaised(.repzStringOperation) == true)
}

@Test("IPR fields access")
func instructionPrefixRegisterFieldsAccess() {
    var ipr: DWord = 0xDEADBEAF  // 1101_1110_1010_1101_1011_1110_1010_1111
    ipr &= 0xFF8
    #expect(ipr.segmentRegisterIndex == SegmentRegister.Name.DS.rawValue)
    ipr = ipr & 0xFF8 | 1
    #expect(ipr.segmentRegisterIndex == SegmentRegister.Name.ES.rawValue)
    ipr = ipr & 0xFF8 | 2
    #expect(ipr.segmentRegisterIndex == SegmentRegister.Name.CS.rawValue)
    ipr = ipr & 0xFF8 | 3
    #expect(ipr.segmentRegisterIndex == SegmentRegister.Name.SS.rawValue)
    ipr = ipr & 0xFF8 | 4
    #expect(ipr.segmentRegisterIndex == SegmentRegister.Name.DS.rawValue)
    ipr = ipr & 0xFF8 | 5
    #expect(ipr.segmentRegisterIndex == SegmentRegister.Name.FS.rawValue)
    ipr = ipr & 0xFF8 | 6
    #expect(ipr.segmentRegisterIndex == SegmentRegister.Name.GS.rawValue)
    #expect(ipr.operandSizeMask == 0xFFFF)
    ipr.setFlag(.operandSizeOverride, 0)
    #expect(ipr.operandSizeMask == 0xFFFFFFFF)
}
