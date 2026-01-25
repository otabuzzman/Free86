extension MemoryIO: DirectMemory {
    public func ld8(from address: A) -> Byte {
        self[address]
    }
    public func ld16(from address: A) -> Word {
        var word = Word(ld8(from: address))
        word.upperHalf = Word(ld8(from: address + 1))
        return word
    }
    public func ld(from address: A) -> DWord {
        var dword = DWord(ld8(from: address))
        dword |= DWord(ld8(from: address + 1)) << 8
        dword |= DWord(ld8(from: address + 2)) << 16
        dword |= DWord(ld8(from: address + 3)) << 24
        return dword
    }
    public func ld64(from address: A) -> QWord {
        var qword = QWord(ld8(from: address))
        qword |= QWord(ld8(from: address + 1)) << 8
        qword |= QWord(ld8(from: address + 2)) << 16
        qword |= QWord(ld8(from: address + 3)) << 24
        qword |= QWord(ld8(from: address + 4)) << 32
        qword |= QWord(ld8(from: address + 5)) << 40
        qword |= QWord(ld8(from: address + 6)) << 48
        qword |= QWord(ld8(from: address + 7)) << 56
        return qword
    }
    public func st8(at address: A, byte: Byte) {
        self[address] = byte
    }
    public func st16(at address: A, word: Word) {
        st8(at: address, byte: Byte(word.lowerHalf))
        st8(at: address + 1, byte: Byte(word.upperHalf))
    }
    public func st(at address: A, dword: DWord) {
        st8(at: address, byte: Byte(truncatingIfNeeded: dword))
        st8(at: address + 1, byte: Byte(truncatingIfNeeded: dword >> 8))
        st8(at: address + 2, byte: Byte(truncatingIfNeeded: dword >> 16))
        st8(at: address + 3, byte: Byte(truncatingIfNeeded: dword >> 24))
    }
    public func st64(at address: A, qword: QWord) {
        st8(at: address, byte: Byte(truncatingIfNeeded: qword))
        st8(at: address + 1, byte: Byte(truncatingIfNeeded: qword >> 8))
        st8(at: address + 2, byte: Byte(truncatingIfNeeded: qword >> 16))
        st8(at: address + 3, byte: Byte(truncatingIfNeeded: qword >> 24))
        st8(at: address + 4, byte: Byte(truncatingIfNeeded: qword >> 32))
        st8(at: address + 5, byte: Byte(truncatingIfNeeded: qword >> 40))
        st8(at: address + 6, byte: Byte(truncatingIfNeeded: qword >> 48))
        st8(at: address + 7, byte: Byte(truncatingIfNeeded: qword >> 56))
    }
}
