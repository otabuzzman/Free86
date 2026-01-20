typealias LinearAddress = DWord

extension LinearAddress {
    var pageTablesIndices: Self {
        self >> 12
    }
    var pageDirectoryIndex: Self {
        self >> 20 & 0xFFC
    }
    var pageTableIndex: Self {
        self >> 10 & 0xFFC
    }
    var offset: Self {
        self & 0xFFF
    }
}
