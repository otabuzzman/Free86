struct LogicalAddress {
    var selector: SegmentSelector
    var offset: NearPointer
}

/// PM (1986), 2.2, Intel 64 IA-32 SDM (latest), Vol. 3A, fig. 3-1
typealias FarPointer = LogicalAddress
typealias NearPointer = DWord
