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
    func st8InReadonly()
    func st16InReadonly()
    func stInReadonly()
    /// store byte/ word/ dword as user in read-only memory
    func st8InUserReadonly()
    func st16InUserReadonly()
    func stInUserReadonly()

    func translate(_ linear: LinearAddress, writable: Bool, user: Bool) throws
}
