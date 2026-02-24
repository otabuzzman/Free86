extension Free86 {
    func auxLdtr(_ selector: SegmentSelector) throws {
        let dti = DWord(selector.index)
        if dti == 0 {
            ldt = SegmentRegister(selector, SegmentDescriptor(0))
        } else {
            if !selector.isGDT {
                throw Interrupt(.GP, errorCode: dti)
            }
            if (dti + 7) > gdt.shadow.limit {
                throw Interrupt(.GP, errorCode: dti)
            }
            lax = gdt.shadow.base + dti
            let xsd = SegmentDescriptor(try ld64ReadonlyCplX())
            if !xsd.isSystemSegment || !xsd.isType(.LDT) {
                throw Interrupt(.GP, errorCode: dti)
            }
            if !xsd.isFlagRaised(.P) {
                throw Interrupt(.NP, errorCode: dti)
            }
            ldt = SegmentRegister(selector, xsd)
        }
    }
    func auxLtr(_ selector: SegmentSelector) throws {
        let dti = DWord(selector.index)
        if dti == 0 {
            tr = SegmentRegister(selector, SegmentDescriptor(0))
        } else {
            if !selector.isGDT {
                throw Interrupt(.GP, errorCode: dti)
            }
            if (dti + 7) > gdt.shadow.limit {
                throw Interrupt(.GP, errorCode: dti)
            }
            lax = gdt.shadow.base + dti
            var xsd = SegmentDescriptor(try ld64ReadonlyCplX())
            if !xsd.isSystemSegment || !xsd.isType(.TSSAvailable) || !xsd.isType(.TSS16Available) {
                throw Interrupt(.GP, errorCode: dti)
            }
            if !xsd.isFlagRaised(.P) {
                throw Interrupt(.NP, errorCode: dti)
            }
            tr = SegmentRegister(selector, xsd)
            xsd.upper.setBit(9)  /// bit 9 distinguishes available (0)/ busy (1) TSS
            try st64WritableCplX(qword: xsd.qword)
        }
    }
    func auxLarLsl(_ o32: Bool, _ isLsl: Bool) throws {
        let selector: SegmentSelector
        if cr0.isProtectedMode || eflags.isFlagRaised(.VM) {
            throw Interrupt(.UD)
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
        if xsd.isSystemSegment {
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
            if (xsd.dpl < cpl) || (xsd.dpl < selector.rpl) {
                return notok
            }
        } else {  // code/ data segment
            if (xsd.type & 0b0_1100) == 0 {  // if non-conforming code segments
                if (xsd.dpl < cpl) || (xsd.dpl < selector.rpl) {
                    return notok
                }
            }
        }
        if limit {
            return xsd.limit
        } else {
            return xsd.flags
        }
    }
    func auxVerrVerw(_ selector: SegmentSelector, _ writable: Bool) throws {
        osmSrc = compileEflags()
        if try isSegmentAccessible(selector, writable) {
            osmSrc.raiseBit(EflagsFlag.ZF.rawValue)
        } else {
            osmSrc.clearBit(EflagsFlag.ZF.rawValue)
        }
        osmDst = ((osmSrc >> 6) & 1) ^ 1
        osm = 24
    }
    func isSegmentAccessible(_ selector: SegmentSelector, _ writable: Bool) throws -> Bool {
        if selector.index == 0 {
            return false
        }
        let xsd = try ldXdtEntry(selector)
        if xsd.qword == 0 {
            return false
        }
        if xsd.isSystemSegment {
            return false
        }
        if (xsd.type & 0b0_1000) != 0 {  // code == 1, data == 0
            if writable {
                return false
            } else {
                if (xsd.type & 0b0_0100) == 0 {
                    if (xsd.dpl < cpl) || (xsd.dpl < selector.rpl) {
                        return false
                    }
                }
                if (xsd.type & 0b0_0010) == 0 {  // readable code segment
                    return true
                }
            }
        } else {
            if writable && (xsd.type & 0b0_0010) == 0 {
                return false
            }
            if (xsd.dpl < cpl) || (xsd.dpl < selector.rpl) {
                return false
            }
        }
        return true
    }
    func auxArpl() throws {
        if cr0.isProtectedMode || eflags.isFlagRaised(.VM) {
            throw Interrupt(.UD)
        }
        modRM = fetch8()
        if modRM.mod == 3 {
            rm = regs[modRM.rM].lowerHalf
        } else {
            segmentTranslation()
            rm = DWord(try ld16WritableCpl3())
        }
        r = regs[modRM.reg]
        osmSrc = compileEflags()
        if (rm & 3) < (r & 3) {
            u = (rm & ~3) | (r & 3)
            if modRM.mod == 3 {
                regs[modRM.rM].lowerHalf = u
            } else {
                try st16WritableCpl3(word: Word(u))
            }
            osmSrc.raiseBit(EflagsFlag.ZF.rawValue)
        } else {
            osmSrc.clearBit(EflagsFlag.ZF.rawValue)
        }
        osmDst = ((osmSrc >> 6) & 1) ^ 1
        osm = 24
    }
    func auxCpuid() {
        switch regs[.EAX] {
        case 0:  // vendor ID
            regs[.EAX] = 1
            regs[.EBX] = 0x756e6547  // "uneG"
            regs[.EDX] = 0x49656e69  // "Ieni"
            regs[.ECX] = 0x6c65746e  // "letn"
            break
        case 1:   // processor info and feature flags
            fallthrough
        default:  // https://datasheets.chipdb.org/Intel/x86/CPUID/24161821.pdf
            regs[.EAX] = (5 << 8) | (4 << 4) | 3  // type | family | model | stepping
            regs[.EBX] = 8 << 8                   //   00     0101    0100       0011
            regs[.ECX] = 0
            regs[.EDX] = 1 << 4
            break
        }
    }
    func aux16Bound() throws {
        modRM = fetch8()
        if modRM.mod == 3 {
            throw Interrupt(.UD)
        }
        segmentTranslation()
        u = DWord(try ld16ReadonlyCpl3()).signExtendedWord
        lax = lax &+ 2
        v = DWord(try ld16ReadonlyCpl3()).signExtendedWord
        r = regs[modRM.reg].signExtendedWord
        if (r < u) || (r > v) {
            throw Interrupt(.BR)
        }
    }
    func auxBound() throws {
        modRM = fetch8()
        if modRM.mod == 3 {
            throw Interrupt(.UD)
        }
        segmentTranslation()
        u = try ldReadonlyCpl3()
        lax = lax &+ 4
        v = try ldReadonlyCpl3()
        r = regs[modRM.reg]
        if (r < u) || (r > v) {
            throw Interrupt(.BR)
        }
    }
    func aux16Pusha() throws {
        lax = ssBase &+ ((regs[.ESP] &- 16) & ssBase)
        for reg in (0...7).reversed() {
            r = regs[reg]
            try st16WritableCpl3(word: Word(r))
            lax = lax &+ 2
        }
        regs[.ESP] = (regs[.ESP] & ~ssBase) | ((regs[.ESP] &- 16) & ssBase)
    }
    func auxPusha() throws {
        lax = ssBase &+ ((regs[.ESP] &- 32) & ssBase)
        for reg in (0...7).reversed() {
            r = regs[reg]
            try stWritableCpl3(dword: r)
            lax = lax &+ 4
        }
        regs[.ESP] = (regs[.ESP] & ~ssBase) | ((regs[.ESP] &- 32) & ssBase)
    }
    func aux16Popa() throws {
        lax = ssBase &+ (regs[.ESP] & ssBase)
        for reg in (0...7).reversed() {
            if reg != GeneralRegister.Name.ESP.rawValue {
                regs[reg].lowerHalf = DWord(try ld16ReadonlyCpl3())
            }
            lax = lax &+ 2
        }
        regs[.ESP] = (regs[.ESP] & ~ssBase) | ((regs[.ESP] &+ 16) & ssBase)
    }
    func auxPopa() throws {
        lax = ssBase &+ (regs[.ESP] & ssBase)
        for reg in (0...7).reversed() {
            if reg != GeneralRegister.Name.ESP.rawValue {
                regs[reg].lowerHalf = DWord(try ldReadonlyCpl3())
            }
            lax = lax &+ 4
        }
        regs[.ESP] = (regs[.ESP] & ~ssBase) | ((regs[.ESP] &+ 32) & ssBase)
    }
    func aux16Leave() throws {
        let ebp = regs[.EBP]
        lax = ssBase &+ (ebp & ssBase)
        regs[.EBP].lowerHalf = DWord(try ld16ReadonlyCpl3())
        regs[.ESP] = (regs[.ESP] & ~ssBase) | ((ebp & 2) & ssBase)
    }
    func auxLeave() throws {
        let ebp = regs[.EBP]
        lax = ssBase &+ (ebp & ssBase)
        regs[.EBP] = try ldReadonlyCpl3()
        regs[.ESP] = (regs[.ESP] & ~ssBase) | ((ebp & 2) & ssBase)
    }
    func aux16Enter() throws {
        imm16 = DWord(fetch16())
        imm = DWord(fetch8() & 0x1f)
        var esp = regs[.ESP]
        var ebp = regs[.EBP]
        esp = esp &- 2
        lax = ssBase &+ (esp & ssMask)
        try st16WritableCpl3(word: Word(ebp))
        let exp = esp
        if imm != 0 {
            while imm > 1 {
                ebp = ebp &- 2
                lax = ssBase &+ (ebp & ssMask)
                m16 = DWord(try ld16ReadonlyCpl3())
                esp = esp &- 2
                lax = ssBase &+ (esp & ssMask)
                try st16WritableCpl3(word: Word(m16))
                imm -= 1
            }
            esp = esp &- 2
            lax = ssBase &+ (esp & ssMask)
            try st16WritableCpl3(word: Word(exp))
        }
        esp = esp &- imm16
        lax = ssBase &+ (esp & ssMask)
        _ = try ld16WritableCpl3()
        regs[.EBP] = (regs[.EBP] & ~ssMask) | (exp & ssMask)
        regs[.ESP] = (regs[.ESP] & ~ssMask) | (esp & ssMask)
    }
    func auxEnter() throws {
        imm16 = DWord(fetch16())
        imm = DWord(fetch8() & 0x1f)
        var esp = regs[.ESP]
        var ebp = regs[.EBP]
        esp = esp &- 4
        lax = ssBase &+ (esp & ssMask)
        try stWritableCpl3(dword: ebp)
        let exp = esp
        if imm != 0 {
            while imm > 1 {
                ebp = ebp &- 4
                lax = ssBase &+ (ebp & ssMask)
                m = try ldReadonlyCpl3()
                esp = esp &- 4
                lax = ssBase &+ (esp & ssMask)
                try stWritableCpl3(dword: m)
                imm -= 1
            }
            esp = esp &- 4
            lax = ssBase &+ (esp & ssMask)
            try stWritableCpl3(dword: exp)
        }
        esp = esp &- imm16
        lax = ssBase &+ (esp & ssMask)
        _ = try ldWritableCpl3()
        regs[.EBP] = (regs[.EBP] & ~ssMask) | (exp & ssMask)
        regs[.ESP] = (regs[.ESP] & ~ssMask) | (esp & ssMask)
    }
    func ldFarPointer16(_ sreg: SegmentRegister.Name) throws {
        modRM = fetch8()
        segmentTranslation()
        imm = DWord(try ld16ReadonlyCpl3())
        lax = lax &+ 2
        imm16 = DWord(try ld16ReadonlyCpl3())
        setSegmentRegister(sreg, SegmentSelector(imm16))
        regs[modRM.reg].lowerHalf = imm
    }
    func ldFarPointer(_ sreg: SegmentRegister.Name) throws {
        modRM = fetch8()
        segmentTranslation()
        imm = try ldReadonlyCpl3()
        lax = lax &+ 4
        imm16 = DWord(try ld16ReadonlyCpl3())
        setSegmentRegister(sreg, SegmentSelector(imm16))
        regs[modRM.reg] = imm
    }
    func fetch8() -> Byte {
        let byte = memory.ld8(from: far)
        far = far &+ 1
        return byte
    }
    func fetch16() -> Word {
        let word = memory.ld16(from: far)
        far = far &+ 2
        return word
    }
    func fetch() -> DWord {
        let dword = memory.ld(from: far)
        far = far &+ 4
        return dword
    }
    func push16(_ word: Word) throws {
        let esp = regs[.ESP] &- 2
        lax = ssBase &+ (esp & ssMask)
        try st16WritableCpl3(word: Word(word))
        regs[.ESP] = (regs[.ESP] & ~ssMask) | (esp & ssMask)
    }
    func push(_ dword: DWord) throws {
        let esp = regs[.ESP] &- 4
        lax = ssBase &+ (esp & ssMask)
        try stWritableCpl3(dword: dword)
        regs[.ESP] = (regs[.ESP] & ~ssMask) | (esp & ssMask)
    }
    func pop16() throws -> Word {
        let res = try ld16Stack()
        regs[.ESP] = (regs[.ESP] & ~ssMask) | ((regs[.ESP] &+ 4) & ssMask)
        return res
    }
    func pop() throws -> DWord {
        let res = try ldStack()
        regs[.ESP] = (regs[.ESP] & ~ssMask) | ((regs[.ESP] &+ 4) & ssMask)
        return res
    }
    func ld16Stack() throws -> Word {
        lax = ssBase &+ (regs[.ESP] & ssMask)
        return try ld16ReadonlyCpl3()
    }
    func ldStack() throws -> DWord {
        lax = ssBase &+ (regs[.ESP] & ssMask)
        return try ldReadonlyCpl3()
    }
}
