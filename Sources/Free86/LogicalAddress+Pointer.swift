struct LogicalAddress {
    var selector: SegmentSelector
    var offset: NearPointer
}

extension LogicalAddress {
    var translation: LinearAddress {
        0
    }
}

// PM (1986), 2.2, Intel IA-32 SDM (latest), Vol. 3A, fig. 3-1
typealias FarPointer = LogicalAddress
typealias NearPointer = DWord
