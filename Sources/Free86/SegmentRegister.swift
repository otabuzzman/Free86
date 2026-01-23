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
    var descriptorCache = SegmentDescriptor()
}

extension SegmentRegister: Equatable {
    static func == (lhs: SegmentRegister, rhs: SegmentRegister) -> Bool {
        lhs.selector == rhs.selector && lhs.descriptorCache == rhs.descriptorCache
    }
}

extension Array where Element == SegmentRegister {
    subscript (_ register: SegmentRegister.Name) -> Element {
        get { self[register.rawValue] }
        set { self[register.rawValue] = newValue }
    }
}
