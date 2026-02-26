typealias ModRM = Byte

extension ModRM {
    var mod: Int {
        Int(self >> 6)
    }
    var reg: Int {
        Int(self >> 3 & 0b0111)
    }
    var opcode: Int {
        reg
    }
    var rM: Int {
        Int(self & 0b0111)
    }
    var modRM: Self {
        (self & 7) | ((self >> 3) & 0x18)
    }
}

typealias SIB = Byte

extension SIB {
    var scale: Int {
        Int(self >> 6)
    }
    var index: Int {
        Int(self >> 3 & 0b0111)
    }
    var base: Int {
        Int(self & 0b0111)
    }
}
