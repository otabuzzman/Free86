import Testing
@testable import Free86

@Test("machine data type sizes")
func machineDataTypeSizes() {
    #expect(Byte.bitWidth == 8)
    #expect(Word.bitWidth == 16)
    #expect(DWord.bitWidth == 32)
    #expect(QWord.bitWidth == 64)
}

@Test("machine data type upper/ lower half masks")
func machineDataTypeHalfMasks() {
    #expect(Byte.upperHalfMask == 0xF0)
    #expect(Byte.lowerHalfMask == 0x0F)

    #expect(Word.upperHalfMask == 0xFF00)
    #expect(Word.lowerHalfMask == 0x00FF)

    #expect(DWord.upperHalfMask == 0xFFFF_0000)
    #expect(DWord.lowerHalfMask == 0x0000_FFFF)

    #expect(QWord.upperHalfMask == 0xFFFFFFFF_00000000)
    #expect(QWord.lowerHalfMask == 0x00000000_FFFFFFFF)
}

@Test("machine data type upper/ lower half values")
func machineDataTypeHalfValues() {
    var byte: Byte = 0x5A
    #expect(byte.upperHalf == 0x5)
    #expect(byte.lowerHalf == 0xA)
    byte.upperHalf = 0xA
    byte.lowerHalf = 0x5
    #expect(byte == 0xA5)

    var word: Word = 0x5678
    #expect(word.upperHalf == 0x56)
    #expect(word.lowerHalf == 0x78)
    word.upperHalf = 0x78
    word.lowerHalf = 0x56
    #expect(word == 0x7856)

    var dWord: DWord = 0x1234_5678
    #expect(dWord.upperHalf == 0x1234)
    #expect(dWord.lowerHalf == 0x5678)
    dWord.upperHalf = 0x5678
    dWord.lowerHalf = 0x1234
    #expect(dWord == 0x5678_1234)

    var qWord: QWord = 0x01234567_89ABCDEF
    #expect(qWord.upperHalf == 0x01234567)
    #expect(qWord.lowerHalf == 0x89ABCDEF)
    qWord.upperHalf = 0x89ABCDEF
    qWord.lowerHalf = 0x01234567
    #expect(qWord == 0x89ABCDEF_01234567)
}

@Test("machine data type bit masks")
func machineDataTypeBitValues() {
    #expect(Byte.bitMask(for: 7) == 0x80)
    #expect(Byte.bitMask(for: 0) == 0x01)
    #expect(Byte.bitMask(for: 5) == 0x20)

    #expect(Word.bitMask(for: 15) == 0x8000)
    #expect(Word.bitMask(for: 0) == 0x0001)
    #expect(Word.bitMask(for: 9) == 0x0200)

    #expect(DWord.bitMask(for: 31) == 0x8000_0000)
    #expect(DWord.bitMask(for: 0) == 0x0000_0001)
    #expect(DWord.bitMask(for: 17) == 0x0002_0000)

    #expect(QWord.bitMask(for: 63) == 0x80000000_00000000)
    #expect(QWord.bitMask(for: 0) == 0x00000000_00000001)
    #expect(QWord.bitMask(for: 33) == 0x00000002_00000000)
}

@Test("machine data type set/ raise/ clear/ toggle bits")
func machineDataTypeSetRaiseClearToggleBits() {
    var byte: Byte = 0 // 0100_0111_0001_0001
    byte.raiseBit(0)
    byte.raiseBit(4)
    #expect(byte == 0x11)

    var word: Word = 0
    word.lowerHalf = Word(byte)
    word.setBit(8, 1)
    word.toggleBit(9)
    word.toggleBit(10)
    word.raiseBit(14)
    #expect(word == 0x4711)

    var dword: DWord = 0
    dword.lowerHalf = DWord(word)
    dword.raiseBit(16)
    dword.raiseBit(20)
    dword.setBit(24, 1)
    dword.toggleBit(25)
    dword.toggleBit(26)
    dword.raiseBit(30)
    #expect(dword == 0x4711_4711)

    var qword: QWord = 0
    qword.lowerHalf = QWord(dword)
    qword.raiseBit(32)
    qword.raiseBit(36)
    qword.setBit(40, 1)
    qword.toggleBit(41)
    qword.toggleBit(42)
    qword.raiseBit(46)
    qword.raiseBit(48)
    qword.raiseBit(52)
    qword.setBit(56, 1)
    qword.toggleBit(57)
    qword.toggleBit(58)
    qword.raiseBit(62)
    #expect(qword == 0x4711_4711_4711_4711)
}
