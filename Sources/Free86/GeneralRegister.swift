typealias GeneralRegister = DWord

extension GeneralRegister {
    enum Name: Int {
        case EAX  // AL
        case ECX  // CL
        case EDX  // DL
        case EBX  // BL
        case ESP  // AH
        case EBP  // CH
        case ESI  // DH
        case EDI  // BH
    }
    var byteL: Self {
        get { self & 0xFF }
        set { self = (self & ~0xFF) | (newValue & 0xFF) }
    }
    var byteH: Self {
        get { (self & 0x0000_FF00) >> 8 }
        set { self = (self & ~0x0000_FF00) | (newValue & 0xFF) << 8 }
    }
    var byteLH: Self {
        get {
            if isLByteEncoded {
                return byteL
            } else {
                return byteH
            }
        set {
            if isLByteEncoded {
                byteL = newValue
            } else {
                byteH = newValue
            }
    }
    var wordX: Self {
        get { self.lowerHalf }
        set { self.lowerHalf = newValue & Self.lowerHalfMask }
    }
    var isHByteEncoded: Bool {
        self & 0b0100 != 0
    }
    var isLByteEncoded: Bool {
        !isHByteEncoded
    }
}

extension Array where Element == GeneralRegister {
    subscript (_ register: GeneralRegister.Name) -> Element {
        get { self[register.rawValue] }
        set { self[register.rawValue] = newValue }
    }
}

extension FixedWidthInteger {
    func isGeneralRegister(_ name: GeneralRegister.Name) -> Bool {
        self == (name.rawValue & 0b111) as! Self
    }
}
