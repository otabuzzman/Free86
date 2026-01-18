public protocol PagedMemory {
    associatedtype Address: PhysicalAddress
    func ld8Direct(from address: Address) -> Byte
    func ld16Direct(from address: Address) -> Word
    func ldDirect(from address: Address) -> DWord
    func st8Direct(at address: Address, byte: Byte)
    func st16Direct(at address: Address, word: Word)
    func stDirect(at address: Address, dword: DWord)
}
