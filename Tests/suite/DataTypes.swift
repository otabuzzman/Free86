import Testing
@testable import Free86

@Test("data type upper/ lower half masks")
func halfMasks() {
    #expect(Byte.upperHalfMask == 0xF0)
    #expect(Byte.lowerHalfMask == 0x0F)

    #expect(Word.upperHalfMask == 0xFF00)
    #expect(Word.lowerHalfMask == 0x00FF)

    #expect(DWord.upperHalfMask == 0xFFFF_0000)
    #expect(DWord.lowerHalfMask == 0x0000_FFFF)

    #expect(QWord.upperHalfMask == 0xFFFFFFFF_00000000)
    #expect(QWord.lowerHalfMask == 0x00000000_FFFFFFFF)
}

@Test("data type upper/ lower half values")
func halfValues() {
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

@Test("data type bit masks")
func bitValues() {
    #expect(Byte.bitMask(for: 7) == 0x80)
    #expect(Byte.bitMask(for: 0) == 0x01)
    #expect(Byte.bitMask(for: 3) == 0x08)

    #expect(Word.bitMask(for: 15) == 0x8000)
    #expect(Word.bitMask(for: 0) == 0x0001)
    #expect(Word.bitMask(for: 13) == 0x2000)

    #expect(DWord.bitMask(for: 31) == 0x8000_0000)
    #expect(DWord.bitMask(for: 0) == 0x0000_0001)
    #expect(DWord.bitMask(for: 17) == 0x0002_0000)

    #expect(QWord.bitMask(for: 63) == 0x80000000_00000000)
    #expect(QWord.bitMask(for: 0) == 0x00000000_00000001)
    #expect(QWord.bitMask(for: 17) == 0x00000000_00020000)
}

@Test("data type set/ raise/ clear/ toggle bits")
func setRaiseClearToggleBits() {
    var byte: Byte = 0b0000_0110
    var bit = 3
    #expect(byte.isBitRaised(bit) == false)
    byte.setBit(bit, 1)
    #expect(byte == 0b0000_1110)
    #expect(byte.isBitRaised(bit) == true)

    bit = 1
    #expect(byte.isBitRaised(bit) == true)
    byte.setBit(bit, 0)
    #expect(byte == 0b0000_1100)
    #expect(byte.isBitRaised(bit) == false)

    bit = 4
    #expect(byte.isBitRaised(bit) == false)
    byte.raiseBit(bit)
    #expect(byte == 0b0001_1100)
    #expect(byte.isBitRaised(bit) == true)

    bit = 2
    #expect(byte.isBitRaised(bit) == true)
    byte.clearBit(bit)
    #expect(byte == 0b0001_1000)
    #expect(byte.isBitRaised(bit) == false)

    bit = 0
    #expect(byte.isBitRaised(bit) == false)
    byte.toggleBit(bit)
    #expect(byte == 0b0001_1001)
    #expect(byte.isBitRaised(bit) == true)
}
