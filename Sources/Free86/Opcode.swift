typealias Opcode = DWord

extension Opcode {
    var override: Bool {
        get { self.isBitRaised(8) }
        set { self.setBit(8, newValue ? 1 : 0) }
    }
    var isEven: Bool {
        !isOdd
    }
    var isOdd: Bool {
        self & 1 == 1
    }
}
