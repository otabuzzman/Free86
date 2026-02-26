extension MemoryIO: DirectMemory where A: FixedWidthInteger {
    public func ld8(from address: A) -> Byte {
        self[address]
    }
    public func ld16(from address: A) -> Word {
        var word = Word(ld8(from: address))
        word.upperHalf = Word(ld8(from: address &+ 1))
        return word
    }
    public func ld(from address: A) -> DWord {
        var dword = DWord(ld16(from: address))
        dword |= DWord(ld16(from: address &+ 2)) << 16
        return dword
    }
    public func ld64(from address: A) -> QWord {
        var qword = QWord(ld(from: address))
        qword |= QWord(ld(from: address &+ 4)) << 32
        return qword
    }
    public func st8(at address: A, byte: Byte) {
        self[address] = byte
    }
    public func st16(at address: A, word: Word) {
        st8(at: address, byte: Byte(word.lowerHalf))
        st8(at: address &+ 1, byte: Byte(word.upperHalf))
    }
    public func st(at address: A, dword: DWord) {
        st16(at: address, word: Word(truncatingIfNeeded: dword))
        st16(at: address &+ 2, word: Word(dword >> 16))
    }
    public func st64(at address: A, qword: QWord) {
        st(at: address, dword: DWord(truncatingIfNeeded: qword))
        st(at: address &+ 4, dword: DWord(qword >> 32))
    }
}
