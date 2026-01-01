extension DWord: LinearAddress {}

protocol LinearAddress {
    var pageDirectoryIndex: Self { get }
    var pageTableIndex: Self { get }
    var pageOffset: Self { get }
}

extension LinearAddress where Self == DWord {
    var pageDirectoryIndex: Self {
        self >> 22
    }
    var pageTableIndex: Self {
        self >> 12 & 0x3FF
    }
    var pageOffset: Self {
        self & 0xFFF
    }
}
