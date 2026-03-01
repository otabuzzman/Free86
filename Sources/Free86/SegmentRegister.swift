struct SegmentRegister {
    enum Name: Int {
        case ES
        case CS
        case SS
        case DS
        case FS
        case GS
        case LDT  // holds 16-bit selector and cache for LDT
        case TR   // holds 16-bit selector and cache for TSS
    }
    var selector: SegmentSelector = 0
    private(set) var shadow = SegmentDescriptor(0)  // aka descriptor cache
    init(_ selector: SegmentSelector, _ descriptor: SegmentDescriptor) {
        self.selector = selector
        self.shadow = descriptor
    }
}

extension SegmentRegister: Equatable {
    static func == (lhs: SegmentRegister, rhs: SegmentRegister) -> Bool {
        lhs.selector == rhs.selector && lhs.shadow == rhs.shadow
    }
}

extension Array where Element == SegmentRegister {
    subscript (_ register: SegmentRegister.Name) -> Element {
        get { self[register.rawValue] }
        set { self[register.rawValue] = newValue }
    }
}

extension FixedWidthInteger {
    func isSegmentRegister(_ name: SegmentRegister.Name) -> Bool {
        self == (name.rawValue & 0b111) as! Self
    }
}
