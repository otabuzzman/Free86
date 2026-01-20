protocol TranslationLookasideBuffer {
    func tlbLookup(linear: LinearAddress, writable: Bool) throws -> DWord
    func tlbUpdate(linear: LinearAddress, with address: DWord, writable: Bool, user: Bool)
    func tlbFlush()
    func tlbFlush(pageContainingAddress linear: LinearAddress)
}
