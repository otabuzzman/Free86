extension Free86: DirectMemory {
    typealias Address = DWord
    public func ld8(from address: Address) -> Byte {
        memory[address]
    }
    public func ld16(from address: Address) -> Word {
        var word = Word(ld8(from: address))
        word.upperHalf = Word(ld8(from: address + 1))
        return word
    }
    public func ld(from address: Address) -> DWord {
        var dword = DWord(ld8(from: address))
        dword |= DWord(ld8(from: address + 1)) << 8
        dword |= DWord(ld8(from: address + 2)) << 16
        dword |= DWord(ld8(from: address + 3)) << 24
        return dword
    }
    public func st8(at address: Address, byte: Byte) {
        memory[address] = byte
    }
    public func st16(at address: Address, word: Word) {
        st8(at: address, byte: Byte(word.lowerHalf))
        st8(at: address + 1, byte: Byte(word.upperHalf))
    }
    public func st(at address: Address, dword: DWord) {
        st8(at: address, byte: Byte(truncatingIfNeeded: dword))
        st8(at: address + 1, byte: Byte(truncatingIfNeeded: dword >> 8))
        st8(at: address + 2, byte: Byte(truncatingIfNeeded: dword >> 16))
        st8(at: address + 3, byte: Byte(truncatingIfNeeded: dword >> 24))
    }
}
