typealias Opcode = DWord

extension Opcode {
    var override: Bool {
        get { self.isBitRaised(8) }
        set { self.setBit(8, newValue ? 1 : 0) }
    }
    var isEven: Bool {
        !isOdd
    }
    var isOdd: Bool {
        self & 1 == 1
    }
}

enum OpcodeEncodedField: Int {
    case operation
    case condition
    case generalRegister
    case segmentRegister
    case standardSegmentRegister  // ES, CS, DS, SS
    case extendedSegmentRegister  // FS, GS
    case coProcessorOpcode
}

extension Opcode {
    func encoded(_ field: OpcodeEncodedField) -> Int {
        switch field {
        case .operation:
            return Int(self >> 3 & 7)
        case .condition:
            return Int(self & 0xf)
        case .generalRegister:
            return Int(self & 7)
        case .segmentRegister:
            return Int(self & 7)
        case .standardSegmentRegister:
            return Int(self >> 3 & 7)
        case .extendedSegmentRegister:
            return Int(self >> 3 & 7)
        case .coProcessorOpcode:
            return Int(self & 7)
        }
    }
}
