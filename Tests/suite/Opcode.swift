import Testing
@testable import Free86

@Test("opcode fields access")
func opcodeFieldsAccess() {
    var opcode: Opcode = 0x4711
    #expect(opcode.override == true)
    opcode.override = false
    #expect(opcode == 0x4611)
    #expect(opcode.override == false)
    opcode.override = true
    #expect(opcode == 0x4711)
    #expect(opcode.override == true)

    #expect(opcode.isOdd == true)
    #expect(opcode.isEven == false)
    opcode += 1
    #expect(opcode.isOdd == false)
    #expect(opcode.isEven == true)
}

@Test("opcode encoded fields access")
func opcodeEncodedFieldsAccess() {
    var opcode: Opcode = 0xA5  // 0b1010_0101
    #expect(opcode.encoded(.operation) == 4)
    #expect(opcode.encoded(.condition) == 5)
    #expect(opcode.encoded(.generalRegister) == 5)
    #expect(opcode.encoded(.segmentRegister) == 5)
    #expect(opcode.encoded(.standardSegmentRegister) == 4)
    #expect(opcode.encoded(.extendedSegmentRegister) == 4)

    opcode = 0x5A  // 0b0101_1010
    #expect(opcode.encoded(.operation) == 3)
    #expect(opcode.encoded(.condition) == 10)
    #expect(opcode.encoded(.generalRegister) == 2)
    #expect(opcode.encoded(.segmentRegister) == 2)
    #expect(opcode.encoded(.standardSegmentRegister) == 3)
    #expect(opcode.encoded(.extendedSegmentRegister) == 3)
}
