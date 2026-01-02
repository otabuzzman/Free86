typealias Byte = UInt8
typealias Word = UInt16
typealias DWord = UInt32
typealias QWord = UInt64

extension FixedWidthInteger where Self: UnsignedInteger {
    static var halfBitWidth: Int {
        Self.bitWidth / 2
    }
    static var upperHalfMask: Self {
        ~lowerHalfMask
    }
    static var lowerHalfMask: Self {
        (Self(1) << halfBitWidth) - 1
    }
    var upperHalf: Self {
        get { self >> Self.halfBitWidth }
        set { self = (self & Self.lowerHalfMask) | (newValue << Self.halfBitWidth) }
    }
    var lowerHalf: Self {
        get { self & Self.lowerHalfMask }
        set { self = (newValue & Self.lowerHalfMask) | (self & Self.upperHalfMask) }
    }
    static func bitMask(for position: Int) -> Self {
        return Self(1) << position
    }
    mutating func setBit(_ position: Int, _ value: Int = 1) {
        value == 1 ? raiseBit(position) : clearBit(position)
    }
    mutating func raiseBit(_ position: Int) {
        self |= Self.bitMask(for: position)
    }
    mutating func clearBit(_ position: Int) {
        self &= ~Self.bitMask(for: position)
    }
    mutating func toggleBit(_ position: Int) {
        self ^= Self.bitMask(for: position)
    }
    func isBitRaised(_ position: Int) -> Bool {
        (self & Self.bitMask(for: position)) != 0
    }
}
