class PagedMemory<T: DWord>: MemoryIO< DWord> {
}

extension PagedMemory {
}

protocol DirectMemory {
    associatedtype Address: PhysicalAddress
    func ld8(from address: Address) -> Byte
    func ld16(from address: Address) -> Word
    func ld(from address: Address) -> Dword
    func st8(at address: Address, byte: Byte)
    func st16(at address: Address, word: Word)
    func st(at address: Address, dword: DWord)
}

extension DirectMemory {
    func ld8(from address: Address) -> Byte {
        self[address]
    }
    func ld16(from address: Address) -> Word {
        var word = Word(ld8(from: address))
        word.upperHalf = Word(ld8(from: address + 1))
        return word
    }
    func ld(from address: Address) -> Dword {
        var dword = DWord(ld8(from: address))
        dword |= DWord(ld8(from: address + 1) << 8)
        dword |= DWord(ld8(from: address + 2) << 16)
        dword |= DWord(ld8(from: address + 3) << 24)
        return dword
    }
    func st8(at address: Address, byte: Byte) {
        self[address] = byte
    }
    func st16(at address: Address, word: Word) {
        st8(at: address, Byte(word.lowerHalf))
        st8(at: address + 1, Byte(word.upperHalf))
    }
    func st(at address: Address, dword: DWord) {
        st8(at: address, Byte(dword))
        st8(at: address + 1, Byte(dword >> 8))
        st8(at: address + 2, Byte(dword >> 16))
        st8(at: address + 3, Byte(dword >> 24))
    }
}
