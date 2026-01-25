import Testing
@testable import Free86

@Test("segment register bank")
func segmentRegisterBank() {
    let bank = [
        SegmentRegister(0xC0DE, .init(0xC0DECAFE_DEADBEEF)),
        SegmentRegister(0xBEAF, .init(0xDECAFEDE_ADBEEFC0)),
        SegmentRegister(0xC0DE, .init(0xCAFEDEAD_BEEFC0DE)),
        SegmentRegister(0xDEAD, .init(0xFEDEADBE_EFC0DECA)),
        SegmentRegister(0xADC0, .init(0xDEADBEEF_C0DECAFE)),
        SegmentRegister(0xDEDE, .init(0xADBEEFC0_DECAFEDE)),
        SegmentRegister(0xDEAD, .init(0xBEEFC0DE_CAFEDEAD)),
        SegmentRegister(0xDEC0, .init(0xEFC0DECA_FEDEADBE)),
    ]
    #expect(bank[.ES] == SegmentRegister(0xC0DE, .init(0xC0DECAFE_DEADBEEF)))
    #expect(bank[.CS] == SegmentRegister(0xBEAF, .init(0xDECAFEDE_ADBEEFC0)))
    #expect(bank[.SS] == SegmentRegister(0xC0DE, .init(0xCAFEDEAD_BEEFC0DE)))
    #expect(bank[.DS] == SegmentRegister(0xDEAD, .init(0xFEDEADBE_EFC0DECA)))
    #expect(bank[.FS] == SegmentRegister(0xADC0, .init(0xDEADBEEF_C0DECAFE)))
    #expect(bank[.GS] == SegmentRegister(0xDEDE, .init(0xADBEEFC0_DECAFEDE)))
    #expect(bank[.LDT] == SegmentRegister(0xDEAD, .init(0xBEEFC0DE_CAFEDEAD)))
    #expect(bank[.TR] == SegmentRegister(0xDEC0, .init(0xEFC0DECA_FEDEADBE)))
}
