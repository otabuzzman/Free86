struct InterruptsInFlightRegister {
    enum Name: Int, CaseIterable {  // in priority order
        case `internal`
        case NMI
        case INTR
        case current  // pseudo interrupt
    }
    var storage: [InterruptsInFlightRegister.Name: Int] = [
        .`internal`: 0,
        .NMI:  0,
        .INTR: 0
    ]
}

/// all force-unwraps save
extension InterruptsInFlightRegister {
    func isRaised(_ name: InterruptsInFlightRegister.Name) -> Bool {
        guard name != .current else { return false }
        return storage[name]! > 0
    }
    mutating func increment(_ name: InterruptsInFlightRegister.Name) {
        var effective = name
        if name == .current {
            guard let current = current else { return }
            effective = current
        }
        storage[effective]! += 1
    }
    mutating func decrement(_ name: InterruptsInFlightRegister.Name) {
        var effective = name
        if name == .current {
            guard let current = current else { return }
            effective = current
        }
        storage[effective]! -= 1
    }
    private var current: InterruptsInFlightRegister.Name? {
        var result = nil
        if isRaised(.internal) { result = .internal }
        if isRaised(.NMI)  { result = .NMI }
        if isRaised(.INTR) { result = .INTR }
        return result
    }
}

extension InterruptsInFlightRegister {
    func noHigherPriority(than name: InterruptsInFlightRegister.Name) -> Bool {
        var result = true
        for higherPriorityInterrupt in Name.allCases {
            if higherPriorityInterrupt.rawValue >= name.rawValue {
                break
            }
            if isRaised(higherPriorityInterrupt) {
                result = false
            }
        }
        return result
    }
}
