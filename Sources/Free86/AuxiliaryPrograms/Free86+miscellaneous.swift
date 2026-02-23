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
            lax = gdt.shadow.base + dti
            var xsd = SegmentDescriptor(try ld64ReadonlyCplX())
            if !xsd.isSystemSegment || !xsd.isType(.TSSAvailable) || !xsd.isType(.TSS16Available) {
                throw Interrupt(13, errorCode: dti)
            }
            if !xsd.isFlagRaised(.P) {
                throw Interrupt(11, errorCode: dti)
            }
            tr = SegmentRegister(selector, xsd)
            xsd.setFlag(.B)
            try st64WritableCplX(qword: xsd.qword)
        }
    }
    func auxLarLsl(_ o32: Bool, _ isLsl: Bool) throws {
        let selector: SegmentSelector
        if cr0.isProtectedMode || eflags.isFlagRaised(.VM) {
            throw Interrupt(6)
        }
        modRM = fetch8()
        if modRM.mod == 3 {
            selector = SegmentSelector(regs[modRM.rM])
        } else {
            segmentTranslation()
            selector = try ld16ReadonlyCpl3()
        }
        u = try ldDescriptorFields(selector, isLsl)
        osmSrc = compileEflags()
        if u == 0x80000000 {  // notok
            osmSrc.clearBit(EflagsFlag.ZF.rawValue)
        } else {
            osmSrc.raiseBit(EflagsFlag.ZF.rawValue)
            if (o32) {
                regs[modRM.reg] = u
            } else {
                regs[modRM.reg].lowerHalf = u
            }
        }
        osmDst = ((osmSrc >> 6) & 1) ^ 1
        osm = 24
    }
    func ldDescriptorFields(_ selector: SegmentSelector, _ limit: Bool) throws -> DWord {
        let notok: DWord = 0x80000000
        if selector.index == 0 {
            return notok
        }
        let xsd = try ldXdtEntry(selector)
        if xsd.qword == 0 {
            return notok
        }
        rpl = selector.rpl
        dpl = xsd.dpl
        if !xsd.isSystemSegment {  // code/ data segment
            if (xsd.type & 0b0_1100) == 0 {  // if non-conforming code segments
                if (dpl < cpl) || (dpl < rpl) {
                    return notok
                }
            }
        } else {  // system segment
            switch xsd.type & 0b0_1111 {
            case 1, 2, 3, 9, 11:
                // 1:  16 bit TSS (busy)
                // 2:  LDT
                // 3:  16 bit TSS (available)
                // 9:  32 bit TSS (busy)
                // 11: 32 bit TSS (available)
                break
            case 4, 5, 12:
                // 4:  16 bit call gate
                // 5:  task gate
                // 12: 32 bit call gate
                if limit {
                    return notok
                }
                break
            default:
                return notok
            }
            if (dpl < cpl) || (dpl < rpl) {
                return notok
            }
        }
        if (limit) {
            return xsd.limit
        } else {
            return xsd.flags
        }
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
