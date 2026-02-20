extension Free86 {
    func auxJmpf(_ selector: SegmentSelector, _ offset: DWord) {
    }
    func auxJmpfRealOrV86Mode(_ selector: SegmentSelector, _ offset: DWord) {
    }
    func auxJmpfProtectedMode(_ selector: SegmentSelector, _ offset: DWord) {
    }
    func auxCallf(_ o32: Bool, _ selector: SegmentSelector, _ offset: DWord, _ returnAddress: DWord) {
    }
    func auxCallfRealOrV86Mode(_ o32: Bool, _ selector: SegmentSelector, _ offset: DWord, _ returnAddress: DWord) {
    }
    func auxCallfProtectedMode(_ o32: Bool, _ selector: SegmentSelector, _ offset: DWord, _ returnAddress: DWord) {
    }
    func auxRetf(_ o32: Bool, _ releaseStackItems: DWord) {
    }
    func returnRealOrV86Mode(_ o32: Bool, _ isIret: Bool, _ releaseStackItems: DWord) {
    }
    func returnProtectedMode(_ o32: Bool, _ isIret: Bool, _ releaseStackItems: DWord) {
    }
    func resetSegmentRegister(_ sreg: SegmentRegister.Name, _ level: DWord) {
    }
    func raiseInterrupt(_ id: Int, _ errorCode: Int, _ isHW: Bool, _ isSW: Bool, _ returnAddress: DWord) {
    }
    func raiseInterruptRealOrV86Mode(_ id: Int, _ isSW: Bool, _ returnAddress: DWord) {
    }
    func raiseInterruptProtectedMode(_ id: Int, _ errorCode: Int, _ isHW: Bool, _ isSW: Bool, _ returnAddress: DWord) {
    }
    func ldXdtEntry(_ selector: SegmentSelector) {
    }
    func ldTssStack(_ level: DWord) { // seg:offset
    }
    func auxIret(_ o32: Bool) {
    }
}
