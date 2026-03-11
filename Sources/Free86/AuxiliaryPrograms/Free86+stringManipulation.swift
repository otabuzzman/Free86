extension Free86 {
    func auxInsb() throws {
        if (cpl > eflags.iopl) {
            throw Interrupt(.GP, errorCode: 0)
        }
        let mask = ipr.addressSizeMask
        let edi = regs[.EDI]
        let edx = regs[.EDX] & 0xffff
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = io?[edx] ?? 0
            lax = segs[.ES].shadow.base &+ (edi & mask)
            try st8WritableCpl3(byte: u)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 0)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = io?[edx] ?? 0
            lax = segs[.ES].shadow.base &+ (edi & mask)
            try st8WritableCpl3(byte: u)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 0)) & mask)
        }
    }
    func auxOutsb() throws {
        if (cpl > eflags.iopl) {
            throw Interrupt(.GP, errorCode: 0)
        }
        let mask = ipr.addressSizeMask
        let sreg = ipr.segmentRegister
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
    func auxMovsb() throws {
        let mask = ipr.addressSizeMask
        let sreg = ipr.segmentRegister
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
            try st8WritableCpl3(byte: u)
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
            try st8WritableCpl3(byte: u)
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 0)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 0)) & mask)
        }
    }
    func auxStosb() throws {
        let mask = ipr.addressSizeMask
        let edi = regs[.EDI]
        lax = segs[.ES].shadow.base &+ (edi & mask)
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            try st8WritableCpl3(byte: regs[.EAX])
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 0)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            try st8WritableCpl3(byte: regs[.EAX])
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 0)) & mask)
        }
    }
    func auxCmpsb() throws {
        let mask = ipr.addressSizeMask
        let sreg = ipr.segmentRegister
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
    func auxLodsb() throws {
        let mask = ipr.addressSizeMask
        let sreg = ipr.segmentRegister
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
    func auxScasb() throws {
        let mask = ipr.addressSizeMask
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
    func auxInsw() throws {
        if (cpl > eflags.iopl) {
            throw Interrupt(.GP, errorCode: 0)
        }
        let mask = ipr.addressSizeMask
        let edi = regs[.EDI]
        let edx = regs[.EDX] & 0xffff
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = io?[edx] ?? 0
            lax = segs[.ES].shadow.base &+ (edi & mask)
            try st8WritableCpl3(byte: u)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = io?[edx] ?? 0
            lax = segs[.ES].shadow.base &+ (edi & mask)
            try st8WritableCpl3(byte: u)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func auxOutsw() throws {
        if (cpl > eflags.iopl) {
            throw Interrupt(.GP, errorCode: 0)
        }
        let mask = ipr.addressSizeMask
        let sreg = ipr.segmentRegister
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
    func auxMovsw() throws {
        let mask = ipr.addressSizeMask
        let sreg = ipr.segmentRegister
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
            try st8WritableCpl3(byte: u)
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
            try st8WritableCpl3(byte: u)
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func auxStosw() throws {
        let mask = ipr.addressSizeMask
        let edi = regs[.EDI]
        lax = segs[.ES].shadow.base &+ (edi & mask)
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            try st16WritableCpl3(word: regs[.EAX])
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            try st16WritableCpl3(word: regs[.EAX])
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func auxCmpsw() throws {
        let mask = ipr.addressSizeMask
        let sreg = ipr.segmentRegister
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
    func auxLodsw() throws {
        let mask = ipr.addressSizeMask
        let sreg = ipr.segmentRegister
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
    func auxScasw() throws {
        let mask = ipr.addressSizeMask
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
    func aux16Ins() throws {
        if (cpl > eflags.iopl) {
            throw Interrupt(.GP, errorCode: 0)
        }
        let mask = ipr.addressSizeMask
        let edi = regs[.EDI]
        let edx = regs[.EDX] & 0xffff
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            u = io?[edx] ?? 0
            lax = segs[.ES].shadow.base &+ (edi & mask)
            try st8WritableCpl3(byte: u)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = io?[edx] ?? 0
            lax = segs[.ES].shadow.base &+ (edi & mask)
            try st8WritableCpl3(byte: u)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func aux16Outs() throws {
        if (cpl > eflags.iopl) {
            throw Interrupt(.GP, errorCode: 0)
        }
        let mask = ipr.addressSizeMask
        let sreg = ipr.segmentRegister
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
    func aux16Movs() throws {
        let mask = ipr.addressSizeMask
        let sreg = ipr.segmentRegister
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
            try st8WritableCpl3(byte: u)
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
            try st8WritableCpl3(byte: u)
            regs[.ESI] = (esi & ~mask) | ((esi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func aux16Stos() throws {
        let mask = ipr.addressSizeMask
        let edi = regs[.EDI]
        lax = segs[.ES].shadow.base &+ (edi & mask)
        if ipr.isFlagRaised(.repzStringOperation) || ipr.isFlagRaised(.repnzStringOperation) {
            var ecx = regs[.ECX]
            if (ecx & mask) == 0 {
                return
            }
            try st16WritableCpl3(word: regs[.EAX])
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx &- 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            try st16WritableCpl3(word: regs[.EAX])
            regs[.EDI] = (edi & ~mask) | ((edi &+ (DWord(bitPattern: df) << 1)) & mask)
        }
    }
    func aux16Cmps() throws {
        let mask = ipr.addressSizeMask
        let sreg = ipr.segmentRegister
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
    func aux16Lods() throws {
        let mask = ipr.addressSizeMask
        let sreg = ipr.segmentRegister
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
    func aux16Scas() throws {
        let mask = ipr.addressSizeMask
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
    func auxInsd() throws {
        if (cpl > eflags.iopl) {
            throw Interrupt(.GP, errorCode: 0)
        }
        let mask = ipr.addressSizeMask
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
    func auxOutsd() throws {
        if (cpl > eflags.iopl) {
            throw Interrupt(.GP, errorCode: 0)
        }
        let mask = ipr.addressSizeMask
        let sreg = ipr.segmentRegister
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
    func auxMovsd() throws {
        let mask = ipr.addressSizeMask
        let sreg = ipr.segmentRegister
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
    func auxStosd() throws {
        let mask = ipr.addressSizeMask
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
    func auxCmpsd() throws {
        let mask = ipr.addressSizeMask
        let sreg = ipr.segmentRegister
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
    func auxLodsd() throws {
        let mask = ipr.addressSizeMask
        let sreg = ipr.segmentRegister
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
    func auxScasd() throws {
        let mask = ipr.addressSizeMask
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
