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
        assert(!xsd.isSystemSegment, "fatal error: unexpected system segment descriptor")
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
        segs[.CS] = SegmentRegister(selector.index | Word(cpl), xsd)
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
            segs[.CS] = SegmentRegister(selector.index | Word(cpl), xsd)
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
                segs[.SS] = SegmentRegister(segs[.SS].selector, ssd)
                if xsd.isType(.CallGate) {  // 32 bit descriptor
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
                segs[.SS] = SegmentRegister(ss.index | Word(ssd.dpl), ssd)
            } else {  // intralevel
                esp = espStart
            }
            if xsd.isType(.CallGate) {
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
            segs[.CS] = SegmentRegister(gsel.index | Word(cgd.dpl), cgd)
            cpl = cgd.dpl
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
        let homeEip: DWord
        var homeEflags: Eflags = 0
        var esp = regs[.ESP]
        let ssMask: DWord = 0xffff
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
            homeEip = DWord(try ld16ReadonlyCplX())
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
        regs[.ESP] = (regs[.ESP] & ~ssMask) | ((esp &+ releaseStackItems) & ssMask)
        var shadow = segs[.CS].shadow
        shadow.base = LinearAddress(cs << 4)
        segs[.CS] = SegmentRegister(cs, shadow)
        eip = homeEip
        far = 0
        farStart = 0
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
        let homeEsp: DWord, homeEip: DWord
        var homeEflags: Eflags = 0
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
                    farStart = 0
                    regs[.ESP] = (regs[.ESP] & ~ssMask) | (homeEsp & ssMask)
                    self.cpl = 3
                    return
                }
            }
        } else {
            lax = ssBase &+ (esp & ssMask)
            homeEip = DWord(try ld16ReadonlyCplX())
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
            throw Interrupt(.GP, errorCode: DWord(cs.index))
        }
        if !csd.isCodeSegment {
            throw Interrupt(.GP, errorCode: DWord(cs.index))
        }
        if cs.rpl < cpl {
            throw Interrupt(.GP, errorCode: DWord(cs.index))
        }
        if csd.isFlagRaised(.C) {
            if csd.dpl > cs.rpl {
                throw Interrupt(.GP, errorCode: DWord(cs.index))
            }
        } else {
            if csd.dpl != cs.rpl {
                throw Interrupt(.GP, errorCode: DWord(cs.index))
            }
        }
        if !csd.isFlagRaised(.P) {
            throw Interrupt(.NP, errorCode: DWord(cs.index))
        }
        esp = esp &+ releaseStackItems
        if cs.rpl == cpl {
            segs[.CS] = SegmentRegister(cs, csd)
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
                homeEsp = DWord(try ld16ReadonlyCplX())
                esp = esp &+ 2
                lax = ssBase &+ (esp & ssMask)
                ss = SegmentSelector(truncatingIfNeeded: try ld16ReadonlyCplX())
                esp = esp &+ 2
            }
            if ss.index == 0 {
                throw Interrupt(.GP, errorCode: 0)
            } else {
                if ss.rpl != cs.rpl {
                    throw Interrupt(.GP, errorCode: DWord(ss.index))
                }
                let ssd = try ldXdtEntry(ss)
                if ssd.qword == 0 {
                    throw Interrupt(.GP, errorCode: DWord(ss.index))
                }
                if !ssd.isDataSegment || !ssd.isFlagRaised(.W) {
                    throw Interrupt(.GP, errorCode: DWord(ss.index))
                }
                if ssd.dpl != cs.rpl {
                    throw Interrupt(.GP, errorCode: DWord(ss.index))
                }
                if !ssd.isFlagRaised(.P) {
                    throw Interrupt(.NP, errorCode: DWord(ss.index))
                }
                segs[.SS] = SegmentRegister(ss, ssd)
            }
            resetSegmentRegister(.ES, cs.rpl)
            segs[.CS] = SegmentRegister(cs, csd)
            resetSegmentRegister(.DS, cs.rpl)
            resetSegmentRegister(.FS, cs.rpl)
            resetSegmentRegister(.GS, cs.rpl)
            esp = homeEsp &+ releaseStackItems
            self.cpl = cs.rpl
        }
        regs[.ESP] = (regs[.ESP] & ~ssMask) | (esp & ssMask)
        eip = homeEip
        far = 0
        farStart = 0
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
        if xsd.isDataSegment && !xsd.isFlagRaised(.E) {
           if xsd.dpl < level {
                segs[sreg] = SegmentRegister(0, SegmentDescriptor(0))
            }
        }
    }
    func raiseInterrupt(_ id: Int, _ errorCode: Int, _ isHW: Bool, _ isSW: Bool, _ home: LinearAddress) throws {
        if !cr0.isProtectedMode {
            try raiseInterruptRealOrV86Mode(id, isSW, home)
        } else {
            try raiseInterruptProtectedMode(id, errorCode, isHW, isSW, home)
        }
    }
    func raiseInterruptRealOrV86Mode(_ id: Int, _ isSW: Bool, _ home: LinearAddress) throws {
        if (id * 4 + 3) > idt.shadow.limit {
           throw Interrupt(.GP, errorCode: DWord(id * 4 + 2))
        }
        lax = idt.shadow.base &+ DWord((id << 2))
        let offset = try ld16ReadonlyCplX()
        lax = lax &+ 2
        let selector = try ld16ReadonlyCplX()
        var esp = regs[.ESP]
        esp = esp &- 2
        lax = ssBase &+ (esp & ssMask)
        try st16WritableCpl3(word: Word(getEflags()))
        esp = esp &- 2
        lax = ssBase &+ (esp & ssMask)
        try st16WritableCpl3(word: segs[.CS].selector)
        esp = esp &- 2
        lax = ssBase &+ (esp & ssMask)
        try st16WritableCpl3(word: Word(isSW ? home : eip))
        regs[.ESP] = (regs[.ESP] & ~ssMask) | (esp & ssMask)
        eip = DWord(offset)
        far = 0
        farStart = 0
        var shadow = segs[.CS].shadow
        shadow.base = LinearAddress(selector << 4)
        segs[.CS] = SegmentRegister(selector, shadow)
        eflags.clearBit(EflagsFlag.TF.rawValue)
        eflags.clearBit(EflagsFlag.IF.rawValue)
        eflags.clearBit(EflagsFlag.RF.rawValue)
        eflags.clearBit(EflagsFlag.AC.rawValue)
    }
    func raiseInterruptProtectedMode(_ id: Int, _ errorCode: Int, _ isHW: Bool, _ isSW: Bool, _ home: LinearAddress) throws {
        var ss = SegmentSelector(0), ssd = SegmentDescriptor(0), esp: DWord, ssBase: DWord, ssMask: DWord
        var dpl: DWord = 0, isInterlevel: Bool, pushErrorCode = false
        if !isSW && !isHW {
            switch Exception(rawValue: Byte(id)) {
            case .DF:  // double exception
                fallthrough
            case .TS:  // invalid task state segment
                fallthrough
            case .NP:  // segment not present
                fallthrough
            case .SS:  // stack fault
                fallthrough
            case .GP:  // general protection
                fallthrough
            case .PF:  // page fault
                fallthrough
            case .AC:  // alignment check (80486)
                pushErrorCode = true
                break
            default:
                break
            }
        }
        if (id * 8 + 7) > idt.shadow.limit {
            throw Interrupt(.GP, errorCode: DWord(id * 8 + 2))
        }
        lax = idt.shadow.base + DWord(id * 8)
        let isd = SegmentDescriptor(try ld64ReadonlyCplX())
        let type = SegmentDescriptorType(rawValue: isd.type)
        switch type {
        case .TaskGate:
            fallthrough
        case .InterruptGate16:
            fallthrough
        case .TrapGate16:
            assert(false, "fatal error: unsupported system segment descriptor")
            break
        case .InterruptGate:
            fallthrough
        case .TrapGate:
            break
        default:
            throw Interrupt(.GP, errorCode: DWord(id * 8 + 2))
        }
        if (isSW && isd.dpl < cpl) {
            throw Interrupt(.GP, errorCode: DWord(id * 8 + 2))
        }
        if !isd.isFlagRaised(.P) {
            throw Interrupt(.NP, errorCode: DWord(id * 8 + 2))
        }
        let gsel = SegmentSelector(truncatingIfNeeded: (isd.qword >> 16) & 0xffff)  // different fields in call gate
        let goff = DWord(truncatingIfNeeded: (isd.qword >> 32) & 0xffff0000) | DWord(truncatingIfNeeded: isd.qword & 0x0000ffff)
        if gsel.index == 0 {
            throw Interrupt(.GP, errorCode: 0)
        }
        let cgd = try ldXdtEntry(gsel)
        if cgd.qword == 0 {
            throw Interrupt(.GP, errorCode: 0)
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
        if !cgd.isFlagRaised(.C) && (cgd.dpl < cpl) {  // interlevel
            let tss = try ldTssStack(cgd.dpl)  // seg:offset
            ss = SegmentSelector(truncatingIfNeeded: tss >> 32)
            esp = DWord(truncatingIfNeeded: tss)
            if ss.index == 0 {
                throw Interrupt(.TS, errorCode: 0)
            }
            if ss.rpl != cgd.dpl {
                throw Interrupt(.TS, errorCode: DWord(ss.index))
            }
            ssd = try ldXdtEntry(ss)
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
            ssBase = ssd.base
            ssMask = ssd.segmentSizeMask
            isInterlevel = true
        } else if cgd.isFlagRaised(.C) || (cgd.dpl == cpl) {  // intralevel
            if eflags.isFlagRaised(.VM) {
                throw Interrupt(.GP, errorCode: DWord(gsel.index))
            }
            dpl = cpl
            ssBase = segs[.SS].shadow.base
            ssMask = segs[.SS].shadow.segmentSizeMask
            esp = regs[.ESP]
            isInterlevel = false
        } else {
            throw Interrupt(.GP, errorCode: DWord(gsel.index))
        }
        if isd.isType(.InterruptGate) || isd.isType(.TrapGate) {  // 32 bit descriptor (?)
            if isInterlevel {
                if eflags.isFlagRaised(.VM) {
                    esp = esp &- 4
                    lax = ssBase &+ (esp & ssMask)
                    try stWritableCpl3(dword: DWord(segs[.GS].selector))
                    esp = esp &- 4
                    lax = ssBase &+ (esp & ssMask)
                    try stWritableCpl3(dword: DWord(segs[.FS].selector))
                    esp = esp &- 4
                    lax = ssBase &+ (esp & ssMask)
                    try stWritableCpl3(dword: DWord(segs[.DS].selector))
                    esp = esp &- 4
                    lax = ssBase &+ (esp & ssMask)
                    try stWritableCpl3(dword: DWord(segs[.ES].selector))
                }
                esp = esp &- 4
                lax = ssBase &+ (esp & ssMask)
                try stWritableCpl3(dword: DWord(segs[.SS].selector))
                esp = esp &- 4
                lax = ssBase &+ (esp & ssMask)
                try stWritableCpl3(dword: regs[.ESP])
            }
            esp = esp &- 4
            lax = ssBase &+ (esp & ssMask)
            try stWritableCpl3(dword: getEflags())
            esp = esp &- 4
            lax = ssBase &+ (esp & ssMask)
            try stWritableCpl3(dword: DWord(segs[.CS].selector))
            esp = esp &- 4
            lax = ssBase &+ (esp & ssMask)
            try stWritableCpl3(dword: isSW ? home : eip)
            if pushErrorCode {
                esp = esp &- 4
                lax = ssBase &+ (esp & ssMask)
                try stWritableCpl3(dword: DWord(errorCode))
            }
        } else {
            if isInterlevel {
                if eflags.isFlagRaised(.VM) {
                    esp = esp &- 2
                    lax = ssBase &+ (esp & ssMask)
                    try st16WritableCpl3(word: segs[.GS].selector)
                    esp = esp &- 2
                    lax = ssBase &+ (esp & ssMask)
                    try st16WritableCpl3(word: segs[.FS].selector)
                    esp = esp &- 2
                    lax = ssBase &+ (esp & ssMask)
                    try st16WritableCpl3(word: segs[.DS].selector)
                    esp = esp &- 2
                    lax = ssBase &+ (esp & ssMask)
                    try st16WritableCpl3(word: segs[.ES].selector)
                }
                esp = esp &- 2
                lax = ssBase &+ (esp & ssMask)
                try st16WritableCpl3(word: segs[.SS].selector)
                esp = esp &- 2
                lax = ssBase &+ (esp & ssMask)
                try st16WritableCpl3(word: Word(regs[.ESP]))
            }
            esp = esp &- 2
            lax = ssBase &+ (esp & ssMask)
            try st16WritableCpl3(word: Word(getEflags()))
            esp = esp &- 2
            lax = ssBase &+ (esp & ssMask)
            try st16WritableCpl3(word: (segs[.CS].selector))
            esp = esp &- 2
            lax = ssBase &+ (esp & ssMask)
            try st16WritableCpl3(word: Word(isSW ? home : eip))
            if pushErrorCode {
                esp = esp &- 2
                lax = ssBase &+ (esp & ssMask)
                try st16WritableCpl3(word: Word((errorCode)))
            }
        }
        if isInterlevel {
            if eflags.isFlagRaised(.VM) {
                segs[.ES] = SegmentRegister(0, SegmentDescriptor(0))
                segs[.DS] = SegmentRegister(0, SegmentDescriptor(0))
                segs[.FS] = SegmentRegister(0, SegmentDescriptor(0))
                segs[.GS] = SegmentRegister(0, SegmentDescriptor(0))
            }
            segs[.SS] = SegmentRegister(ss.index | Word(dpl), ssd)
        }
        segs[.CS] = SegmentRegister(gsel.index | Word(dpl), cgd)
        cpl = dpl
        regs[.ESP] = (regs[.ESP] & ~ssMask) | (esp & ssMask)
        eip = goff
        far = 0
        farStart = 0
        if isd.isType(.TrapGate) {  // .TrapGate (?)
            eflags.clearBit(EflagsFlag.IF.rawValue)
        }
        eflags.clearBit(EflagsFlag.TF.rawValue)
        eflags.clearBit(EflagsFlag.NT.rawValue)
        eflags.clearBit(EflagsFlag.RF.rawValue)
        eflags.clearBit(EflagsFlag.VM.rawValue)
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
        var res: QWord
        if !tr.shadow.isFlagRaised(.P) {
            throw Interrupt(.NP, errorCode: DWord(tr.selector.index))
        }
        if !tr.shadow.isType(.TSSAvailable) && !tr.shadow.isType(.TSS16Available) {
            throw Interrupt(.GP, errorCode: DWord(tr.selector.index))
        }
        let tss32 = tr.shadow.isType(.TSSAvailable) ? 1 : 0
        let offset = (level * 4 + 2) << tss32  // offset of privileged (E)SP in TSS
        if (offset + (4 << tss32) - 1) > tr.shadow.limit {
            throw Interrupt(.TS, errorCode: DWord(tr.selector.index))
        }
        lax = tr.shadow.base &+ offset
        if tss32 == 1 {
            res = QWord(try ldReadonlyCplX())  // privileged ESP
            lax = lax &+ 4
        } else {
            res = QWord(try ld16ReadonlyCplX())  // privileged SP
            lax = lax &+ 2
        }
        res |= QWord(try ld16ReadonlyCplX()) << 32  // privileged SS
        return res
    }
    func auxIret(_ o32: Bool) throws {
        if !cr0.isProtectedMode || eflags.isFlagRaised(.VM) {
            if eflags.isFlagRaised(.VM) {
                if eflags.iopl != 3 {
                    throw Interrupt(.GP, errorCode: 0)
                }
            }
            try returnRealOrV86Mode(o32, true, 0)
        } else {
            if eflags.isFlagRaised(.NT) {
                assert(false, "fatal error: EFLAGS.NT set")
            } else {
                try returnProtectedMode(o32, true, 0)
            }
        }
    }
}
