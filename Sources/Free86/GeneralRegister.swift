public typealias GeneralRegister = DWord

extension GeneralRegister {
    public enum Name: Int {
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
    var encodedByte: Self {
        get {
            if isByteLEncoded {
                return byteL
            } else {
                return byteH
            }
        }
        set {
            if isByteLEncoded {
                byteL = newValue
            } else {
                byteH = newValue
            }
        }
    }
    var isByteHEncoded: Bool {
        self & 0b0100 != 0
    }
    var isByteLEncoded: Bool {
        !isByteHEncoded
    }
}

extension Array where Element == GeneralRegister {
    public subscript (_ register: GeneralRegister.Name) -> Element {
        get { self[register.rawValue] }
        set { self[register.rawValue] = newValue }
    }
}

extension FixedWidthInteger {
    func isGeneralRegister(_ name: GeneralRegister.Name) -> Bool {
        self == (name.rawValue & 0b111) as! Self
    }
}
