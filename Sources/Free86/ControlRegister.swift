public typealias CR0 = DWord
public typealias CR3 = DWord
typealias CR4 = DWord

enum CR0Flag: Int {
    case PE
    case MP
    case EM
    case TS
    case ET
    case WP = 16  // 80486
    case PG = 31
}

extension CR0 {
    func isFlagRaised(_ flag: CR0Flag) -> Bool {
        self.isBitRaised(flag.rawValue)
    }
    mutating func setFlag(_ flag: CR0Flag, _ value: Int = 1) {
        self.setBit(flag.rawValue, value)
    }
}

extension CR0 {
    public var isPagingEnabled: Bool {
        isFlagRaised(.PG) && isProtectedMode
    }
    var isProtectedMode: Bool {
        isFlagRaised(.PE)
    }
    var isRealOrV86Mode: Bool {
        !isProtectedMode
    }
}

extension CR3 {
    var pageDirectoryBase: DWord {
        self & ~0xFFF
    }
}

enum CR4Flag: Int {
    case TSD = 2  // 80486
}

extension CR4 {
    func isFlagRaised(_ flag: CR4Flag) -> Bool {
        self.isBitRaised(flag.rawValue)
    }
}
