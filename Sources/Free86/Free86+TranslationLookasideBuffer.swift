extension Free86: TranslationLookasideBuffer {
    func tlbLookup(linear: LinearAddress, writable: Bool) throws -> DWord {
        var hash: Int
        let pxi = linear.pageTablesIndices  // PDE and PTE indices
        if (writable) {
            hash = tlbWritable[pxi]
        } else {
            hash = tlbReadOnly[pxi]
        }
        if (hash == -1) {
            try translate(linear, writable: writable, user: cpl == 3)
            if (writable) {
                hash = tlbWritable[pxi]
            } else {
                hash = tlbReadOnly[pxi]
            }
        }
        return linear ^ DWord(hash)
    }

    func tlbUpdate(linear: LinearAddress, with address: DWord, writable: Bool, user: Bool) {
        let hash = Int(linear ^ address)    // XOR hash function (by design, no necessity)
        let pxi = linear.pageTablesIndices  // PD and PT indices (top 20 bits of linear address)
        if (tlbReadOnlyCplX[pxi] == -1) {
            if (tlbPagesCount >= 2048) {  // flush TLB if full
                /// if present, keep PTE immediately preceding this PTE to improve performance
                tlbFlush(preservePageIfPresent: pxi - 1)
            }
            /// record LA just added to TLB
            tlbPages[tlbPagesCount] = pxi
            tlbPagesCount += 1
        }
        /// update mapping tables
        tlbReadOnlyCplX[pxi] = hash
        if (writable) {
            tlbWritableCplX[pxi] = hash
        } else {
            tlbWritableCplX[pxi] = -1
        }
        if (user) {
            tlbReadOnlyCpl3[pxi] = hash;
            if (writable) {
                tlbWritableCpl3[pxi] = hash;
            } else {
                tlbWritableCpl3[pxi] = -1
            }
        } else {
            tlbReadOnlyCpl3[pxi] = -1
            tlbWritableCpl3[pxi] = -1
        }
    }

    func tlbFlush() {
        for i in 0..<tlbPagesCount {
            tlbClear(tlbPages[i])
        }
        tlbPagesCount = 0
    }

    func tlbFlush(pageContainingAddress linear: LinearAddress) {
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
        tlbReadOnlyCplX[pxi] = -1
        tlbWritableCplX[pxi] = -1
        tlbReadOnlyCpl3[pxi] = -1
        tlbWritableCpl3[pxi] = -1
    }
}

extension UnsafeMutablePointer {
    subscript(index: LinearAddress) -> Pointee {
        get { self[Int(index)] }
        set { self[Int(index)] = newValue }
    }
}
