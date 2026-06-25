public typealias InterruptsInFlightRegister = DWord

enum InterruptsInFlightRegisterFlag {
    case `internal`
    case NMI
    case INTR
    case software
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
    func isActivePriority(_ flag: InterruptsInFlightRegisterFlag) -> Bool {
        guard flag.rawValue > 0 else { return false }
        return self & ((1 << flag.rawValue) - 1) != 0
    }
}
