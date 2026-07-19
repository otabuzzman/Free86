typealias InterruptsInFlightRegister = DWord

enum InterruptsInFlightRegisterFlag: Int {
    case FV  = 7
    case NMI = 10  // NMI id (2) << 8
}

extension InterruptsInFlightRegister {
    func isFlagRaised(_ flag: InterruptsInFlightRegisterFlag) -> Bool {
        self.isBitRaised(flag.rawValue)
    }
    mutating func setFlag(_ flag: InterruptsInFlightRegisterFlag, _ value: Int = 1) {
        self.setBit(flag.rawValue, value)
    }
    static func maskFlag(_ flag: InterruptsInFlightRegisterFlag) -> Self {
        Self.bitMask(for: flag.rawValue)
    }
}

extension InterruptsInFlightRegister {
    var fex: Int {
        get { Int(self & 0x0000001f) }
        set { self = (self & ~0x0000001f) | Self(newValue) }
    }
}
