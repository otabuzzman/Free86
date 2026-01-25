public protocol DirectMemory {
    associatedtype Address: PhysicalAddress
    func ld8(from address: Address) -> Byte
    func ld16(from address: Address) -> Word
    func ld(from address: Address) -> DWord
    func ld64(from address: Address) -> QWord
    func st8(at address: Address, byte: Byte)
    func st16(at address: Address, word: Word)
    func st(at address: Address, dword: DWord)
    func st64(at address: Address, qword: QWord)
}
