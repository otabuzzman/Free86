typealias CR0 = DWord

enum CR0Flag: Int {
    case PE
    case MP
    case EM
    case TS
    case ET
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
    var isPagingEnabled: Bool {
        isFlagRaised(.PG)
    }
    var isProtectedMode: Bool {
        isFlagRaised(.PE)
    }
    var isRealOrV86Mode: Bool {
        !isProtectedMode
    }
}
