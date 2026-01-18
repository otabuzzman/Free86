extension MemoryIO: PagedMemory {
    public func ld8Direct(from address: A) -> Byte {
        self[address]
    }
    public func ld16Direct(from address: A) -> Word {
        var word = Word(ld8Direct(from: address))
        word.upperHalf = Word(ld8Direct(from: address + 1))
        return word
    }
    public func ldDirect(from address: A) -> DWord {
        var dword = DWord(ld8Direct(from: address))
        dword |= DWord(ld8Direct(from: address + 1)) << 8
        dword |= DWord(ld8Direct(from: address + 2)) << 16
        dword |= DWord(ld8Direct(from: address + 3)) << 24
        return dword
    }
    public func st8Direct(at address: A, byte: Byte) {
        self[address] = byte
    }
    public func st16Direct(at address: A, word: Word) {
        st8Direct(at: address, byte: Byte(word.lowerHalf))
        st8Direct(at: address + 1, byte: Byte(word.upperHalf))
    }
    public func stDirect(at address: A, dword: DWord) {
        st8Direct(at: address, byte: Byte(truncatingIfNeeded: dword))
        st8Direct(at: address + 1, byte: Byte(truncatingIfNeeded: dword >> 8))
        st8Direct(at: address + 2, byte: Byte(truncatingIfNeeded: dword >> 16))
        st8Direct(at: address + 3, byte: Byte(truncatingIfNeeded: dword >> 24))
    }
}
