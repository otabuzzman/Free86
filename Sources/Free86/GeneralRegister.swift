typealias GeneralRegister = DWord

extension GeneralRegister {
    enum Name: Int {
        case EAX
        case ECX
        case EDX
        case EBX
        case ESP
        case EBP
        case ESI
        case EDI
    }
    var byteL: Self {
        get { self & 0xFF }
        set { self = (self & ~0xFF) | (newValue & 0xFF) }
    }
    var byteH: Self {
        get { (self & 0x0000_FF00) >> 8 }
        set { self = (self & ~0x0000_FF00) | (newValue & 0xFF) << 8 }
    }
    var wordX: Self {
        get { self.lowerHalf }
        set { self.lowerHalf = newValue & Self.lowerHalfMask }
    }
}

extension Array where Element == GeneralRegister {
    subscript (_ register: GeneralRegister.Name) -> Element {
        get { self[register.rawValue] }
        set { self[register.rawValue] = newValue }
    }
}
