typealias InstructionPrefixRegister = DWord

enum InstructionPrefixRegisterFlag: Int {
    case repzStringOperation = 4
    case repnzStringOperation
    case lockSignal
    case addressSizeOverride
    case operandSizeOverride
}

extension InstructionPrefixRegister {
    func isFlagRaised(_ flag: InstructionPrefixRegisterFlag) -> Bool {
        self.isBitRaised(flag.rawValue)
    }
    mutating func setFlag(_ flag: InstructionPrefixRegisterFlag, _ value: Int = 1) {
        self.setBit(flag.rawValue, value)
    }
    static func maskFlag(_ flag: InstructionPrefixRegisterFlag) -> Self {
        Self.bitMask(for: flag.rawValue)
    }
}

extension InstructionPrefixRegister {
    var segmentRegister: Int {
        get {
            guard
                self & 7 > 0
            else { return SegmentRegister.Name.DS.rawValue }
            return Int(self) & 7 - 1
        }
        set {
            assert((0..<7).contains(newValue), "fatal error")
            self = Self(truncatingIfNeeded: newValue + 1)
        }
    }
    var addressSizeMask: DWord {
        self.isFlagRaised(.addressSizeOverride) ? 0xFFFF : 0xFFFFFFFF
    }
    var segmentOverride: Bool {
        self & 7 > 0
    }
}
