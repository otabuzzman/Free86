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
}

extension InstructionPrefixRegister {
    var segmentRegisterIndex: Int {
        guard
            self & 0x7 > 0
        else { return SegmentRegister.Name.DS.rawValue }
        return Int(self & 0x07 - 1)
    }
}
