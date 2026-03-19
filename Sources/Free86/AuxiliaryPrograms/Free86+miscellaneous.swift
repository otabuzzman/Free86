extension Free86 {
    func auxLdtr(_ selector: SegmentSelector) throws {
        if selector.isNull {
            ldt = SegmentRegister(selector, SegmentDescriptor(0))
        } else {
            if selector.isLDT {
                throw Interrupt(.GP, errorCode: DWord(selector.indexTI))
            }
            if (selector.index + 7) > gdt.shadow.limit {
                throw Interrupt(.GP, errorCode: DWord(selector.indexTI))
            }
            lax = gdt.shadow.base + DWord(selector.index)
            let xsd = SegmentDescriptor(try ld64ReadonlyCplX())
            if !xsd.isSystemSegment || !xsd.isType(.LDT) {
                throw Interrupt(.GP, errorCode: DWord(selector.indexTI))
            }
            if !xsd.isFlagRaised(.P) {
                throw Interrupt(.NP, errorCode: DWord(selector.indexTI))
            }
            ldt = SegmentRegister(selector, xsd)
        }
    }
    func auxLtr(_ selector: SegmentSelector) throws {
        if selector.isNull {
            tr = SegmentRegister(selector, SegmentDescriptor(0))
        } else {
            if !selector.isGDT {
                throw Interrupt(.GP, errorCode: DWord(selector.indexTI))
            }
            if (selector.index + 7) > gdt.shadow.limit {
                throw Interrupt(.GP, errorCode: DWord(selector.indexTI))
            }
            lax = gdt.shadow.base + DWord(selector.index)
            var xsd = SegmentDescriptor(try ld64ReadonlyCplX())
            if !xsd.isType(.TSSAvailable) && !xsd.isType(.TSS16Available) {
                throw Interrupt(.GP, errorCode: DWord(selector.indexTI))
            }
            if !xsd.isFlagRaised(.P) {
                throw Interrupt(.NP, errorCode: DWord(selector.indexTI))
            }
            tr = SegmentRegister(selector, xsd)
            xsd.upper.setBit(9)  /// bit 9 distinguishes available (0)/ busy (1) TSS
            try st64WritableCplX(qword: xsd.qword)
        }
    }
    func auxLarLsl(_ o32: Bool, _ isLsl: Bool) throws {
        let selector: SegmentSelector
        if !cr0.isProtectedMode || eflags.isFlagRaised(.VM) {
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
            osmSrc.setFlag(.ZF, .zero)
        } else {
            osmSrc.setFlag(.ZF)
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
        if selector.isNull {
            return notok
        }
        let xsd = try ldXdtEntry(selector)
        if xsd.qword == 0 {
            return notok
        }
        let type = SegmentDescriptorType(rawValue: xsd.type)
        if xsd.isSystemSegment {
            switch type {
            case .TSS16Available:
                fallthrough
            case .LDT:
                fallthrough
            case .TSS16Busy:
                fallthrough
            case .TSSAvailable:
                fallthrough
            case .TSSBusy:
                break
            case .CallGate16:
                fallthrough
            case .TaskGate:
                fallthrough
            case .CallGate:
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
        } else {
            switch type {
            case .DataRO:
                fallthrough
            case .DataROAccessed:
                fallthrough
            case .DataRW:
                fallthrough
            case .DataRWAccessed:
                if (xsd.dpl < cpl) || (xsd.dpl < selector.rpl) {
                    return notok
                }
                fallthrough
            default:
                break
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
            osmSrc.setFlag(.ZF)
        } else {
            osmSrc.setFlag(.ZF, .zero)
        }
        osmDst = ((osmSrc >> 6) & 1) ^ 1
        osm = 24
    }
    func isSegmentAccessible(_ selector: SegmentSelector, _ writable: Bool) throws -> Bool {
        if selector.isNull {
            return false
        }
        let xsd = try ldXdtEntry(selector)
        if xsd.qword == 0 {
            return false
        }
        if xsd.isSystemSegment {
            return false
        }
        if xsd.isCodeSegment {
            if writable {
                return false
            } else {
                if !xsd.isFlagRaised(.C) {
                    if (xsd.dpl < cpl) || (xsd.dpl < selector.rpl) {
                        return false
                    }
                }
                if !xsd.isFlagRaised(.R) {
                    return true
                }
            }
        } else {
            if writable && !xsd.isFlagRaised(.W) {
                return false
            }
            if (xsd.dpl < cpl) || (xsd.dpl < selector.rpl) {
                return false
            }
        }
        return true
    }
    func auxArpl() throws {
        if !cr0.isProtectedMode || eflags.isFlagRaised(.VM) {
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
                try st16WritableCpl3(word: u)
            }
            osmSrc.setFlag(.ZF)
        } else {
            osmSrc.setFlag(.ZF, .zero)
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
        lax = ssBase &+ ((regs[.ESP] &- 16) & ssMask)
        for reg in (0...7).reversed() {
            r = regs[reg]
            try st16WritableCpl3(word: r)
            lax = lax &+ 2
        }
        regs[.ESP] = (regs[.ESP] & ~ssMask) | ((regs[.ESP] &- 16) & ssMask)
    }
    func auxPusha() throws {
        lax = ssBase &+ ((regs[.ESP] &- 32) & ssMask)
        for reg in (0...7).reversed() {
            r = regs[reg]
            try stWritableCpl3(dword: r)
            lax = lax &+ 4
        }
        regs[.ESP] = (regs[.ESP] & ~ssMask) | ((regs[.ESP] &- 32) & ssMask)
    }
    func aux16Popa() throws {
        lax = ssBase &+ (regs[.ESP] & ssMask)
        for reg in (0...7).reversed() {
            if !reg.isGeneralRegister(.ESP) {
                regs[reg].lowerHalf = DWord(try ld16ReadonlyCpl3())
            }
            lax = lax &+ 2
        }
        regs[.ESP] = (regs[.ESP] & ~ssMask) | ((regs[.ESP] &+ 16) & ssMask)
    }
    func auxPopa() throws {
        lax = ssBase &+ (regs[.ESP] & ssMask)
        for reg in (0...7).reversed() {
            if !reg.isGeneralRegister(.ESP) {
                regs[reg] = try ldReadonlyCpl3()
            }
            lax = lax &+ 4
        }
        regs[.ESP] = (regs[.ESP] & ~ssMask) | ((regs[.ESP] &+ 32) & ssMask)
    }
    func aux16Leave() throws {
        let ebp = regs[.EBP]
        lax = ssBase &+ (ebp & ssMask)
        regs[.EBP].lowerHalf = DWord(try ld16ReadonlyCpl3())
        regs[.ESP] = (regs[.ESP] & ~ssMask) | ((ebp &+ 2) & ssMask)
    }
    func auxLeave() throws {
        let ebp = regs[.EBP]
        lax = ssBase &+ (ebp & ssMask)
        regs[.EBP] = try ldReadonlyCpl3()
        regs[.ESP] = (regs[.ESP] & ~ssMask) | ((ebp &+ 4) & ssMask)
    }
    func aux16Enter() throws {
        imm16 = fetch16()
        imm = DWord(fetch8() & 0x1f)
        var esp = regs[.ESP]
        var ebp = regs[.EBP]
        esp = esp &- 2
        lax = ssBase &+ (esp & ssMask)
        try st16WritableCpl3(word: ebp)
        let exp = esp
        if imm != 0 {
            while imm > 1 {
                ebp = ebp &- 2
                lax = ssBase &+ (ebp & ssMask)
                m16 = try ld16ReadonlyCpl3()
                esp = esp &- 2
                lax = ssBase &+ (esp & ssMask)
                try st16WritableCpl3(word: m16)
                imm -= 1
            }
            esp = esp &- 2
            lax = ssBase &+ (esp & ssMask)
            try st16WritableCpl3(word: exp)
        }
        esp = esp &- DWord(imm16)
        lax = ssBase &+ (esp & ssMask)
        _ = try ld16WritableCpl3()
        regs[.EBP] = (regs[.EBP] & ~ssMask) | (exp & ssMask)
        regs[.ESP] = (regs[.ESP] & ~ssMask) | (esp & ssMask)
    }
    func auxEnter() throws {
        imm16 = fetch16()
        imm = DWord(fetch8() & 0x1f)
        var esp = regs[.ESP]
        var ebp = regs[.EBP]
        esp = esp &- 4
        lax = ssBase &+ (esp & ssMask)
        try stWritableCpl3(dword: ebp)
        let exp = (ebp & ~ssMask) | (esp & ssMask)
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
        esp = esp &- DWord(imm16)
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
        imm16 = try ld16ReadonlyCpl3()
        try setSegmentRegister(sreg, SegmentSelector(imm16))
        regs[modRM.reg].lowerHalf = imm
    }
    func ldFarPointer(_ sreg: SegmentRegister.Name) throws {
        modRM = fetch8()
        segmentTranslation()
        imm = try ldReadonlyCpl3()
        lax = lax &+ 4
        imm16 = try ld16ReadonlyCpl3()
        try setSegmentRegister(sreg, SegmentSelector(imm16))
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
        try st16WritableCpl3(word: word)
        regs[.ESP] = (regs[.ESP] & ~ssMask) | (esp & ssMask)
    }
    func push16(_ word: DWord) throws {
        try push16(Word(word & 0xffff))
    }
    func push(_ dword: Word) throws {
        try push(DWord(dword))
    }
    func push(_ dword: DWord) throws {
        let esp = regs[.ESP] &- 4
        lax = ssBase &+ (esp & ssMask)
        try stWritableCpl3(dword: dword)
        regs[.ESP] = (regs[.ESP] & ~ssMask) | (esp & ssMask)
    }
    func pop16() throws -> Word {
        let res = try ld16Stack()
        regs[.ESP] = (regs[.ESP] & ~ssMask) | ((regs[.ESP] &+ 2) & ssMask)
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
