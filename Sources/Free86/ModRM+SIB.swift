protocol ModRM {
    var mod: Int { get }
    var reg: Int { get }
    var opcode: Int { get }
    var rM: Int { get }
}

extension Byte: ModRM {
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
}

protocol SIB {
    var scale: Int { get }
    var index: Int { get }
    var base: Int { get }
}

extension Byte: SIB {
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
