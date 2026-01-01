extension DWord: PageTableEntry {}

protocol PageTableEntry {
    var pageFrameAddress: DWord { get }
}

enum PageTableEntryFlag: Int {
    case P = 0
    case W = 1
    case U = 2
    case A = 5
    case D = 6
}

extension PageTableEntry where Self == DWord {
    var pageFrameAddress: Self {
        self >> 12
    }
}

// page table entry flags
extension PageTableEntry where Self == DWord {
    func isFlagRaised(_ flag: PageTableEntryFlag) -> Bool {
        self.isBitRaised(flag.rawValue)
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
    mutating func setFlag(_ flag: PageTableEntryFlag, _ value: Int = 1) {
        self.setBit(flag.rawValue, value)
    }
}
