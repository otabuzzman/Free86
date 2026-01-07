protocol LinearAddress {
    var pageTablesIndices: Self { get }
    var pageDirectoryIndex: Self { get }
    var pageTableIndex: Self { get }
    var offset: Self { get }
}

extension DWord: LinearAddress {
    var pageTablesIndices: Self {
        self >> 12
    }
    var pageDirectoryIndex: Self {
        self >> 22
    }
    var pageTableIndex: Self {
        self >> 12 & 0x3FF
    }
    var offset: Self {
        self & 0xFFF
    }
}
