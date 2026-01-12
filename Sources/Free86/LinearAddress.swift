typealias LinearAddress = DWord

extension LinearAddress {
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

extension LinearAddress {
    var translation: any PhysicalAddress {
        DWord(0)
    }
}
