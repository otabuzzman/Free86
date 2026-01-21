protocol PagedMemory {
    /// load byte/ word/ dword from read-only supervisor memory
    func ld8FromReadonly() throws -> Byte
    func ld16FromReadonly()
    func ldFromReadonly()
    /// load byte/ word/ dword from read-only user memory
    func ld8FromUserReadonly() throws
    func ld16FromUserReadonly()
    func ldFromUserReadonly()
    /// load byte/ word/ dword from writable user memory
    func ld8FromUserWritable() throws
    func ld16FromUserWritable()
    func ldFromUserWritable()
    /// store byte/ word/ dword in read-only supervisor memory
    func st8InWritable(byte: Byte)
    func st16InWritable(word: Word)
    func stInWritable(dword: DWord)
    /// store byte/ word/ dword in read-only user memory
    func st8InUserWritable(byte: Byte)
    func st16InUserWritable(word: Word)
    func stInUserWritable(dword: DWord)
}
