import Testing
@testable import Free86

@Test("control register CR0 flags positions")
func conrolRegister0FlagsPositions() {
    #expect(CR0Flag.PE.rawValue == 0)
    #expect(CR0Flag.MP.rawValue == 1)
    #expect(CR0Flag.EM.rawValue == 2)
    #expect(CR0Flag.TS.rawValue == 3)
    #expect(CR0Flag.ET.rawValue == 4)
    #expect(CR0Flag.WP.rawValue == 16)
    #expect(CR0Flag.PG.rawValue == 31)
}

@Test("control register CR0 set/ check flags")
func conrolRegister0SetCheckFlags() {
    var cr0: CR0 = 0xDEADBEAF  // 0b1101_1110_1010_1101_1100_1011_1010_1111
    #expect(cr0.isFlagRaised(.PE) == true)
    cr0.setFlag(.PE, 0)
    #expect(cr0 == 0xDEADBEAE)
}

@Test("control register CR3 fields Access")
func conrolRegister3FieldsAccess() {
    let cr3: CR3 = 0xCAFEBABE
    #expect(cr3.pageDirectoryBase == 0xCAFEB000)
}
