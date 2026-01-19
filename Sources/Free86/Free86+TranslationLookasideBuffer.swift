/// TLB
extension Free86 {
    func lookup(linear: LinearAddress, writable: Bool) -> DWord {
        var hash: Int
        let pxi = linear.pageTablesIndices  // PDE and PTE indices
        if (writable) {
            hash = tlbWritable[pxi]
        } else {
            hash = tlbReadOnly[pxi]
        }
        if (hash == -1) {
            translate(linear, writable: writable, user: cpl == 3)
            if (writable) {
                hash = tlbWritable[pxi]
            } else {
                hash = tlbReadOnly[pxi]
            }
        }
        return linear ^ DWord(hash)
    }

    func update(linear: LinearAddress, with address: DWord, writable: Bool, user: Bool) {
        let hash = Int(linear ^ address)  // poor man's XOR hash function (by design, no necessity)
        let pxi = linear.pageTablesIndices  // PD and PT indices (top 20 bits of linear address)
        if (tlbReadOnlyCplX[pxi] == -1) {
            if (tlbPagesCount >= 2048) {  // flush TLB if full
                /// if present, keep PTE immediately preceding this PTE to improve performance
                flush(preservePageIfPresent: (pxi - 1) & 0xfffff)
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

    func flush() {
        for i in 0..<tlbPagesCount {
            clear(tlbPages[i])
        }
        tlbPagesCount = 0
    }

    func flush(pageContainingAddress linear: LinearAddress) {
        clear(linear.pageTablesIndices)
    }

    private func flush(preservePageIfPresent pxi: DWord) {
        var n = 0
        for i in 0..<tlbPagesCount {
            let _pxi = tlbPages[i]
            if _pxi == pxi {
                tlbPages[n] = _pxi
                n += 1
            } else {
                clear(_pxi)
            }
        }
        tlbPagesCount = n
    }

    private func clear(_ pxi: DWord) {
        tlbReadOnlyCplX[pxi] = -1
        tlbWritableCplX[pxi] = -1
        tlbReadOnlyCpl3[pxi] = -1
        tlbWritableCpl3[pxi] = -1
    }
}

/// page translation
extension Free86 {
    func translate(_ linear: LinearAddress, writable: Bool, user: Bool) {
    }
}

extension UnsafeMutablePointer {
    subscript(index: LinearAddress) -> Pointee {
        get { self[Int(index)] }
        set { self[Int(index)] = newValue }
    }
}
