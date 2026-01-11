import Testing
@testable import Free86

@Test("instruction prefix register flags")
func instructionPrefixRegisterFlags() {
    var ipr: DWord = 0xDEADBEAF  // 0b1101_1110_1010_1101_1100_1010_1111_1110
    #expect(ipr.isFlagRaised(.repzStringOperation) == false)
    #expect(ipr.isFlagRaised(.repnzStringOperation) == true)
    #expect(ipr.isFlagRaised(.lockSignal) == false)
    #expect(ipr.isFlagRaised(.operandSizeOverride) == true)
    #expect(ipr.isFlagRaised(.addressSizeOverride) == false)

    ipr.setBit(4)
    #expect(ipr.isFlagRaised(.repzStringOperation) == true)

    ipr.setBit(0, 0)
    #expect(ipr.segmentRegisterIndex == SegmentRegister.Name.GS.rawValue)

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

    #expect(InstructionPrefixRegister.maskFlag(.repzStringOperation)  == 0b00_0001_0000)
    #expect(InstructionPrefixRegister.maskFlag(.repnzStringOperation) == 0b00_0010_0000)
    #expect(InstructionPrefixRegister.maskFlag(.lockSignal)           == 0b00_0100_0000)
    #expect(InstructionPrefixRegister.maskFlag(.operandSizeOverride)  == 0b00_1000_0000)
    #expect(InstructionPrefixRegister.maskFlag(.addressSizeOverride)  == 0b01_0000_0000)
}
