protocol PagedMemory {
    /// load byte/ word/ dword as supervisor from read-only memory
    func ld8FromReadonly()
    func ld16FromReadonly()
    func ldFromReadonly()
    /// load byte/ word/ dword as user from read-only memory
    func ld8FromUserReadonly()
    func ld16FromUserReadonly()
    func ldFromUserReadonly()
    /// load byte/ word/ dword as user from writable memory
    func ld8FromUserWritable()
    func ld16FromUserWritable()
    func ldFromUserWritable()
    /// store byte/ word/ dword as supervisor in read-only memory
    func st8InReadonly(byte: Byte)
    func st16InReadonly(word: Word)
    func stInReadonly(dword: DWord)
    /// store byte/ word/ dword as user in read-only memory
    func st8InUserReadonly(byte: Byte)
    func st16InUserReadonly(word: Word)
    func stInUserReadonly(dword: DWord)

    func translate(_ linear: LinearAddress, writable: Bool, user: Bool) throws
}
