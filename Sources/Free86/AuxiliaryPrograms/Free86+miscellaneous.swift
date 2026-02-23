extension Free86 {
    func auxLdtr(_ selector: SegmentSelector) throws {
        let dti = DWord(selector.index)
        if dti == 0 {
            ldt = SegmentRegister(selector, SegmentDescriptor(0))
        } else {
            if !selector.isGDT {
                throw Interrupt(13, errorCode: dti)
            }
            if (dti + 7) > gdt.shadow.limit {
                throw Interrupt(13, errorCode: dti)
            }
            lax = gdt.shadow.base + dti
            let xsd = SegmentDescriptor(try ld64ReadonlyCplX())
            if !xsd.isSystemSegment || !xsd.isType(.LDT) {
                throw Interrupt(13, errorCode: dti)
            }
            if !xsd.isFlagRaised(.P) {
                throw Interrupt(11, errorCode: dti)
            }
            ldt = SegmentRegister(selector, xsd)
        }
    }
    func auxLtr(_ selector: SegmentSelector) throws {
        let dti = DWord(selector.index)
        if (dti == 0) {
            tr = SegmentRegister(selector, SegmentDescriptor(0))
        } else {
            if !selector.isGDT {
                throw Interrupt(13, errorCode: dti)
            }
            if (dti + 7) > gdt.shadow.limit {
                throw Interrupt(13, errorCode: dti)
            }
            lax = gdt.shadow.base + dti;
            let xsd = SegmentDescriptor(try ld64ReadonlyCplX());
            if !xsd.isSystemSegment || (xsd.type != .TSSAvailable && xs.type != .TSS16Available) {
                throw Interrupt(13, errorCode: dti)
            }
            if !xsd.isFlagRaised(.P) {
                throw Interrupt(11, errorCode: dti)
            }
            tr = SegmentRegister(selector, xsd)
            xsd.xsd.setFlag(.B)
            try st64WritableCplX(xsd.qword());
        }
    }
    func auxLarLsl(_ o32: Bool, _ isLsl: Bool) {
    }
    func ldDescriptorFields(_ selector: SegmentSelector, _ limit: Bool) -> DWord {
        0
    }
    func auxVerrVerw(_ selector: SegmentSelector, _ isVerw: Bool) {
    }
    func isSegmentAccessible(_ selector: SegmentSelector, _ writable: Bool) -> Bool {
        false
    }
    func auxArpl() {
    }
    func auxCpuid() {
    }
    func aux16Bound() {
    }
    func auxBound() {
    }
    func aux16Pusha() {
    }
    func auxPusha() {
    }
    func aux16Popa() {
    }
    func auxPopa() {
    }
    func aux16Leave() {
    }
    func auxLeave() {
    }
    func aux16Enter() {
    }
    func auxEnter() {
    }
    func ldFarPointer16(_ sreg: SegmentRegister.Name) {
    }
    func ldFarPointer(_ sreg: SegmentRegister.Name) {
    }
}
