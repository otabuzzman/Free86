struct SegmentDescriptor {
    var upper: DWord = 0
    var lower: DWord = 0
    var qword: QWord {
        QWord(upper) << 32 | QWord(lower)
    }
    init(_ qword: QWord) {
        upper = DWord(qword >> 32)
        lower = DWord(truncatingIfNeeded: qword)
    }
}

extension SegmentDescriptor {
    init(_ base: LinearAddress, _ limit: DWord, _ type: SegmentDescriptorType, _ dpl: DWord) {
        assert(dpl >= 0 && dpl <= 3, "fatal error")
        self.base  = base
        self.limit = limit
        self.type = type.rawValue
        self.dpl  = dpl
    }
}

enum SegmentDescriptorFlag: Int {
    /// all descriptor types
    case G = 23  // granularity, 1 = limit in 4 kB units
    case D = 22  // D (default)/ B (big) flag if code/ data segment, 0 = 16 bit
    case P = 15  // 1 = segment is present
    case S = 12  // 0 = system segment
    /// TSS
    case B = 9   // 1 = TSS busy
}

extension SegmentDescriptor {
    var base: LinearAddress {
        get {
            (lower & 0xFFFF_0000) >> 16 |
            (upper & 0x0000_00FF) << 16 |
             upper & 0xFF00_0000
        }
        set {
            lower = lower & 0x0000_FFFF | newValue << 16
            upper = upper & 0x00FF_FF00 | newValue >> 16 & 0x0000_00FF | newValue & 0xFF00_0000
        }
    }
    var limit: DWord {
        get {
            let limit =
            lower & 0x0000_FFFF |
            upper & 0x000F_0000
            return isFlagRaised(.G) ? limit << 12 | 0xFFF : limit
        }
        set {
            lower = lower & 0xFFFF_0000 | newValue & 0x0000_FFFF
            upper = upper & 0xFFF0_FFFF | newValue & 0x000F_0000
        }
    }
    var flags: DWord {
        upper & 0x00F0_FF00
    }
    var type: DWord {
        get { (upper & 0x0000_1F00) >> 8 }
        set { upper = upper & 0xFFFF_E0FF | DWord(newValue) << 8 }
    }
    var dpl: DWord {
        get {
            (upper & 0x0000_6000) >> 13 & 0b0011
        }
        set {
            assert(newValue >= 0 && newValue <= 3, "fatal error")
            upper = upper & ~0x0000_6000 | DWord(newValue << 13)
        }
    }
    var segmentSizeMask: DWord {
        self.isFlagRaised(.D) ? 0xFFFFFFFF : 0xFFFF
    }
}

extension SegmentDescriptor {
    var isSystemSegment: Bool {
        !isFlagRaised(.S)
    }
    var isDataSegment: Bool {
        !isSystemSegment && type >> 3 == 0b10
    }
    var isCodeSegment: Bool {
        !isSystemSegment && type >> 3 == 0b11
    }
}

extension SegmentDescriptor {
    func isFlagRaised(_ flag: SegmentDescriptorFlag) -> Bool {
        upper.isBitRaised(flag.rawValue)
    }
    mutating func setFlag(_ flag: SegmentDescriptorFlag, _ value: Int = 1) {
        upper.setBit(flag.rawValue, value)
    }
}

///combined S bit and type field
enum SegmentDescriptorType: DWord {
    case zero                         = 0b0_0000  // reserved
    case TSS16Available               = 0b0_0001
    case LDT                          = 0b0_0010
    case TSS16Busy                    = 0b0_0011
    case CallGate16                   = 0b0_0100
    case TaskGate                     = 0b0_0101
    case InterruptGate16              = 0b0_0110
    case TrapGate16                   = 0b0_0111
    // Reserved                         0b0_1000
    case TSSAvailable                 = 0b0_1001
    // Reserved                         0b0_1010
    case TSSBusy                      = 0b0_1011
    case CallGate                     = 0b0_1100
    // Reserved                         0b0_1101
    case InterruptGate                = 0b0_1110
    case TrapGate                     = 0b0_1111
    case DataRO                       = 0b1_0000
    case DataROAccessed               = 0b1_0001
    case DataRW                       = 0b1_0010
    case DataRWAccessed               = 0b1_0011
    case DataROExpandDown             = 0b1_0100
    case DataROExpandDownAccessed     = 0b1_0101
    case DataRWExpandDown             = 0b1_0110
    case DataRWExpandDownAccessed     = 0b1_0111
    case CodeExOnly                   = 0b1_1000
    case CodeExOnlyAccessed           = 0b1_1001
    case CodeExRead                   = 0b1_1010
    case CodeExReadAccessed           = 0b1_1011
    case CodeExOnlyConforming         = 0b1_1100
    case CodeExOnlyConformingAccessed = 0b1_1101
    case CodeExReadConforming         = 0b1_1110
    case CodeExReadConformingAccessed = 0b1_1111
}

extension SegmentDescriptor {
    func isType(_ type: SegmentDescriptorType) -> Bool {
        type.rawValue == self.type | (isSystemSegment ? 0b0_0000 : 0b1_0000)
    }
}

extension SegmentDescriptor: Equatable {
    static func == (lhs: SegmentDescriptor, rhs: SegmentDescriptor) -> Bool {
        lhs.upper == rhs.upper && lhs.lower == rhs.lower
    }
}
