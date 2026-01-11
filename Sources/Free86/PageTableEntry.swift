typealias PageTableEntry = DWord

enum PageTableEntryFlag: Int {
    case P = 0  // 1 = page is present
    case W = 1  // 1 = page is writable
    case U = 2  // 1 = page is user accessible
    case D = 6  // 1 = page has been written (dirty)
}

extension PageTableEntry {
    func isFlagRaised(_ flag: PageTableEntryFlag) -> Bool {
        self.isBitRaised(flag.rawValue)
    }
    mutating func setFlag(_ flag: PageTableEntryFlag, _ value: Int = 1) {
        self.setBit(flag.rawValue, value)
    }
}

extension PageTableEntry {
    var pageFrameAddress: Self {
        self >> 12
    }
    var isPresent: Bool {
        isFlagRaised(.P)
    }
    var isWritable: Bool {
        isFlagRaised(.W)
    }
    var isUser: Bool {
        isFlagRaised(.U)
    }
    var isDirty: Bool {
        isFlagRaised(.D)
    }
}
