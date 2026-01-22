import Testing
@testable import Free86

@Test("modR/M byte elements")
func modRMByteElements() {
    var modRM: Byte
    modRM = 0b10_100_101
    #expect(modRM.mod == 2)
    #expect(modRM.reg == 4)
    #expect(modRM.opcode == 4)
    #expect(modRM.rM == 5)

    modRM = 0b01_011_010
    #expect(modRM.mod == 1)
    #expect(modRM.reg == 3)
    #expect(modRM.opcode == 3)
    #expect(modRM.rM == 2)
}

@Test("SIB byte elements")
func sibByteElements() {
    var sib: Byte
    sib = 0b10_100_101
    #expect(sib.scale == 2)
    #expect(sib.index == 4)
    #expect(sib.base == 5)

    sib = 0b01_011_010
    #expect(sib.scale == 1)
    #expect(sib.index == 3)
    #expect(sib.base == 2)
}
