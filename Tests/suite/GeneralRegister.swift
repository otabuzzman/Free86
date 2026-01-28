import Testing
@testable import Free86

@Test("general register X/ H/ L set/ get")
func generalRegisterXHLSetGet() {
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

@Test("general register index access by name")
func generalRegisterIndexAccessByName() {
    let bank = [
        GeneralRegister(0xDEADC0DE),
        GeneralRegister(0xDEADBEAF),
        GeneralRegister(0xBEAFC0DE),
        GeneralRegister(0xBEEFDEAD),
        GeneralRegister(0xDEDEADC0),
        GeneralRegister(0xADC0DEDE),
        GeneralRegister(0xDEC0DEAD),
        GeneralRegister(0xDEADDEC0),
    ]
    #expect(bank[.EAX] == 0xDEADC0DE)
    #expect(bank[.ECX] == 0xDEADBEAF)
    #expect(bank[.EDX] == 0xBEAFC0DE)
    #expect(bank[.EBX] == 0xBEEFDEAD)
    #expect(bank[.ESP] == 0xDEDEADC0)
    #expect(bank[.EBP] == 0xADC0DEDE)
    #expect(bank[.ESI] == 0xDEC0DEAD)
    #expect(bank[.EDI] == 0xDEADDEC0)
}
