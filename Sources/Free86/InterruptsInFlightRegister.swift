typealias InterruptsInFlightRegister = DWord

enum InterruptsInFlightRegisterFlag: Int, CaseIterable {
    case `internal`  // in priority order
    case NMI
    case INTR
    case current  // pseudo flag
    case DF = 8   // double fault
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
    func noHigherPriority(than flag: InterruptsInFlightRegisterFlag) -> Bool {
        var result = true
        for higherPriorityInterrupt in InterruptsInFlightRegisterFlag.allCases {
            if higherPriorityInterrupt.rawValue >= flag.rawValue {
                break
            }
            if isFlagRaised(higherPriorityInterrupt) {
                result = false
            }
        }
        return result
    }
    var current: InterruptsInFlightRegisterFlag? {
        var result: InterruptsInFlightRegisterFlag? = nil
        if isFlagRaised(.internal) { result = .internal }
        if isFlagRaised(.NMI)  { result = .NMI }
        if isFlagRaised(.INTR) { result = .INTR }
        return result
    }
}
