protocol Cr0 { }

enum Cr0Flag: Int {
    case PE
    case MP
    case EM
    case TS
    case ET
    case PG = 31
}

extension DWord: Cr0 { }

extension Cr0 where Self == DWord {
    func isFlagRaised(_ flag: Cr0Flag) -> Bool {
        self.isBitRaised(flag.rawValue)
    }
    mutating func setFlag(_ flag: Cr0Flag, _ value: Int = 1) {
        self.setBit(flag.rawValue, value)
    }
}
