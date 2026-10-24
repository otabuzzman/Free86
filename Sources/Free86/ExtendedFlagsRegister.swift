public typealias ExtendedFlagsRegister = DWord

enum ExtendedFlagsRegisterFlag: Int {
    case CF = 0
    case PF = 2
    case AF = 4
    case ZF = 6
    case SF = 7
    case TF = 8   // systems flag
    case IF = 9   // systems flag
    case DF = 10  // control flag
    case OF = 11
    case NT = 14  // systems flag
    case RF = 16  // systems flag
    case VM = 17  // systems flag
    case AC = 18   // 80486
    case VIF = 19  // 80486
    case VIP = 20  // 80486
    case ID = 21   // 80486
}

extension ExtendedFlagsRegister {
    func isFlagRaised(_ flag: ExtendedFlagsRegisterFlag) -> Bool {
        self.isBitRaised(flag.rawValue)
    }
    mutating func setFlag(_ flag: ExtendedFlagsRegisterFlag, _ value: Int = 1) {
        self.setBit(flag.rawValue, value)
    }
    static func flagMask(for flag: ExtendedFlagsRegisterFlag) -> Self {
        return Self.bitMask(for: flag.rawValue)
    }
}

extension ExtendedFlagsRegister {
    var iopl: Int {
        get { Int((self & 0x00003000) >> 12) }
        set { self = (self & ~0x00003000) | Self(newValue) << 12 }
    }
}
