struct SegmentRegister {
    enum Name: Int {
        case ES
        case CS
        case SS
        case DS
        case FS
        case GS
        case LDT // holds 16-bit selector and cache for LDT
        case TR // holds 16-bit selector and cache for TSS
    }
    var selector: SegmentSelector
    var descriptorCache: SegmentDescriptor
}
