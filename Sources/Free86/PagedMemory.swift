protocol PagedMemory {
    /// load byte/ word/ dword/ qword from read-only supervisor memory
    func ld8ReadonlyCplX() throws -> Byte
    func ld16ReadonlyCplX() throws -> Word
    func ldReadonlyCplX() throws -> DWord
    func ld64ReadonlyCplX() throws -> QWord
    /// load byte/ word/ dword from read-only user memory
    func ld8ReadonlyCpl3() throws -> Byte
    func ld16ReadonlyCpl3() throws -> Word
    func ldReadonlyCpl3() throws -> DWord
    /// load byte/ word/ dword from writable user memory
    func ld8WritableCpl3() throws -> Byte
    func ld16WritableCpl3() throws -> Word
    func ldWritableCpl3() throws -> DWord
    /// store byte/ word/ dword in read-only supervisor memory
    func st8WritableCplX(byte: Byte) throws
    func st16WritableCplX(word: Word) throws
    func stWritableCplX(dword: DWord) throws
    /// store byte/ word/ dword in read-only user memory
    func st8WritableCpl3(byte: Byte) throws
    func st16WritableCpl3(word: Word) throws
    func stWritableCpl3(dword: DWord) throws
}
