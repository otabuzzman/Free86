extension Free86 {
    func applyAddressingForm(computeEffectiveAddress: Bool = false) {
        var sreg: SegmentRegister.Name
        var offset: DWord  // effective address
        var exp: Int?  // ESP or EBP register
        if ipr.isFlagRaised(.addressSizeOverride) {
            if modRM.mod == 0 && modRM.rM == 6 {
                offset = DWord(fetch16())
                sreg = .DS
            } else {
                switch modRM.mod {
                case 0:
                    offset = 0
                case 1:
                    offset = DWord(fetch8()).signExtendedByte
                default:
                    offset = DWord(fetch16())
                }
                switch modRM.rM {
                case 0:
                    offset = (offset &+ regs[.EBX] &+ regs[.ESI]) & 0xffff
                    sreg = .DS
                case 1:
                    offset = (offset &+ regs[.EBX] &+ regs[.EDI]) & 0xffff
                    sreg = .DS
                case 2:
                    offset = (offset &+ regs[.EBP] &+ regs[.ESI]) & 0xffff
                    sreg = .SS
                case 3:
                    offset = (offset &+ regs[.EBP] &+ regs[.EDI]) & 0xffff
                    sreg = .SS
                case 4:
                    offset = (offset &+ regs[.ESI]) & 0xffff
                    sreg = .DS
                case 5:
                    offset = (offset &+ regs[.EDI]) & 0xffff
                    sreg = .DS
                case 6:
                    offset = (offset &+ regs[.EBP]) & 0xffff
                    sreg = .SS
                case 7:
                    fallthrough
                default:
                    offset = (offset &+ regs[.EBX]) & 0xffff
                    sreg = .DS
                }
            }
            if ipr.segmentOverride {
                sreg = SegmentRegister.Name(rawValue: ipr.segmentRegister)!  // save to force-unwrap
            }
        } else {
            switch modRM.modRM {
            case 0x00, 0x01, 0x02, 0x03, 0x06, 0x07:
                exp = modRM.rM
                offset = regs[modRM.rM]
            case 0x04:
                sib = fetch8()
                if sib.base.isGeneralRegister(.EBP) {
                    offset = fetch()
                } else {
                    exp = sib.base
                    offset = regs[sib.base]
                }
                if !sib.index.isGeneralRegister(.ESP) {
                    offset = offset &+ (regs[sib.index] << sib.scale)
                }
            case 0x05:
                offset = fetch()
            case 0x08, 0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x0f:
                u = DWord(fetch8()).signExtendedByte
                exp = modRM.rM
                offset = regs[modRM.rM] &+ u
            case 0x0c:
                sib = fetch8()
                u = DWord(fetch8()).signExtendedByte
                exp = sib.base
                offset = regs[sib.base] &+ u
                if !sib.index.isGeneralRegister(.ESP) {
                    offset = offset &+ (regs[sib.index] << sib.scale)
                }
            case 0x14:
                sib = fetch8()
                offset = fetch()
                exp = sib.base
                offset = regs[sib.base] &+ offset
                if !sib.index.isGeneralRegister(.ESP) {
                    offset = offset &+ (regs[sib.index] << sib.scale)
                }
            case 0x10, 0x11, 0x12, 0x13, 0x15, 0x16, 0x17:
                fallthrough
            default:
                offset = fetch()
                offset = regs[modRM.rM] &+ offset
            }
            if ipr.segmentOverride {
                sreg = SegmentRegister.Name(rawValue: ipr.segmentRegister)!  // save to force-unwrap
            } else {
                if let exp = exp, exp.isGeneralRegister(.ESP) || exp.isGeneralRegister(.EBP) {
                    sreg = .SS
                } else {
                    sreg = .DS
                }
            }
        }
        if computeEffectiveAddress || x8664LongMode && !ipr.isFlagRaised(.addressSizeOverride) && !ipr.segmentOverride {
            lax = offset
        } else {
            lax = segs[sreg].shadow.base &+ offset
        }
    }
    func ldMemoryOffset(_ writable: Bool) throws {
        var offset: DWord
        var stride: DWord
        var notok: Bool
        if !ipr.isFlagRaised(.addressSizeOverride) {
            offset = DWord(fetch())
            stride = 4  // 32 bit mode
        } else {
            offset = DWord(fetch16())
            stride = 2  // 16 bit mode
        }
        if opcode.isEven {
            stride = 1  // 8 bit mode, opcodes A0, A2
        }
        let sreg = ipr.segmentRegister
        /// type checking
        if !isSegmentAccessible(sreg, writable) {
            throw Interrupt(.GP, errorCode: 0)
        }
        /// limit checking
        if segs[sreg].shadow.isDataSegment && segs[sreg].shadow.isFlagRaised(.E) {  // expand-down segment
            notok = offset < segs[sreg].shadow.limit &+ 1
        } else {
            notok = offset > segs[sreg].shadow.limit &+ 1 &- stride
        }
        if notok {
            if sreg == .SS {
                throw Interrupt(.SS, errorCode: 0)
            } else {
                throw Interrupt(.GP, errorCode: 0)
            }
        }
        lax = segs[sreg].shadow.base &+ offset
    }
    func setSegmentRegister(_ sreg: SegmentRegister.Name, _ selector: SegmentSelector) throws {
        if cr0.isRealOrV86Mode {
            setSegmentRegisterRealOrV86Mode(sreg, selector)
        } else {
            try setSegmentRegisterProtectedMode(sreg, selector)
        }
    }
    func setSegmentRegisterRealOrV86Mode(_ sreg: SegmentRegister.Name, _ selector: SegmentSelector) {
        let la = LinearAddress(selector) << 4
        if eflags.isFlagRaised(.VM) {
            var xsd = SegmentDescriptor(la, 0xffff, .DataRWAccessed, 3)
            xsd.setFlag(.G)
            xsd.setFlag(.S)
            segs[sreg] = SegmentRegister(selector, xsd)
        } else {
            segs[sreg] = SegmentRegister(selector, SegmentDescriptor(la, 0xffff, .none, 0))
        }
    }
    func setSegmentRegisterProtectedMode(_ sreg: SegmentRegister.Name, _ selector: SegmentSelector) throws {
        let xdt: SegmentRegister
        if selector.isNull {
            if sreg == .SS {
                throw Interrupt(.GP, errorCode: 0)
            }
            segs[sreg] = SegmentRegister(selector, SegmentDescriptor(0))
        } else {
            if selector.isLDT {
                xdt = ldt
            } else {
                xdt = gdt
            }
            if (selector.index + 7) > xdt.shadow.limit {
                throw Interrupt(.GP, errorCode: DWord(selector.indexAndTI))
            }
            lax = xdt.shadow.base + DWord(selector.index)
            var xsd = SegmentDescriptor(try ld64ReadonlyCplX())
            if xsd.isSystemSegment {
                throw Interrupt(.GP, errorCode: DWord(selector.indexAndTI))
            }
            if sreg == .SS {
                if xsd.isCodeSegment || !xsd.isFlagRaised(.W) {
                    throw Interrupt(.GP, errorCode: DWord(selector.indexAndTI))
                }
                if (selector.rpl != cpl) || (xsd.dpl != cpl) {
                    throw Interrupt(.GP, errorCode: DWord(selector.indexAndTI))
                }
            } else {
                if xsd.isCodeSegment && !xsd.isFlagRaised(.R) {
                    throw Interrupt(.GP, errorCode: DWord(selector.indexAndTI))
                }
                if xsd.isDataSegment || !xsd.isFlagRaised(.C) {
                    if (xsd.dpl < cpl) || (xsd.dpl < selector.rpl) {
                        throw Interrupt(.GP, errorCode: DWord(selector.indexAndTI))
                    }
                }
            }
            if !xsd.isFlagRaised(.P) {
                if sreg == .SS {
                    throw Interrupt(.SS, errorCode: DWord(selector.indexAndTI))
                } else {
                    throw Interrupt(.NP, errorCode: DWord(selector.indexAndTI))
                }
            }
            if !xsd.isFlagRaised(.A) {
                xsd.setFlag(.A)
                try st64WritableCplX(qword: xsd.qword)
            }
            segs[sreg] = SegmentRegister(selector, xsd)
        }
    }
}
