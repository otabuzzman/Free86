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
    func maskFlag(_ flag: InstructionPrefixRegisterFlag) -> Self {
        Self.bitMask(for: flag.rawValue)
    }
}

extension InstructionPrefixRegister {
    var segmentRegisterIndex: Int {
        assert(7 > self & 7) // ES..GS are 1..6
        guard
            self & 0x7 > 0
        else { return SegmentRegister.Name.DS.rawValue }
        return Int(self & 0x07 - 1)
    }
}
