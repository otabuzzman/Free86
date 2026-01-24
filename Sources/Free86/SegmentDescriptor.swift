struct SegmentDescriptor {
    var upper: DWord = 0
    var lower: DWord = 0
}

extension SegmentDescriptor {
    init(_ base: DWord, _ limit: DWord, _ type: SegmentDescriptorType, _ dpl: Int) {
        assert(dpl >= 0 && dpl <= 3)
        self.base  = base
        self.limit = limit
        self.type = type.rawValue
        self.dpl = dpl
    }
}

enum SegmentDescriptorFlag: Int {
    case G = 23  // granularity, 1 = limit in 4 kB units
    case DB = 22  // D flag if code segment, 0 = 16 bit addresses and 16/ 8 bit operands, 1 = 32 bit addresses and 32/ 8 bit operands
                  // B flag if data segment and expand down, 0 = 0xFFFF upper bound, 1 = 0xFFFFFFFF upper bound
    case P = 15  // 1 = segment is present
    case S = 12  // 0 = system segment
}

/// segment descriptor fields
extension SegmentDescriptor {
    var base: DWord {
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
    var type: DWord {
        get { (upper & 0x0000_1F00) >> 8 }
        set { upper = upper & 0xFFFF_E0FF | DWord(newValue) << 8 }
    }
    var dpl: Int {
        get {
            Int((upper & 0x0000_6000) >> 13 & 0b0011)
        }
        set {
            assert(newValue >= 0 && newValue <= 3)
            upper = upper & 0xFFFF_9FFF | DWord(newValue << 13)
        }
    }
}

/// segment descriptor main types
extension SegmentDescriptor {
    var isSystemSegment: Bool {
        !isFlagRaised(.S)
    }
    var isDataSegment: Bool {
        !isSystemSegment && type >> 3 == 0
    }
    var isCodeSegment: Bool {
        !isSystemSegment && type >> 3 == 1
    }
}

/// segment descriptor flags
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
    // Reserved                         0b0_0000
    case TSS16Available               = 0b0_0001
    case LDT                          = 0b0_0010
    case TSS16Busy                    = 0b0_0011
    case CallGate16                   = 0b0_0100
    case TaskGate                     = 0b0_0101
    case InterruptGate16              = 0b0_0110
    case TrapGate16                   = 0b0_0111
    // Reserved                         0x0_1000
    case TSSAvailable                 = 0b0_1001
    // Reserved                         0x0_1010
    case TSSBusy                      = 0b0_1011
    case CallGate                     = 0b0_1100
    // Reserved                         0x0_1101
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

/// segment descriptor types
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
