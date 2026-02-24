extension Free86 {
    func aux_INSB() throws {
        if (cpl > eflags.iopl) {
            throw Interrupt(.GP, errorCode: 0)
        }
        let mask = ipr.operandSizeMask
        let edi = regs[.EDI]
        let edx = regs[.EDX] & 0xffff
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = io?[edx] ?? 0
            lax = segs[.ES].shadow.base &+ (edi & mask)
            try st8WritableCpl3(byte: Byte(truncatingIfNeeded: u))
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 0)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = io?[edx] ?? 0
            lax = segs[.ES].shadow.base &+ (edi & mask)
            try st8WritableCpl3(byte: Byte(truncatingIfNeeded: u))
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 0)) & mask)
        }
    }
    func aux_OUTSB() throws {
        if (cpl > eflags.iopl) {
            throw Interrupt(.GP, errorCode: 0)
        }
        let mask = ipr.operandSizeMask
        let sreg = ipr.segmentRegisterIndex
        let esi = regs[.ESI]
        let edx = regs[.EDX] & 0xffff
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            lax = segs[sreg].shadow.base &+ (esi & mask)
            u = DWord(try ld8ReadonlyCpl3())
            io?[edx] = u
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 0)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            lax = segs[sreg].shadow.base &+ (esi & mask)
            u = DWord(try ld8ReadonlyCpl3())
            io?[edx] = u
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 0)) & mask)
        }
    }
    func aux_MOVSB() throws {
        let mask = ipr.operandSizeMask
        let sreg = ipr.segmentRegisterIndex
        let esi = regs[.ESI]
        let edi = regs[.EDI]
        lax = segs[sreg].shadow.base &+ (esi & mask)
        let la = segs[.ES].shadow.base &+ (edi & mask)
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = DWord(try ld8ReadonlyCpl3())
            lax = la
            try st8WritableCpl3(byte: Byte(truncatingIfNeeded: u))
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 0)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 0)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = DWord(try ld8ReadonlyCpl3())
            lax = la
            try st8WritableCpl3(byte: Byte(truncatingIfNeeded: u))
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 0)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 0)) & mask)
        }
    }
    func aux_STOSB() throws {
        let mask = ipr.operandSizeMask
        let edi = regs[.EDI]
        lax = segs[.ES].shadow.base &+ (edi & mask)
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            try st8WritableCpl3(byte: Byte(truncatingIfNeeded: regs[.EAX]))
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 0)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            try st8WritableCpl3(byte: Byte(truncatingIfNeeded: regs[.EAX]))
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 0)) & mask)
        }
    }
    func aux_CMPSB() throws {
        let mask = ipr.operandSizeMask
        let sreg = ipr.segmentRegisterIndex
        let esi = regs[.ESI]
        let edi = regs[.EDI]
        lax = segs[sreg].shadow.base &+ (esi & mask)
        let la = segs[.ES].shadow.base &+ (edi & mask)
        operation = 7
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = DWord(try ld8ReadonlyCpl3())
            lax = la
            v = DWord(try ld8ReadonlyCpl3())
            calculate8(u, v)
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 0)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 0)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
             if ipr.isFlagRaised(.repzStringOperation) {
                if !(osmDst == 0) {
                    return
                }
            } else {
                if osmDst == 0 {
                    return
                }
            }
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = DWord(try ld8ReadonlyCpl3())
            lax = la
            v = DWord(try ld8ReadonlyCpl3())
            calculate8(u, v)
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 0)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 0)) & mask)
        }
    }
    func aux_LODSB() throws {
        let mask = ipr.operandSizeMask
        let sreg = ipr.segmentRegisterIndex
        let esi = regs[.ESI]
        lax = segs[sreg].shadow.base &+ (esi & mask)
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = DWord(try ld8ReadonlyCpl3())
            regs[.EAX] = (regs[.EAX] & 0xffffff00) | u
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 0)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = DWord(try ld8ReadonlyCpl3())
            regs[.EAX] = (regs[.EAX] & 0xffffff00) | u
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 0)) & mask)
        }
    }
    func aux_SCASB() throws {
        let mask = ipr.operandSizeMask
        let edi = regs[.EDI]
        lax = segs[.ES].shadow.base &+ (edi & mask)
        operation = 7
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = DWord(try ld8ReadonlyCpl3())
            calculate8(regs[.EAX], u)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 0)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
             if ipr.isFlagRaised(.repzStringOperation) {
                if !(osmDst == 0) {
                    return
                }
            } else {
                if osmDst == 0 {
                    return
                }
            }
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = DWord(try ld8ReadonlyCpl3())
            calculate8(regs[.EAX], u)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 0)) & mask)
        }
    }
    func aux_INSW() throws {
        if (cpl > eflags.iopl) {
            throw Interrupt(.GP, errorCode: 0)
        }
        let mask = ipr.operandSizeMask
        let edi = regs[.EDI]
        let edx = regs[.EDX] & 0xffff
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = io?[edx] ?? 0
            lax = segs[.ES].shadow.base &+ (edi & mask)
            try st8WritableCpl3(byte: Byte(truncatingIfNeeded: u))
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = io?[edx] ?? 0
            lax = segs[.ES].shadow.base &+ (edi & mask)
            try st8WritableCpl3(byte: Byte(truncatingIfNeeded: u))
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func aux_OUTSW() throws {
        if (cpl > eflags.iopl) {
            throw Interrupt(.GP, errorCode: 0)
        }
        let mask = ipr.operandSizeMask
        let sreg = ipr.segmentRegisterIndex
        let esi = regs[.ESI]
        let edx = regs[.EDX] & 0xffff
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            lax = segs[sreg].shadow.base &+ (esi & mask)
            u = DWord(try ld16ReadonlyCpl3())
            io?[edx] = u
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            lax = segs[sreg].shadow.base &+ (esi & mask)
            u = DWord(try ld16ReadonlyCpl3())
            io?[edx] = u
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func aux_MOVSW() throws {
        let mask = ipr.operandSizeMask
        let sreg = ipr.segmentRegisterIndex
        let esi = regs[.ESI]
        let edi = regs[.EDI]
        lax = segs[sreg].shadow.base &+ (esi & mask)
        let la = segs[.ES].shadow.base &+ (edi & mask)
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = DWord(try ld16ReadonlyCpl3())
            lax = la
            try st8WritableCpl3(byte: Byte(truncatingIfNeeded: u))
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = DWord(try ld16ReadonlyCpl3())
            lax = la
            try st8WritableCpl3(byte: Byte(truncatingIfNeeded: u))
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func aux_STOSW() throws {
        let mask = ipr.operandSizeMask
        let edi = regs[.EDI]
        lax = segs[.ES].shadow.base &+ (edi & mask)
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            try st16WritableCpl3(word: Word(truncatingIfNeeded: regs[.EAX]))
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            try st16WritableCpl3(word: Word(truncatingIfNeeded: regs[.EAX]))
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func aux_CMPSW() throws {
        let mask = ipr.operandSizeMask
        let sreg = ipr.segmentRegisterIndex
        let esi = regs[.ESI]
        let edi = regs[.EDI]
        lax = segs[sreg].shadow.base &+ (esi & mask)
        let la = segs[.ES].shadow.base &+ (edi & mask)
        operation = 7
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = DWord(try ld16ReadonlyCpl3())
            lax = la
            v = DWord(try ld16ReadonlyCpl3())
            calculate16(u, v)
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
             if ipr.isFlagRaised(.repzStringOperation) {
                if !(osmDst == 0) {
                    return
                }
            } else {
                if osmDst == 0 {
                    return
                }
            }
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = DWord(try ld16ReadonlyCpl3())
            lax = la
            v = DWord(try ld16ReadonlyCpl3())
            calculate16(u, v)
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func aux_LODSW() throws {
        let mask = ipr.operandSizeMask
        let sreg = ipr.segmentRegisterIndex
        let esi = regs[.ESI]
        lax = segs[sreg].shadow.base &+ (esi & mask)
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = DWord(try ld16ReadonlyCpl3())
            regs[.EAX] = u
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = DWord(try ld16ReadonlyCpl3())
            regs[.EAX] = u
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func aux_SCASW() throws {
        let mask = ipr.operandSizeMask
        let edi = regs[.EDI]
        lax = segs[.ES].shadow.base &+ (edi & mask)
        operation = 7
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = DWord(try ld16ReadonlyCpl3())
            calculate16(regs[.EAX], u)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
             if ipr.isFlagRaised(.repzStringOperation) {
                if !(osmDst == 0) {
                    return
                }
            } else {
                if osmDst == 0 {
                    return
                }
            }
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = DWord(try ld16ReadonlyCpl3())
            calculate16(regs[.EAX], u)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func aux16_INS() throws {
        if (cpl > eflags.iopl) {
            throw Interrupt(.GP, errorCode: 0)
        }
        let mask = ipr.operandSizeMask
        let edi = regs[.EDI]
        let edx = regs[.EDX] & 0xffff
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = io?[edx] ?? 0
            lax = segs[.ES].shadow.base &+ (edi & mask)
            try st8WritableCpl3(byte: Byte(truncatingIfNeeded: u))
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = io?[edx] ?? 0
            lax = segs[.ES].shadow.base &+ (edi & mask)
            try st8WritableCpl3(byte: Byte(truncatingIfNeeded: u))
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func aux16_OUTS() throws {
        if (cpl > eflags.iopl) {
            throw Interrupt(.GP, errorCode: 0)
        }
        let mask = ipr.operandSizeMask
        let sreg = ipr.segmentRegisterIndex
        let esi = regs[.ESI]
        let edx = regs[.EDX] & 0xffff
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            lax = segs[sreg].shadow.base &+ (esi & mask)
            u = DWord(try ld16ReadonlyCpl3())
            io?[edx] = u
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            lax = segs[sreg].shadow.base &+ (esi & mask)
            u = DWord(try ld16ReadonlyCpl3())
            io?[edx] = u
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func aux16_MOVS() throws {
        let mask = ipr.operandSizeMask
        let sreg = ipr.segmentRegisterIndex
        let esi = regs[.ESI]
        let edi = regs[.EDI]
        lax = segs[sreg].shadow.base &+ (esi & mask)
        let la = segs[.ES].shadow.base &+ (edi & mask)
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = DWord(try ld16ReadonlyCpl3())
            lax = la
            try st8WritableCpl3(byte: Byte(truncatingIfNeeded: u))
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = DWord(try ld16ReadonlyCpl3())
            lax = la
            try st8WritableCpl3(byte: Byte(truncatingIfNeeded: u))
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func aux16_STOS() throws {
        let mask = ipr.operandSizeMask
        let edi = regs[.EDI]
        lax = segs[.ES].shadow.base &+ (edi & mask)
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            try st16WritableCpl3(word: Word(truncatingIfNeeded: regs[.EAX]))
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            try st16WritableCpl3(word: Word(truncatingIfNeeded: regs[.EAX]))
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func aux16_CMPS() throws {
        let mask = ipr.operandSizeMask
        let sreg = ipr.segmentRegisterIndex
        let esi = regs[.ESI]
        let edi = regs[.EDI]
        lax = segs[sreg].shadow.base &+ (esi & mask)
        let la = segs[.ES].shadow.base &+ (edi & mask)
        operation = 7
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = DWord(try ld16ReadonlyCpl3())
            lax = la
            v = DWord(try ld16ReadonlyCpl3())
            calculate16(u, v)
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
             if ipr.isFlagRaised(.repzStringOperation) {
                if !(osmDst == 0) {
                    return
                }
            } else {
                if osmDst == 0 {
                    return
                }
            }
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = DWord(try ld16ReadonlyCpl3())
            lax = la
            v = DWord(try ld16ReadonlyCpl3())
            calculate16(u, v)
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func aux16_LODS() throws {
        let mask = ipr.operandSizeMask
        let sreg = ipr.segmentRegisterIndex
        let esi = regs[.ESI]
        lax = segs[sreg].shadow.base &+ (esi & mask)
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = DWord(try ld16ReadonlyCpl3())
            regs[.EAX] = (regs[.EAX] & 0xffff0000) | u
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = DWord(try ld16ReadonlyCpl3())
            regs[.EAX] = (regs[.EAX] & 0xffff0000) | u
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func aux16_SCAS() throws {
        let mask = ipr.operandSizeMask
        let edi = regs[.EDI]
        lax = segs[.ES].shadow.base &+ (edi & mask)
        operation = 7
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = DWord(try ld16ReadonlyCpl3())
            calculate16(regs[.EAX], u)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
             if ipr.isFlagRaised(.repzStringOperation) {
                if !(osmDst == 0) {
                    return
                }
            } else {
                if osmDst == 0 {
                    return
                }
            }
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = DWord(try ld16ReadonlyCpl3())
            calculate16(regs[.EAX], u)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func aux_INSD() throws {
        if (cpl > eflags.iopl) {
            throw Interrupt(.GP, errorCode: 0)
        }
        let mask = ipr.operandSizeMask
        let edi = regs[.EDI]
        let edx = regs[.EDX] & 0xffff
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = io?[edx] ?? 0
            lax = segs[.ES].shadow.base &+ (edi & mask)
            try stWritableCpl3(dword: u)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 2)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = io?[edx] ?? 0
            lax = segs[.ES].shadow.base &+ (edi & mask)
            try stWritableCpl3(dword: u)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 2)) & mask)
        }
    }
    func aux_OUTSD() throws {
        if (cpl > eflags.iopl) {
            throw Interrupt(.GP, errorCode: 0)
        }
        let mask = ipr.operandSizeMask
        let sreg = ipr.segmentRegisterIndex
        let esi = regs[.ESI]
        let edx = regs[.EDX] & 0xffff
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            lax = segs[sreg].shadow.base &+ (esi & mask)
            u = try ldReadonlyCpl3()
            io?[edx] = u
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 2)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            lax = segs[sreg].shadow.base &+ (esi & mask)
            u = try ldReadonlyCpl3()
            io?[edx] = u
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 2)) & mask)
        }
    }
    func aux_MOVSD() throws {
        let mask = ipr.operandSizeMask
        let sreg = ipr.segmentRegisterIndex
        let esi = regs[.ESI]
        let edi = regs[.EDI]
        lax = segs[sreg].shadow.base &+ (esi & mask)
        let la = segs[.ES].shadow.base &+ (edi & mask)
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = try ldReadonlyCpl3()
            lax = la
            try stWritableCpl3(dword: u)
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 2)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 2)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = try ldReadonlyCpl3()
            lax = la
            try stWritableCpl3(dword: u)
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 2)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 2)) & mask)
        }
    }
    func aux_STOSD() throws {
        let mask = ipr.operandSizeMask
        let edi = regs[.EDI]
        lax = segs[.ES].shadow.base &+ (edi & mask)
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            try stWritableCpl3(dword: regs[.EAX])
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 2)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            try stWritableCpl3(dword: regs[.EAX])
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 2)) & mask)
        }
    }
    func aux_CMPSD() throws {
        let mask = ipr.operandSizeMask
        let sreg = ipr.segmentRegisterIndex
        let esi = regs[.ESI]
        let edi = regs[.EDI]
        lax = segs[sreg].shadow.base &+ (esi & mask)
        let la = segs[.ES].shadow.base &+ (edi & mask)
        operation = 7
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = try ldReadonlyCpl3()
            lax = la
            v = try ldReadonlyCpl3()
            calculate(u, v)
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 2)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 2)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
             if ipr.isFlagRaised(.repzStringOperation) {
                if !(osmDst == 0) {
                    return
                }
            } else {
                if osmDst == 0 {
                    return
                }
            }
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = try ldReadonlyCpl3()
            lax = la
            v = try ldReadonlyCpl3()
            calculate(u, v)
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 2)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 2)) & mask)
        }
    }
    func aux_LODSD() throws {
        let mask = ipr.operandSizeMask
        let sreg = ipr.segmentRegisterIndex
        let esi = regs[.ESI]
        lax = segs[sreg].shadow.base &+ (esi & mask)
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = try ldReadonlyCpl3()
            regs[.EAX] = u
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 2)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = try ldReadonlyCpl3()
            regs[.EAX] = u
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 2)) & mask)
        }
    }
    func aux_SCASD() throws {
        let mask = ipr.operandSizeMask
        let edi = regs[.EDI]
        lax = segs[.ES].shadow.base &+ (edi & mask)
        operation = 7
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = try ldReadonlyCpl3()
            calculate(regs[.EAX], u)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 2)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
             if ipr.isFlagRaised(.repzStringOperation) {
                if !(osmDst == 0) {
                    return
                }
            } else {
                if osmDst == 0 {
                    return
                }
            }
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = try ldReadonlyCpl3()
            calculate(regs[.EAX], u)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 2)) & mask)
        }
    }
}
