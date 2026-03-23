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
