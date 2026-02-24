extension Free86: TranslationLookasideBuffer {
    func tlbLookup(linear: LinearAddress, writable: Bool) throws -> DWord {
        var hash: Int
        let pxi = linear.pageTablesIndices  // PDE and PTE indices
        if (writable) {
            hash = tlbWritable[pxi]
        } else {
            hash = tlbReadonly[pxi]
        }
        if (hash == -1) {
            try translate(linear, writable: writable, user: cpl == 3)
            if (writable) {
                hash = tlbWritable[pxi]
            } else {
                hash = tlbReadonly[pxi]
            }
        }
        return linear ^ DWord(hash)
    }
    func tlbUpdate(linear: LinearAddress, with address: DWord, writable: Bool, user: Bool) {
        let hash = Int(linear ^ address)    // XOR hash function (by design, no necessity)
        let pxi = linear.pageTablesIndices  // PD and PT indices (top 20 bits of linear address)
        if (tlbReadonlyCplX[pxi] == -1) {
            if (tlbPagesCount >= 2048) {  // flush TLB if full
                /// if present, keep PTE immediately preceding this PTE to improve performance
                tlbFlush(preservePageIfPresent: pxi &- 1)
            }
            /// record LA just added to TLB
            tlbPages[tlbPagesCount] = pxi
            tlbPagesCount += 1
        }
        /// update mapping tables
        tlbReadonlyCplX[pxi] = hash
        if (writable) {
            tlbWritableCplX[pxi] = hash
        } else {
            tlbWritableCplX[pxi] = -1
        }
        if (user) {
            tlbReadonlyCpl3[pxi] = hash
            if (writable) {
                tlbWritableCpl3[pxi] = hash
            } else {
                tlbWritableCpl3[pxi] = -1
            }
        } else {
            tlbReadonlyCpl3[pxi] = -1
            tlbWritableCpl3[pxi] = -1
        }
    }
    func tlbFlush() {
        for i in 0..<tlbPagesCount {
            tlbClear(tlbPages[i])
        }
        tlbPagesCount = 0
    }

    private func tlbFlush(pageContainingAddress linear: LinearAddress) {
        tlbClear(linear.pageTablesIndices)
    }
    private func tlbFlush(preservePageIfPresent pxi: DWord) {
        var n = 0
        for i in 0..<tlbPagesCount {
            let pageTablesIndex = tlbPages[i]
            if pageTablesIndex == pxi {
                tlbPages[n] = pageTablesIndex
                n += 1
            } else {
                tlbClear(pageTablesIndex)
            }
        }
        tlbPagesCount = n
    }
    private func tlbClear(_ pxi: DWord) {
        tlbReadonlyCplX[pxi] = -1
        tlbWritableCplX[pxi] = -1
        tlbReadonlyCpl3[pxi] = -1
        tlbWritableCpl3[pxi] = -1
    }
}

extension UnsafeMutablePointer {
    subscript(index: LinearAddress) -> Pointee {
        get { self[Int(index)] }
        set { self[Int(index)] = newValue }
    }
}

extension Free86 {
    func translate(_ linear: LinearAddress, writable: Bool, user: Bool) throws {
        if !cr0.isPagingEnabled {
            tlbUpdate(linear: linear & ~0xFFF, with: linear & ~0xFFF, writable: true, user: false)
            return
        }
        /// paging enabled
        var errorCode: DWord = 0
        let pdeAddress = cr3.pageDirectoryBase &+ linear.pageDirectoryOffset
        var pde: PageTableEntry = memory.ld(from: pdeAddress)
        if pde.isPresent {
            let pteAddress = pde.pageFrameAddress &+ linear.pageTableOffset
            var pte: PageTableEntry = memory.ld(from: pteAddress)
            if pde.isPresent {
                let supervisor = !user
                let pxe = pde & pte
                /// error: user request and page not user-accessible
                if user && !pxe.isUser {
                    errorCode.setFlag(.P)
                }
                /// error: writable request and page not writable and not supervisor or (supervisor and) WP is set
                if writable && !pxe.isWritable && (!supervisor || cr0.isFlagRaised(.WP)) {
                    errorCode.setFlag(.P)
                }
                if errorCode == 0 {
                    if !pde.accessed {
                        pde.setFlag(.A)
                        memory.st(at: pdeAddress, dword: pde)
                    }
                    let isBlankPage = writable && !pte.isDirty
                    if isBlankPage && !pte.accessed {
                        pte.setFlag(.A)
                        if isBlankPage {
                            pte.setFlag(.D)
                        }
                        memory.st(at: pteAddress, dword: pte)
                    }
                    var wFlag = false
                    var uFlag = false
                    if pte.isDirty && (pxe.isWritable || supervisor) {
                        wFlag = true
                    }
                    if pxe.isUser {
                        uFlag = true
                    }
                    tlbUpdate(linear: linear & ~0xFFF, with: pte & ~0xFFF, writable: wFlag, user: uFlag)
                    return
                }
            }
        }
        /// page fault
        cr2 = linear
        if writable {
            errorCode.setFlag(.W)
        }
        if user {
            errorCode.setFlag(.U)
        }
        throw Interrupt(.PF, errorCode: errorCode)
    }
}
