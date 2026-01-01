import Testing
@testable import Free86

@Test("linear adddress indices")
func linearAddressIndices() {
    var linearAddress: DWord
    linearAddress = 0xDEADCAFE // 0b1101_1110_1010_1101_1100_1010_1111_1110
    #expect(linearAddress.pageDirectoryIndex == 0b0011_0111_1010)
    #expect(linearAddress.pageTableIndex     == 0b0010_1101_1100)
    #expect(linearAddress.pageOffset         == 0b1010_1111_1110)
    linearAddress = 0xBEAFC0DE // 0b1011_1110_1010_1111_1100_0000_1101_1110
    #expect(linearAddress.pageDirectoryIndex == 0b0010_1111_1010)
    #expect(linearAddress.pageTableIndex     == 0b0010_1111_1100)
    #expect(linearAddress.pageOffset         == 0b0000_1101_1110)
}
