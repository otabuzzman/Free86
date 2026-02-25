extension Free86 {
    func segmentTranslation() {
    }
    func ldMemoryOffset(_ writable: Bool) throws {
        var la: QWord
        var notok: Bool
        var stride: DWord
        if !ipr.isFlagRaised(.addressSizeOverride) {
            la = QWord(fetch())
            stride = 4  // 32 bit mode
        } else {
            la = QWord(fetch16())
            stride = 2  // 16 bit mode
        }
        if (opcode & 0x01) != 0 {
            stride = 1  // 8 bit mode, opcodes A0, A2
        }
        let sreg = ipr.segmentRegisterIndex
        /// type checking
        if sreg == SegmentRegister.Name.CS.rawValue {  // code segment, WR requested or CS not readable
            notok = writable || !segs[sreg].shadow.isFlagRaised(.R)
        } else {  // data segment, WR requested and DS not writable
            notok = writable && !segs[sreg].shadow.isFlagRaised(.W)
        }
        if notok {
            throw Interrupt(.GP, errorCode: 0)
        }
        la = QWord(segs[sreg].shadow.base) &+ la
        /// limit checking
        if segs[sreg].shadow.isFlagRaised(.E) {  // expand-down segment
            notok = la < DWord(segs[sreg].shadow.base) &+ DWord(segs[sreg].shadow.limit) &+ 1
        } else {
            notok = la > DWord(segs[sreg].shadow.base) &+ DWord(segs[sreg].shadow.limit) &+ 1 &- stride
        }
        if notok {
            if sreg == 2 {
                throw Interrupt(.SS, errorCode: 0)
            } else {
                throw Interrupt(.GP, errorCode: 0)
            }
        }
        lax = DWord(truncatingIfNeeded: la)
    }
    func setSegmentRegister(_ sreg: SegmentRegister.Name, _ selector: SegmentSelector) {
    }
    func setSegmentRegisterRealOrV86Mode(_ sreg: SegmentRegister.Name, _ selector: SegmentSelector) {
    }
    func setSegmentRegisterProtectedMode(_ sreg: SegmentRegister.Name, _ selector: SegmentSelector) {
    }
}
