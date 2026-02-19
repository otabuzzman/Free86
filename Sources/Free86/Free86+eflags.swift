extension Free86 {
    func isCF() -> Bool {  // carry (bit 0)
        false
    }
    func isPF() -> Bool {  // parity (bit 2)
        false
    }
    func isAF() -> Bool {  // adjust (bit 4)
        false
    }
    func isOF() -> Bool {  // overflow (bit 11)
        false
    }
    func isBE() -> Bool {  // below or equal, signed comparison
        false
    }
    func isLE() -> Bool {  // less or equal, unsigned comparison
        false
    }
    func isLT() -> Bool {  // less than
        false
    }
    func canJmp(int condition: Int) -> Bool {
        false
    }
    func compileEflags(_ shift: Bool = false) -> DWord {
        0
    }
    func getEflags() -> DWord {
        0
    }
    func setEflags(_ bits: DWord, _ mask: DWord) {
    }
}
