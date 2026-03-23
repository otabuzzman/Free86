extension Free86 {
    func segmentTranslation() {
        var sreg: SegmentRegister.Name, exp: Int = 0
        if x8664LongMode && !ipr.isFlagRaised(.addressSizeOverride) && !ipr.segmentOverride {
            switch modRM.modRM {
            case 0x00, 0x01, 0x02, 0x03, 0x06, 0x07:
                lax = regs[modRM.rM]
                break
            case 0x04:
                sib = fetch8()
                if sib.base.isGeneralRegister(.EBP) {
                    lax = fetch()
                } else {
                    lax = regs[sib.base]
                }
                if !sib.index.isGeneralRegister(.ESP) {
                    lax = lax &+ (regs[sib.index] << sib.scale)
                }
                break
            case 0x05:
                lax = fetch()
                break
            case 0x08, 0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x0f:
                u = DWord(fetch8()).signExtendedByte
                lax = regs[modRM.rM] &+ u
                break
            case 0x0c:
                sib = fetch8()
                u = DWord(fetch8()).signExtendedByte
                lax = regs[sib.base] &+ u
                if !sib.index.isGeneralRegister(.ESP) {
                    lax = lax &+ (regs[sib.index] << sib.scale)
                }
                break
            case 0x14:
                sib = fetch8()
                lax = fetch()
                lax = regs[sib.base] &+ lax
                if !sib.index.isGeneralRegister(.ESP) {
                    lax = lax &+ (regs[sib.index] << sib.scale)
                }
                break
            case 0x10, 0x11, 0x12, 0x13, 0x15, 0x16, 0x17:
                fallthrough
            default:
                lax = fetch()
                lax = regs[modRM.rM] &+ lax
                break
            }
            return
        }
        if ipr.isFlagRaised(.addressSizeOverride) {
            if modRM.mod == 0 && modRM.rM == 6 {
                lax = DWord(fetch16())
                sreg = .DS
            } else {
                switch modRM.mod {
                case 0:
                    lax = 0
                    break
                case 1:
                    lax = DWord(fetch8()).signExtendedByte
                    break
                default:
                    lax = DWord(fetch16())
                    break
                }
                switch modRM.rM {
                case 0:
                    lax = (lax &+ regs[.EBX] &+ regs[.ESI]) & 0xffff
                    sreg = .DS
                    break
                case 1:
                    lax = (lax &+ regs[.EBX] &+ regs[.EDI]) & 0xffff
                    sreg = .DS
                    break
                case 2:
                    lax = (lax &+ regs[.EBP] &+ regs[.ESI]) & 0xffff
                    sreg = .SS
                    break
                case 3:
                    lax = (lax &+ regs[.EBP] &+ regs[.EDI]) & 0xffff
                    sreg = .SS
                    break
                case 4:
                    lax = (lax &+ regs[.ESI]) & 0xffff
                    sreg = .DS
                    break
                case 5:
                    lax = (lax &+ regs[.EDI]) & 0xffff
                    sreg = .DS
                    break
                case 6:
                    lax = (lax &+ regs[.EBP]) & 0xffff
                    sreg = .SS
                    break
                case 7:
                    fallthrough
                default:
                    lax = (lax &+ regs[.EBX]) & 0xffff
                    sreg = .DS
                    break
                }
            }
            if ipr.segmentOverride {
                sreg = SegmentRegister.Name(rawValue: ipr.segmentRegister)!  // save to force-unwrap
            }
            lax = segs[sreg].shadow.base &+ lax
            return
        }
        switch modRM.modRM {
            case 0x00, 0x01, 0x02, 0x03, 0x06, 0x07:
                exp = modRM.rM
                lax = regs[modRM.rM]
                break
            case 0x04:
                sib = fetch8()
                exp = sib.base
                if sib.base.isGeneralRegister(.EBP) {
                    lax = fetch()
                    exp = 0
                } else {
                    lax = regs[sib.base]
                }
                if !sib.index.isGeneralRegister(.ESP) {
                    lax = lax &+ (regs[sib.index] << sib.scale)
                }
                break
            case 0x05:
                lax = fetch()
                break
            case 0x08, 0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x0f:
                u = DWord(fetch8()).signExtendedByte
                exp = modRM.rM
                lax = regs[modRM.rM] &+ u
                break
            case 0x0c:
                sib = fetch8()
                u = DWord(fetch8()).signExtendedByte
                exp = sib.base
                lax = regs[sib.base] &+ u
                if !sib.index.isGeneralRegister(.ESP) {
                    lax = lax &+ (regs[sib.index] << sib.scale)
                }
                break
            case 0x14:
                sib = fetch8()
                lax = fetch()
                exp = sib.base
                lax = regs[sib.base] &+ lax
                if !sib.index.isGeneralRegister(.ESP) {
                    lax = lax &+ (regs[sib.index] << sib.scale)
                }
                break
            case 0x10, 0x11, 0x12, 0x13, 0x15, 0x16, 0x17:
                fallthrough
            default:
                lax = fetch()
                lax = regs[modRM.rM] &+ lax
                break
        }
        if ipr.segmentOverride {
            sreg = SegmentRegister.Name(rawValue: ipr.segmentRegister)!  // save to force-unwrap
        } else {
            if exp.isGeneralRegister(.ESP) || exp.isGeneralRegister(.EBP) {
                sreg = .SS
            } else {
                sreg = .DS
            }
        }
        lax = segs[sreg].shadow.base &+ lax
    }
    func ldMemoryOffset(_ writable: Bool) throws {
        var la: QWord
        var notok: Bool
        var stride: QWord
        if !ipr.isFlagRaised(.addressSizeOverride) {
            la = QWord(fetch())
            stride = 4  // 32 bit mode
        } else {
            la = QWord(fetch16())
            stride = 2  // 16 bit mode
        }
        if opcode.isEven {
            stride = 1  // 8 bit mode, opcodes A0, A2
        }
        let sreg = ipr.segmentRegister
        /// type checking
        if sreg.isSegmentRegister(.CS) {  // code segment, WR requested or CS not readable
            notok = writable || !segs[sreg].shadow.isFlagRaised(.R)
        } else {  // data segment, WR requested and DS not writable
            notok = writable && !segs[sreg].shadow.isFlagRaised(.W)
        }
        if notok {
            throw Interrupt(.GP, errorCode: 0)
        }
        la = QWord(segs[sreg].shadow.base) &+ la
        /// limit checking
        let b = QWord(segs[sreg].shadow.base)
        let l = QWord(segs[sreg].shadow.limit)
        if segs[sreg].shadow.isFlagRaised(.E) {  // expand-down segment
            notok = la < b &+ l &+ 1
        } else {
            notok = la > b &+ l &+ 1 &- stride
        }
        if notok {
            if sreg.isSegmentRegister(.SS) {
                throw Interrupt(.SS, errorCode: 0)
            } else {
                throw Interrupt(.GP, errorCode: 0)
            }
        }
        lax = DWord(truncatingIfNeeded: la)
    }
    func setSegmentRegister(_ sreg: SegmentRegister.Name, _ selector: SegmentSelector) throws {
        if !cr0.isProtectedMode {
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
                throw Interrupt(.GP, errorCode: DWord(selector.indexTI))
            }
            lax = xdt.shadow.base + DWord(selector.index)
            var xsd = SegmentDescriptor(try ld64ReadonlyCplX())
            if xsd.isSystemSegment {
                throw Interrupt(.GP, errorCode: DWord(selector.indexTI))
            }
            if sreg == .SS {
                if xsd.isCodeSegment || !xsd.isFlagRaised(.W) {
                    throw Interrupt(.GP, errorCode: DWord(selector.indexTI))
                }
                if (selector.rpl != cpl) || (xsd.dpl != cpl) {
                    throw Interrupt(.GP, errorCode: DWord(selector.indexTI))
                }
            } else {
                if xsd.isCodeSegment && !xsd.isFlagRaised(.R) {
                    throw Interrupt(.GP, errorCode: DWord(selector.indexTI))
                }
                if xsd.isDataSegment || !xsd.isFlagRaised(.C) {
                    if (xsd.dpl < cpl) || (xsd.dpl < selector.rpl) {
                        throw Interrupt(.GP, errorCode: DWord(selector.indexTI))
                    }
                }
            }
            if !xsd.isFlagRaised(.P) {
                if sreg == .SS {
                    throw Interrupt(.SS, errorCode: DWord(selector.indexTI))
                } else {
                    throw Interrupt(.NP, errorCode: DWord(selector.indexTI))
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
