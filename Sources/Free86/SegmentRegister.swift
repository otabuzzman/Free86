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
    private(set) var hidden = SegmentDescriptor(0)  // aka descriptor cache
    init(_ selector: SegmentSelector, _ descriptor: SegmentDescriptor) {
        self.selector = selector
        self.hidden = descriptor
    }
}

extension SegmentRegister: Equatable {
    static func ==(lhs: SegmentRegister, rhs: SegmentRegister) -> Bool {
        lhs.selector == rhs.selector && lhs.hidden == rhs.hidden
    }
}

extension Array where Element == SegmentRegister {
    subscript (_ register: SegmentRegister.Name) -> Element {
        get { self[register.rawValue] }
        set { self[register.rawValue] = newValue }
    }
}
