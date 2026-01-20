protocol PagedMemory {
    func translate(_ linear: LinearAddress, writable: Bool, user: Bool) throws
}
