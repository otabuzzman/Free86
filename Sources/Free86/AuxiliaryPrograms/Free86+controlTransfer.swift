extension Free86 {
    func auxJmpf(_ selector: SegmentSelector, _ offset: LinearAddress) throws {
        if cr0.isProtectedMode || eflags.isFlagRaised(.VM) {
            auxJmpfRealOrV86Mode(selector, offset)
        } else {
            try auxJmpfProtectedMode(selector, offset)
        }
    }
    func auxJmpfRealOrV86Mode(_ selector: SegmentSelector, _ offset: LinearAddress) {
        eip = offset, far = far_start = 0
        segs[.CS].selector = selector
        segs[.CS].shadow.base = selector << 4
    }
    func auxJmpfProtectedMode(_ selector: SegmentSelector, _ offset: LinearAddress) throws {
        if selector.index == 0 {
            throw Interrupt(.GP, errorCode: 0)
        }
        let xsd = try ldXdtEntry(selector)
        if xsd.qword == 0 {
            throw Interrupt(.GP, errorCode: selector.index)
        }
        if xsd.isSystemSegment {
            if (xsd.type & 0b0_1000) == 0 {  // mask-out invalid system descriptors
                throw Interrupt(.GP, errorCode: selector.index)
            }
            if (xsd.type & 0b0_0100) != 0 {  // call, Interrupt, or trap gate
                if xsd.dpl > cpl {
                    throw Interrupt(.GP, errorCode: selector.index)
                }
            } else {  // TSS
                if selector.rpl > cpl {
                    throw Interrupt(.GP, errorCode: selector.index)
                }
                if xsd.dpl != cpl {
                    throw Interrupt(.GP, errorCode: selector.index)
                }
            }
            if !xsd.isFlagRaised(.P) {
                throw Interrupt(.GP, errorCode: selector.index)
            }
            if offset > xsd.limit {
                throw Interrupt(.GP, errorCode: selector.index)
            }
            setSegmentRegister(1, selector.index | cpl, xsd.base, xsd.limit, xsd.flags)
            eip = offset, far = far_start = 0
        } else {
            throw "fatal: not a system segment descriptor in JMP"
        }
    }
    func auxCallf(_ o32: Bool, _ selector: SegmentSelector, _ offset: LinearAddress, _ home: LinearAddress) {
    }
    func auxCallfRealOrV86Mode(_ o32: Bool, _ selector: SegmentSelector, _ offset: LinearAddress, _ home: LinearAddress) {
    }
    func auxCallfProtectedMode(_ o32: Bool, _ selector: SegmentSelector, _ offset: LinearAddress, _ home: LinearAddress) {
    }
    func auxRetf(_ o32: Bool, _ releaseStackItems: DWord) {
    }
    func returnRealOrV86Mode(_ o32: Bool, _ isIret: Bool, _ releaseStackItems: DWord) {
    }
    func returnProtectedMode(_ o32: Bool, _ isIret: Bool, _ releaseStackItems: DWord) {
    }
    func resetSegmentRegister(_ sreg: SegmentRegister.Name, _ level: DWord) {
    }
    func raiseInterrupt(_ id: Int, _ errorCode: Int, _ isHW: Bool, _ isSW: Bool, _ home: LinearAddress) {
    }
    func raiseInterruptRealOrV86Mode(_ id: Int, _ isSW: Bool, _ home: LinearAddress) {
    }
    func raiseInterruptProtectedMode(_ id: Int, _ errorCode: Int, _ isHW: Bool, _ isSW: Bool, _ home: LinearAddress) {
    }
    func ldXdtEntry(_ selector: SegmentSelector) throws -> SegmentDescriptor {
        let xdt: SegmentRegister
        if selector.isLDT {
            xdt = ldt
        } else {
            xdt = gdt
        }
        let dti = DWord(selector.index)
        if (dti + 7) > xdt.shadow.limit {
            return SegmentDescriptor(0)
        }
        lax = xdt.shadow.base + dti
        return SegmentDescriptor(try ld64ReadonlyCplX())
    }
    func ldTssStack(_ level: DWord) throws -> QWord {  // seg:offset
        0
    }
    func auxIret(_ o32: Bool) {
    }
}
