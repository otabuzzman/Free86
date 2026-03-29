typealias ModRM = Byte

extension ModRM {
    var mod: Int {
        Int(self >> 6)
    }
    var reg: Int {
        Int(self >> 3 & 0b0111)
    }
    var rM: Int {
        Int(self & 0b0111)
    }
}

extension ModRM {
    var opcode: Int {
        reg
    }
    var modRM: Self {
        Self(rM | mod << 3)
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
