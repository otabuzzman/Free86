extension Free86 {
    func aux16Ins() throws {
        if (cpl > eflags.iopl) {
            throw Interrupt(13)
        }
        let mask = ipr.operandSizeMask
        let edi = regs[.EDI]
        let edx = regs[.EDX] & 0xffff
        if ipr.isFlagRaised(.operandSizeOverride) || ipr.isFlagRaised(.addressSizeOverride) {
            var ecx = regs[.ECX];
            if ((ecx & mask) == 0) {
                return
            }
            u = io?[edx] ?? 0
            lax = segs[.ES].shadow.base + (edi & mask)
            try st8WritableCpl3(byte: Byte(u))
            regs[.EDI] = (edi & ~mask) | ((edi + (DWord(truncatingIfNeeded: df) << 0)) & mask)
            regs[.ECX] = (ecx & ~mask) | ((ecx - 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            u = io?[edx] ?? 0
            lax = segs[.ES].shadow.base + (edi & mask)
            try st8WritableCpl3(byte: Byte(u))
            regs[.EDI] = (edi & ~mask) | ((edi + (DWord(truncatingIfNeeded: df) << 0)) & mask)
        }
    }
    func aux16Outs() throws {
        if (cpl > eflags.iopl) {
            throw Interrupt(13)
        }
        let mask = ipr.operandSizeMask
        let sreg = ipr.segmentRegisterIndex
        let esi = regs[.ESI]
        let edx = regs[.EDX] & 0xffff
        if ipr.isFlagRaised(.operandSizeOverride) || ipr.isFlagRaised(.addressSizeOverride) {
            var ecx = regs[.ECX]
            if ((ecx & mask) == 0) {
                return
            }
            lax = segs[sreg].shadow.base + (esi & mask)
            u = DWord(try ld8ReadonlyCpl3())
            io?[edx] = u
            regs[.ESI] = (esi & ~mask) | ((esi + (DWord(truncatingIfNeeded: df) << 0)) & mask);
            regs[.ECX] = (ecx & ~mask) | ((ecx - 1) & mask)
            ecx = regs[.ECX]
            if (ecx & mask) != 0 {
                far = farStart
            }
        } else {
            lax = segs[sreg].shadow.base + (esi & mask);
            u = DWord(try ld8ReadonlyCpl3())
            io?[edx] = u
            regs[.ESI] = (esi & ~mask) | ((esi + (DWord(truncatingIfNeeded: df) << 0)) & mask);
        }
    }
    func aux16Movs() {
    }
    func aux16Stos() {
    }
    func aux16Cmps() {
    }
    func aux16Lods() {
    }
    func aux16Scas() {
    }
    func auxInsb() {
    }
    func auxOutsb() {
    }
    func auxMovsb() {
    }
    func auxStosb() {
    }
    func auxCmpsb() {
    }
    func auxLodsb() {
    }
    func auxScasb() {
    }
    func auxInsw() {
    }
    func auxOutsw() {
    }
    func auxMovsw() {
    }
    func auxStosw() {
    }
    func auxCmpsw() {
    }
    func auxLodsw() {
    }
    func auxScasw() {
    }
    func auxInsd() {
    }
    func auxOutsd() {
    }
    func auxMovsd() {
    }
    func auxStosd() {
    }
    func auxCmpsd() {
    }
    func auxLodsd() {
    }
    func auxScasd() {
    }
}
