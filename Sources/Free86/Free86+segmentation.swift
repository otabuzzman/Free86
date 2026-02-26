extension Free86 {
    func segmentTranslation() {
        if x8664LongMode && !ipr.isFlagRaised(.addressSizeOverride) && !ipr.segmentOverride {
            switch modRM.modRM {
            case 0x00, 0x01, 0x02, 0x03, 0x06, 0x07:
                lax = regs[modRM.rM]
                break
            case 0x04:
                sib = fetch8()
                base = sib.base
                if base == GeneralRegister.Name.EBP.rawValue {
                    lax = fetch()
                } else {
                    lax = regs[base]
                }
                if sib.index != GeneralRegister.Name.ESP.rawValue {
                    lax = lax + (regs[sib.index] << sib.scale)
                }
                break
            case 0x05:
                lax = fetch()
                break
            case 0x08, 0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x0f:
                u = fetch8().signExtendedByte
                lax = regs[modRM.rM] + u
                break
            case 0x0c:
                sib = fetch8()
                u = fetch8().signExtendedByte
                base = sib.base
                lax = regs[base] + u
                if sib.index != GeneralRegister.Name.ESP.rawValue {
                    lax = lax + (regs[sib.index] << sib.scale)
                }
                break
            case 0x14:
                sib = fetch8()
                lax = fetch()
                base = sib.base
                lax = regs[base] + lax
                if sib.index != GeneralRegister.Name.ESP.rawValue {
                    lax = lax + (regs[sib.index] << sib.scale)
                }
                break
            case 0x10, 0x11, 0x12, 0x13, 0x15, 0x16, 0x17:
                fallthrough
            default:
                lax = fetch()
                lax = regs[modRM.rM] + lax
                break
            }
            return
        } else if ipr.isFlagRaised(.addressSizeOverride) {
            if modRM.mod == 0 && modRM.rM == 6 {
                lax = fetch16()
                sregDefault = .DS
            } else {
                switch modRM.mod {
                case 0:
                    lax = 0
                    break
                case 1:
                    lax = fetch8().signExtendedByte
                    break
                default:
                    lax = fetch16()
                    break
                }
                switch modRM.rM {
                case 0:
                    lax = (lax + regs[3] + regs[6]) & 0xffff
                    sregDefault = .DS
                    break
                case 1:
                    lax = (lax + regs[3] + regs[7]) & 0xffff
                    sregDefault = .DS
                    break
                case 2:
                    lax = (lax + regs[5] + regs[6]) & 0xffff
                    sregDefault = .SS
                    break
                case 3:
                    lax = (lax + regs[5] + regs[7]) & 0xffff
                    sregDefault = .SS
                    break
                case 4:
                    lax = (lax + regs[6]) & 0xffff
                    sregDefault = .DS
                    break
                case 5:
                    lax = (lax + regs[7]) & 0xffff
                    sregDefault = .DS
                    break
                case 6:
                    lax = (lax + regs[5]) & 0xffff
                    sregDefault = .SS
                    break
                case 7:
                    fallthrough
                default:
                    lax = (lax + regs[3]) & 0xffff
                    sregDefault = .DS
                    break
                }
            }
            sreg = ipr.segmentOverride ? ipr.segmentRegisterIndex : sregDefault
            lax = segs[sreg].shadow.base + lax
            return
        }
        switch modRM.modRM {
            case 0x00, 0x01, 0x02, 0x03, 0x06, 0x07:
                lax = regs[modRM.rM]
                break
            case 0x04:
                sib = fetch8()
                base = sib.base
                if base == GeneralRegister.Name.EBP.rawValue {
                    lax = fetch()
                    base = GeneralRegister.Name.EAX.rawValue
                } else {
                    lax = regs[base]
                }
                if sib.index != GeneralRegister.Name.ESP.rawValue {
                    lax = lax + (regs[sib.index] << sib.scale)
                }
                break
            case 0x05:
                lax = fetch()
                base = GeneralRegister.Name.EAX.rawValue
                break
            case 0x08, 0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x0f:
                u = fetch8().signExtendedByte
                lax = regs[modRM.rM] + u
                break
            case 0x0c:
                sib = fetch8()
                u = fetch8().signExtendedByte
                base = sib.base
                lax = regs[base] + u
                if sib.index != GeneralRegister.Name.ESP.rawValue {
                    lax = lax + (regs[sib.index] << sib.scale)
                }
                break
            case 0x14:
                sib = fetch8()
                lax = fetch()
                base = sib.base
                lax = regs[base] + lax
                if sib.index != GeneralRegister.Name.ESP.rawValue {
                    lax = lax + (regs[sib.index] << sib.scale)
                }
                break
            case 0x10, 0x11, 0x12, 0x13, 0x15, 0x16, 0x17:
                fallthrough
            default:
                lax = fetch()
                lax = regs[modRM.rM] + lax
                break
        }
        if ipr.segmentOverride {
            sreg = ipr.segmentRegisterIndex
        } else {
            if base == GeneralRegister.Name.ESP.rawValue || base == GeneralRegister.Name.EBP.rawValue {
                sreg = .SS
            } else {
                sreg = .DS
            }
        }
        lax = segs[sreg].shadow.base + lax
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
    func setSegmentRegister(_ sreg: SegmentRegister.Name, _ selector: SegmentSelector) throws {
        if !cr0.isProtectedMode {
            setSegmentRegisterRealOrV86Mode(sreg, selector)
        } else {
            try setSegmentRegisterProtectedMode(sreg, selector)
        }
    }
    func setSegmentRegisterRealOrV86Mode(_ sreg: SegmentRegister.Name, _ selector: SegmentSelector) {
        if eflags.isFlagRaised(.VM) {
            var xsd = SegmentDescriptor(selector << 4, 0xffff, .DataRWAccessed, 3)
            xsd.raiseBit(SegmentDescriptorFlag.G.rawValue)
            xsd.raiseBit(SegmentDescriptorFlag.S.rawValue)
            segs[sreg] = SegmentRegister(selector, xsd)
        } else {
            segs[sreg] = SegmentRegister(selector, SegmentDescriptor(selector << 4, 0xffff, .zero, 0))
        }
    }
    func setSegmentRegisterProtectedMode(_ sreg: SegmentRegister.Name, _ selector: SegmentSelector) throws {
        let xdt: SegmentRegister
        if selector.index == 0 {
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
            let dti = DWord(selector.index)
            if (dti + 7) > xdt.shadow.limit {
                throw Interrupt(.GP, errorCode: DWord(selector.index))
            }
            lax = xdt.shadow.base + dti
            let xsd = SegmentDescriptor(try ld64ReadonlyCplX())
            if xsd.isSystemSegment {
                throw Interrupt(.GP, errorCode: DWord(selector.index))
            }
            if sreg == .SS {
                if xsd.isCodeSegment || !xsd.isFlagRaised(.W) {
                    throw Interrupt(.GP, errorCode: DWord(selector.index))
                }
                if (selector.rpl != cpl) || (xsd.dpl != cpl) {
                    throw Interrupt(.GP, errorCode: DWord(selector.index))
                }
            } else {
                if xsd.isCodeSegment && !xsd.isFlagRaised(.R) {
                    throw Interrupt(.GP, errorCode: DWord(selector.index))
                }
                if xsd.isDataSegment && xsd.isFlagRaised(.E) {
                    if (xsd.dpl < cpl) || (xsd.dpl < selector.rpl) {
                        throw Interrupt(.GP, errorCode: DWord(selector.index))
                    }
                }
            }
            if !xsd.isFlagRaised(.P) {
                if sreg == .SS {
                    throw Interrupt(.SS, errorCode: DWord(selector.index))
                } else {
                    throw Interrupt(.NP, errorCode: DWord(selector.index))
                }
            }
            if !xsd.isFlagRaised(.A) {
                xsd.setFlag(.A)
                try st64WritableCplX(xsd.qword)
            }
            segs[sreg] = SegmentRegister(selector, xsd)
        }
    }
}
