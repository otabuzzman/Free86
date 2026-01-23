typealias LinearAddress = DWord

extension LinearAddress {
    var pageTablesIndices: Self {
        self >> 12
    }
    var pageDirectoryOffset: Self {
        self >> 20 & 0xFFC
    }
    var pageTableOffset: Self {
        self >> 10 & 0xFFC
    }
    var offset: Self {
        self & 0xFFF
    }
}
