extension Free86 {
    func auxJmpf(_ selector: SegmentSelector, _ offset: LinearAddress) throws {
        if !cr0.isProtectedMode || eflags.isFlagRaised(.VM) {
            auxJmpfRealOrV86Mode(selector, offset)
        } else {
            try auxJmpfProtectedMode(selector, offset)
        }
    }
    func auxJmpfRealOrV86Mode(_ selector: SegmentSelector, _ offset: LinearAddress) {
        eip = offset
        far = 0
        farStart = 0
        var shadow = segs[.CS].shadow
        shadow.base = LinearAddress(selector << 4)
        segs[.CS] = SegmentRegister(selector, shadow)
    }
    func auxJmpfProtectedMode(_ selector: SegmentSelector, _ offset: LinearAddress) throws {
        if selector.index == 0 {
            throw Interrupt(.GP, errorCode: 0)
        }
        let xsd = try ldXdtEntry(selector)
        if xsd.qword == 0 {
            throw Interrupt(.GP, errorCode: DWord(selector.index))
        }
        assert(!xsd.isSystemSegment, "fatal error: system segment descriptor")
        if !xsd.isCodeSegment {
            throw Interrupt(.GP, errorCode: DWord(selector.index))
        }
        if xsd.isFlagRaised(.C) {
            if xsd.dpl > cpl {
                throw Interrupt(.GP, errorCode: DWord(selector.index))
            }
        } else {
            if selector.rpl > cpl {
                throw Interrupt(.GP, errorCode: DWord(selector.index))
            }
            if xsd.dpl != cpl {
                throw Interrupt(.GP, errorCode: DWord(selector.index))
            }
        }
        if !xsd.isFlagRaised(.P) {
            throw Interrupt(.NP, errorCode: DWord(selector.index))
        }
        if offset > xsd.limit {
            throw Interrupt(.GP, errorCode: DWord(selector.index))
        }
        setSegmentRegister(.CS, selector.index | Word(cpl), shadow: xsd.base, xsd.limit, xsd.flags)
        eip = offset
        far = 0
        farStart = 0
    }
    func auxCallf(_ o32: Bool, _ selector: SegmentSelector, _ offset: LinearAddress, _ home: LinearAddress) throws {
        if !cr0.isProtectedMode || eflags.isFlagRaised(.VM) {
            try auxCallfRealOrV86Mode(o32, selector, offset, home)
        } else {
            try auxCallfProtectedMode(o32, selector, offset, home)
        }
    }
    func auxCallfRealOrV86Mode(_ o32: Bool, _ selector: SegmentSelector, _ offset: LinearAddress, _ home: LinearAddress) throws {
        var esp = regs[.ESP]
        if o32 {
            esp = esp &- 4
            lax = ssBase &+ (esp & ssMask)
            try stWritableCpl3(dword: DWord(segs[.CS].selector))
            esp = esp &- 4
            lax = ssBase &+ (esp & ssMask)
            try stWritableCpl3(dword: home)
        } else {
            esp = esp &- 2
            lax = ssBase &+ (esp & ssMask)
            try st16WritableCpl3(word: segs[.CS].selector)
            esp = esp &- 2
            lax = ssBase &+ (esp & ssMask)
            try st16WritableCpl3(word: Word(home))
        }
        regs[.ESP] = (regs[.ESP] & ~ssMask) | (esp & ssMask)
        eip = offset
        far = 0
        farStart = 0
        var shadow = segs[.CS].shadow
        shadow.base = LinearAddress(selector << 4)
        segs[.CS] = SegmentRegister(selector, shadow)
    }
    func auxCallfProtectedMode(_ o32: Bool, _ selector: SegmentSelector, _ offset: LinearAddress, _ home: LinearAddress) throws {
        var esp: DWord
        if selector.index == 0 {
            throw Interrupt(.GP, errorCode: 0)
        }
        let xsd = try ldXdtEntry(selector)
        if xsd.qword == 0 {
            throw Interrupt(.GP, errorCode: DWord(selector.index))
        }
        let espStart = regs[.ESP]
        if !xsd.isSystemSegment {
            if !xsd.isCodeSegment {
                throw Interrupt(.GP, errorCode: DWord(selector.index))
            }
            if xsd.isFlagRaised(.C) {
                if xsd.dpl > cpl {
                    throw Interrupt(.GP, errorCode: DWord(selector.index))
                }
            } else {
                if selector.rpl > cpl {
                    throw Interrupt(.GP, errorCode: DWord(selector.index))
                }
                if xsd.dpl != cpl {
                    throw Interrupt(.GP, errorCode: DWord(selector.index))
                }
            }
            if !xsd.isFlagRaised(.P) {
                throw Interrupt(.NP, errorCode: DWord(selector.index))
            }
            esp = espStart
            if o32 {
                esp = esp &- 4
                lax = ssBase &+ (esp & ssMask)
                try stWritableCpl3(dword: DWord(segs[.CS].selector))
                esp = esp &- 4
                lax = ssBase &+ (esp & ssMask)
                try stWritableCpl3(dword: home)
            } else {
                esp = esp &- 2
                lax = ssBase &+ (esp & ssMask)
                try st16WritableCpl3(word: segs[.CS].selector)
                esp = esp &- 2
                lax = ssBase &+ (esp & ssMask)
                try st16WritableCpl3(word: Word(home))
            }
            if offset > xsd.limit {
                throw Interrupt(.GP, errorCode: DWord(selector.index))
            }
            regs[.ESP] = (regs[.ESP] & ~ssMask) | (esp & ssMask)
            setSegmentRegister(.CS, selector.index | Word(cpl), shadow: xsd.base, xsd.limit, xsd.flags)
            eip = offset
            far = 0
            farStart = 0
        } else {
            let type = SegmentDescriptorType(rawValue: xsd.type)
            switch type {
            case .TSS16Available:
                fallthrough
            case .TaskGate:
                fallthrough
            case .TSSAvailable:
                assert(false, "fatal error: unsupported system segment descriptor")
                break
            case .CallGate16:
                fallthrough
            case .CallGate:
                break
            default:
                throw Interrupt(.GP, errorCode: DWord(selector.index))
            }
            if (xsd.dpl < cpl) || (xsd.dpl < selector.rpl) {
                throw Interrupt(.GP, errorCode: DWord(selector.index))
            }
            if !xsd.isFlagRaised(.P) {
                throw Interrupt(.NP, errorCode: DWord(selector.index))
            }
            let gsel = SegmentSelector(truncatingIfNeeded: (xsd.qword >> 16) & 0xffff)  // different fields in call gate
            let goff = DWord(truncatingIfNeeded: (xsd.qword >> 32) & 0xffff0000) | DWord(truncatingIfNeeded: xsd.qword & 0x0000ffff)
            let gpac = xsd.flags & 0x1f
            if gsel.index == 0 {
                throw Interrupt(.GP, errorCode: 0)
            }
            let cgd = try ldXdtEntry(gsel)
            if cgd.qword == 0 {
                throw Interrupt(.GP, errorCode: DWord(gsel.index))
            }
            if !cgd.isCodeSegment {
                throw Interrupt(.GP, errorCode: DWord(gsel.index))
            }
            if cgd.dpl > cpl {
                throw Interrupt(.GP, errorCode: DWord(gsel.index))
            }
            if !cgd.isFlagRaised(.P) {
                throw Interrupt(.NP, errorCode: DWord(gsel.index))
            }
            if !cgd.isFlagRaised(.C) && (cgd.dpl < cpl) {  // intralevel
                let tss = try ldTssStack(cgd.dpl)  // seg:offset
                let ss = SegmentSelector(truncatingIfNeeded: tss >> 32)
                esp = DWord(truncatingIfNeeded: tss)
                if ss.index == 0 {
                    throw Interrupt(.TS, errorCode: 0)
                }
                if ss.rpl != cgd.dpl {
                    throw Interrupt(.TS, errorCode: DWord(ss.index))
                }
                let ssd = try ldXdtEntry(ss)
                if ssd.qword == 0 {
                    throw Interrupt(.TS, errorCode: DWord(ss.index))
                }
                if ssd.dpl != cgd.dpl {
                    throw Interrupt(.TS, errorCode: DWord(ss.index))
                }
                if !ssd.isDataSegment || !ssd.isFlagRaised(.W) {
                    throw Interrupt(.TS, errorCode: DWord(ss.index))
                }
                if !ssd.isFlagRaised(.P) {
                    throw Interrupt(.NP, errorCode: DWord(ss.index))
                }
                // osBase = ssBase
                // osMask = ssMask
                segs[.SS] = SegmentRegister(segs[.SS].selector, SegmentDescriptor(ssd.qword))
                if type == .CallGate {
                    esp = esp &- 4
                    lax = ssBase &+ (esp & ssMask)
                    try stWritableCpl3(dword: DWord(segs[.SS].selector))
                    esp = esp &- 4
                    lax = ssBase &+ (esp & ssMask)
                    try stWritableCpl3(dword: espStart)
                    for _ in (1...gpac).reversed() {
                        u = 0 // Xe(osBase &+ ((espStart &+ (i &- 1) &* 4) & osMask))
                        esp = esp &- 4
                        lax = ssBase &+ (esp & ssMask)
                        try stWritableCpl3(dword: u)
                    }
                } else {
                    esp = esp &- 2
                    lax = ssBase &+ (esp & ssMask)
                    try st16WritableCpl3(word: segs[.SS].selector)
                    esp = esp &- 2
                    lax = ssBase &+ (esp & ssMask)
                    try st16WritableCpl3(word: Word(espStart))
                    for _ in (1...gpac).reversed() {
                        u = 0 // Ye(osBase &+ ((espStart &+ (i &- 1) &* 2) & osMask))
                        esp = esp &- 2
                        lax = ssBase &+ (esp & ssMask)
                        try st16WritableCpl3(word: Word(u))
                    }
                }
                setSegmentRegister(.SS, ss.index | Word(dpl), shadow: ssBase, ssd.limit, ssd.flags)
            } else {  // intralevel
                esp = espStart
            }
            if type == .CallGate {
                esp = esp &- 4
                lax = ssBase &+ (esp & ssMask)
                try stWritableCpl3(dword: DWord(segs[1].selector))
                esp = esp &- 4
                lax = ssBase &+ (esp & ssMask)
                try stWritableCpl3(dword: home)
            } else {
                esp = esp &- 2
                lax = ssBase &+ (esp & ssMask)
                try st16WritableCpl3(word: segs[1].selector)
                esp = esp &- 2
                lax = ssBase &+ (esp & ssMask)
                try st16WritableCpl3(word: Word(home))
            }
            setSegmentRegister(.CS, gsel.index | Word(dpl), shadow: cgd.base, cgd.limit, cgd.flags)
            cpl = dpl
            regs[.ESP] = (regs[.ESP] & ~ssMask) | (esp & ssMask)
            eip = goff
            far = 0
            farStart = 0
        }
    }
    func auxRetf(_ o32: Bool, _ releaseStackItems: DWord) throws {
        if !cr0.isProtectedMode || eflags.isFlagRaised(.VM) {
            try returnRealOrV86Mode(o32, false, releaseStackItems)
        } else {
            try returnProtectedMode(o32, false, releaseStackItems)
        }
    }
    func returnRealOrV86Mode(_ o32: Bool, _ isIret: Bool, _ releaseStackItems: DWord) throws {
        let cs: SegmentSelector
        let homeEip: DWord, homeEflags: Eflags
        var esp = regs[.ESP]
        let ssMask = 0xffff
        if o32 {
            lax = ssBase &+ (esp & ssMask)
            homeEip = try ldReadonlyCplX()
            esp = esp &+ 4
            lax = ssBase &+ (esp & ssMask)
            cs = SegmentSelector(truncatingIfNeeded: try ldReadonlyCplX())
            esp = esp &+ 4
            if isIret {
                lax = ssBase &+ (esp & ssMask)
                homeEflags = try ldReadonlyCplX()
                esp = esp &+ 4
            }
        } else {
            lax = ssBase &+ (esp & ssMask)
            homeEip = try ld16ReadonlyCplX()
            esp = esp &+ 2
            lax = ssBase &+ (esp & ssMask)
            cs = SegmentSelector(try ld16ReadonlyCplX())
            esp = esp &+ 2
            if isIret {
                lax = ssBase &+ (esp & ssMask)
                homeEflags = try ld16ReadonlyCplX()
                esp = esp &+ 2
            }
        }
        regs[.ESP] = (regs[.ESP] & ~ssMask) | ((esp &+ releaseStackItems) & ssMask)
        var shadow = segs[.CS].shadow
        shadow.base = LinearAddress(cs << 4)
        segs[.CS] = SegmentRegister(cs, shadow)
        eip = homeEip
        far = 0
        far_start = 0
        if isIret {
            var mask: DWord = 0
            mask.setFlag(.TF)
            mask.setFlag(.IF)
            mask.setFlag(.NT)
            mask.setFlag(.RF)
            mask.setFlag(.AC)
            mask.setFlag(.ID)
            mask.iopl = 3
            if eflags.isFlagRaised(.VM) {
                mask.iopl = 0
            }
            if !o32 {
                mask &= 0xffff
            }
            setEflags(homeEflags, mask)
        }
    }
    func returnProtectedMode(_ o32: Bool, _ isIret: Bool, _ releaseStackItems: DWord) throws {
        let homeEsp: DWord, homeEip: DWord, homeEflags: Eflags
        let es: SegmentSelector, cs: SegmentSelector, ss: SegmentSelector
        let ds: SegmentSelector, fs: SegmentSelector, gs: SegmentSelector
        let cpl = self.cpl
        var esp = regs[.ESP]
        if o32 {
            lax = ssBase &+ (esp & ssMask)
            homeEip = try ldReadonlyCplX()
            esp = esp &+ 4
            lax = ssBase &+ (esp & ssMask)
            cs = SegmentSelector(truncatingIfNeeded: try ldReadonlyCplX())
            esp = esp &+ 4
            if isIret {
                lax = ssBase &+ (esp & ssMask)
                homeEflags = try ldReadonlyCplX()
                esp = esp &+ 4
                if homeEflags.isFlagRaised(.VM) {
                    lax = ssBase &+ (esp & ssMask)
                    homeEsp = try ldReadonlyCplX()
                    esp = esp &+ 4
                    // pop segment selectors from stack
                    lax = ssBase &+ (esp & ssMask)
                    ss = SegmentSelector(truncatingIfNeeded: try ldReadonlyCplX())
                    esp = esp &+ 4
                    lax = ssBase &+ (esp & ssMask)
                    es = SegmentSelector(truncatingIfNeeded: try ldReadonlyCplX())
                    esp = esp &+ 4
                    lax = ssBase &+ (esp & ssMask)
                    ds = SegmentSelector(truncatingIfNeeded: try ldReadonlyCplX())
                    esp = esp &+ 4
                    lax = ssBase &+ (esp & ssMask)
                    fs = SegmentSelector(truncatingIfNeeded: try ldReadonlyCplX())
                    esp = esp &+ 4
                    lax = ssBase &+ (esp & ssMask)
                    gs = SegmentSelector(truncatingIfNeeded: try ldReadonlyCplX())
                    esp = esp &+ 4
                    var mask: DWord = 0
                    mask.setFlag(.TF)
                    mask.setFlag(.IF)
                    mask.setFlag(.NT)
                    mask.setFlag(.VM)
                    mask.setFlag(.AC)
                    mask.setFlag(.VIF)
                    mask.setFlag(.VIP)
                    mask.setFlag(.ID)
                    mask.iopl = 3
                    setEflags(homeEflags, mask)
                    setSegmentRegisterRealOrV86Mode(.ES, es)
                    setSegmentRegisterRealOrV86Mode(.CS, cs)
                    setSegmentRegisterRealOrV86Mode(.SS, ss)
                    setSegmentRegisterRealOrV86Mode(.DS, ds)
                    setSegmentRegisterRealOrV86Mode(.FS, fs)
                    setSegmentRegisterRealOrV86Mode(.GS, gs)
                    eip = homeEip & 0xffff
                    far = 0
                    far_start = 0
                    regs[.ESP] = (regs[.ESP] & ~ssMask) | (homeEsp & ssMask)
                    cpl = 3
                    return
                }
            }
        } else {
            lax = ssBase &+ (esp & ssMask)
            homeEip = try ld16ReadonlyCplX()
            esp = esp &+ 2
            lax = ssBase &+ (esp & ssMask)
            cs = SegmentSelector(try ld16ReadonlyCplX())
            esp = esp &+ 2
            if isIret {
                lax = ssBase &+ (esp & ssMask)
                homeEflags = Eflags(try ld16ReadonlyCplX())
                esp = esp &+ 2
            }
        }
        if cs.index == 0 {
            throw Interrupt(.GP, errorCode: 0)
        }
        let csd = try ldXdtEntry(cs)
        if csd.qword == 0 {
            throw Interrupt(.GP, errorCode: cs.index)
        }
        if !csd.isCodeSegment {
            throw Interrupt(.GP, errorCode: cs.index)
        }
        if cs.rpl < cpl {
            throw Interrupt(.GP, errorCode: cs.index)
        }
        if csd.isFlagRaised(.C) {
            if csd.dpl > cs.rpl {
                throw Interrupt(.GP, errorCode: cs.index)
            }
        } else {
            if csd.dpl != cs.rpl {
                throw Interrupt(.GP, errorCode: cs.index)
            }
        }
        if !ssd.isFlagRaised(.P) {
            throw Interrupt(.NP, errorCode: DWord(cs.index))
        }
        esp = esp &+ releaseStackItems
        if cs.rpl == cpl {
            setSegmentRegister(.CS, cs, shadow: csd.base, csd.limit, csd.flags)
        } else {
            if o32 {
                lax = ssBase &+ (esp & ssMask)
                homeEsp = try ldReadonlyCplX()
                esp = esp &+ 4
                lax = ssBase &+ (esp & ssMask)
                ss = SegmentSelector(truncatingIfNeeded: try ldReadonlyCplX())
                esp = esp &+ 4
            } else {
                lax = ssBase &+ (esp & ssMask)
                homeEsp = try ld16ReadonlyCplX()
                esp = esp &+ 2
                lax = ssBase &+ (esp & ssMask)
                ss = SegmentSelector(truncatingIfNeeded: try ld16ReadonlyCplX())
                esp = esp &+ 2
            }
            if ss.index == 0 {
                throw Interrupt(.GP, errorCode: 0)
            } else {
                if ss.rpl != cs.rpl {
                    throw Interrupt(.GP, errorCode: ss.index)
                }
                let ssd = try ldXdtEntry(ss)
                if ssd.qword == 0 {
                    throw Interrupt(.GP, errorCode: ss.index)
                }
                if !ssd.isDataSegment || !ssd.isFlagRaised(.W) {
                    throw Interrupt(.GP, errorCode: ss.index)
                }
                if ssd.dpl != rpl {
                    throw Interrupt(.GP, errorCode: ss.index)
                }
                if !ssd.isFlagRaised(.P) {
                    throw Interrupt(.NP, errorCode: DWord(ss.index))
                }
                setSegmentRegister(.SS, ss, shadow: ssd.base, ssd.limit, ssd.flags)
            }
            resetSegmentRegister(.ES, rpl)
            setSegmentRegister(.CS, cs, shadow: csd.base, csd.limit, csd.flags)
            resetSegmentRegister(.DS, rpl)
            resetSegmentRegister(.FS, rpl)
            resetSegmentRegister(.GS, rpl)
            esp = homeEsp &+ releaseStackItems
            ssMask = csd.segmentSizeMask
            cpl = rpl
        }
        regs[.ESP] = (regs[.ESP] & ~ssMask) | (esp & ssMask)
        eip = homeEip
        far = 0
        far_start = 0
        if isIret {
        var mask: DWord = 0
        mask.setFlag(.TF)
        mask.setFlag(.NT)
        mask.setFlag(.RF)
        mask.setFlag(.AC)
        mask.setFlag(.ID)
        mask.iopl = 3
            if cpl == 0 {
                mask.iopl = 3
            }
            if cpl <= eflags.iopl {
                mask.setFlag(.IF)
            }
            if !o32 {
                mask &= 0xffff
            }
            setEflags(homeEflags, mask)
        }
    }
    func resetSegmentRegister(_ sreg: SegmentRegister.Name, _ level: DWord) {
        if (sreg == .FS || sreg == .GS) && segs[sreg].selector.index == 0 {
            return // null selector in FS, GS
        }
        let xsd = segs[sreg].shadow
        if xsd.isDataSegment && !xsd.isFlagRaised(.E)
           if xsd.dpl < level {
                setSegmentRegister(sreg, 0, shadow: 0, 0, 0)
            }
        }
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
