import Testing
@testable import Free86

@Test("general register elements")
func generalRegisterElements() {
    var register: DWord
    register = 0xDEADCAFE
    #expect(register.byteL == 0xFE)
    #expect(register.byteH == 0xCA)
    #expect(register.wordX == 0xCAFE)

    register.wordX = 0xBEEF
    #expect(register.byteL == 0xEF)
    #expect(register.byteH == 0xBE)
    #expect(register.wordX == 0xBEEF)

    register.byteL = 0x55
    register.byteH = 0xAA
    #expect(register == 0xDEADAA55)

    register.upperHalf = 0xC0DE
    #expect(register == 0xC0DEAA55)
}
