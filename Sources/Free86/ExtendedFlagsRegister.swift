typealias Eflags = DWord

enum EflagsFlag: Int {
    case CF = 0
    case PF = 2
    case AF = 4
    case ZF = 6
    case SF = 7
    case TF = 8  // systems flag
    case IF = 9  // systems flag
    case DF = 10
    case OF = 11
    case NT = 14  // systems flag
    case RF = 16  // systems flag
    case VM = 17  // systems flag
}

extension Eflags {
    func isFlagRaised(_ flag: EflagsFlag) -> Bool {
        self.isBitRaised(flag.rawValue)
    }
    mutating func setFlag(_ flag: EflagsFlag, _ value: Int = 1) {
        self.setBit(flag.rawValue, value)
    }
}

extension Eflags {
    var iopl: Int {
        get { Int((self & 0x00003000) >> 12) }
        set { self = (self & ~0x00003000) | (Self(newValue) << 12) }
    }
}
