typealias InstructionPrefixRegister = DWord

enum InstructionPrefixRegisterFlag: Int {
    case repzStringOperation = 4
    case repnzStringOperation
    case lockSignal
    case operandSizeOverride
    case addressSizeOverride
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
    var segmentRegisterIndex: Int {
        guard
            self & 7 > 0
        else { return SegmentRegister.Name.DS.rawValue }
        return Int(self & 7 - 1)
    }
    var operandSizeMask: DWord {
        self.isFlagRaised(.operandSizeOverride) ? 0xFFFF : 0xFFFFFFFF
    }
    var segmentOverride: Bool {
        self & 7 > 0
    }
}
