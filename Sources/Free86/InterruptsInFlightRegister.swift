typealias InterruptsInFlightRegister = DWord

enum InterruptsInFlightRegisterFlag: Int {
    case `internal`  // in priority order
    case NMI
    case INTR
    case contributory = 4  // DE, TS, NM, SS, or GP ISR executing
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
    var current: InterruptsInFlightRegisterFlag? {
        if isFlagRaised(.internal) { return .internal }
        if isFlagRaised(.NMI)  { return .NMI }
        if isFlagRaised(.INTR) { return .INTR }
        return nil
    }
}
