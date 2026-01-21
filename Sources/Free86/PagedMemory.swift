protocol PagedMemory {
    /// load byte/ word/ dword from read-only supervisor memory
    func ld8FromReadonly()
    func ld16FromReadonly()
    func ldFromReadonly()
    /// load byte/ word/ dword from read-only user memory
    func ld8FromUserReadonly()
    func ld16FromUserReadonly()
    func ldFromUserReadonly()
    /// load byte/ word/ dword from writable user memory
    func ld8FromUserWritable()
    func ld16FromUserWritable()
    func ldFromUserWritable()
    /// store byte/ word/ dword in read-only supervisor memory
    func st8InReadonly(byte: Byte)
    func st16InReadonly(word: Word)
    func stInReadonly(dword: DWord)
    /// store byte/ word/ dword in read-only user memory
    func st8InUserReadonly(byte: Byte)
    func st16InUserReadonly(word: Word)
    func stInUserReadonly(dword: DWord)
}
