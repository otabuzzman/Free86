protocol GeneralRegister {
    var byteL: Self { get set }
    var byteH: Self { get set }
    var wordX: Self { get set }
}

extension DWord: GeneralRegister {
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
