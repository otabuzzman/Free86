extension Free86 {
    func auxAam(_ radix: DWord) throws {
        if radix == 0 {
            throw Interrupt(.DE)
        }
        var al = regs[.EAX] & 0xff
        let ah = al / radix
        al = al % radix
        regs[.EAX] = (regs[.EAX] & ~0xffff) | al | (ah << 8)
        osmDst = al.signExtendedByte
        osm = 12
    }
    func auxAad(_ radix: DWord) {
        var al = regs[.EAX] & 0xff
        let ah = (regs[.EAX] >> 8) & 0xff
        al = (ah &* radix &+ al) & 0xff
        regs[.EAX] = (regs[.EAX] & ~0xffff) | al
        osmDst = al.signExtendedByte
        osm = 12
    }
    func auxAaa() {
        var flags = compileEflags()
        var al = regs[.EAX] & 0xff
        var ah = (regs[.EAX] >> 8) & 0xff
        let of: DWord = al > 0xf9 ? 1 : 0
        if ((al & 0x0f) > 9) || flags.isFlagRaised(.AF) {
            al = (al &+ 6) & 0x0f
            ah = (ah &+ 1 &+ of) & 0xff
            flags.setFlag(.CF)
            flags.setFlag(.AF)
        } else {
            flags.setFlag(.CF, .zero)
            flags.setFlag(.AF, .zero)
            al &= 0x0f
        }
        regs[.EAX] = (regs[.EAX] & ~0xffff) | al | (ah << 8)
        osmSrc = flags
        osmDst = ((osmSrc >> 6) & 1) ^ 1
        osm = 24
    }
    func auxAas() {
        var flags = compileEflags()
        var al = regs[.EAX] & 0xff
        var ah = (regs[.EAX] >> 8) & 0xff
        let of: DWord = al < 6 ? 1 : 0
        if ((al & 0x0f) > 9) || flags.isFlagRaised(.AF) {
            al = (al &- 6) & 0x0f
            ah = (ah &- 1 &- of) & 0xff
            flags.setFlag(.CF)
            flags.setFlag(.AF)
        } else {
            flags.setFlag(.CF, .zero)
            flags.setFlag(.AF, .zero)
            al &= 0x0f
        }
        regs[.EAX] = (regs[.EAX] & ~0xffff) | al | (ah << 8)
        osmSrc = flags
        osmDst = ((osmSrc >> 6) & 1) ^ 1
        osm = 24
    }
    func auxDaa() {
        var flags = compileEflags()
        let f0 = flags.isFlagRaised(.CF)
        let f4 = flags.isFlagRaised(.AF)
        var al = regs[.EAX] & 0xff
        flags = 0
        if ((al & 0x0f) > 9) || f4 {
            al = (al &+ 6) & 0xff
            flags.setFlag(.AF)
        }
        if ((al > 0x9f) || f0) {
            al = (al &+ 0x60) & 0xff
            flags.setFlag(.CF)
        }
        regs[.EAX] = (regs[.EAX] & ~0xff) | al
        if al == 0 {
            flags.setFlag(.ZF)
        }
        flags.setFlag(.PF, Free86.parityLUT[Int(al)])
        if (al & 0x80) != 0 {
            flags.setFlag(.SF)
        }
        osmSrc = flags
        osmDst = ((osmSrc >> 6) & 1) ^ 1
        osm = 24
    }
    func auxDas() {
        var flags = compileEflags()
        let f0 = flags.isFlagRaised(.CF)
        let f4 = flags.isFlagRaised(.AF)
        var al = regs[.EAX] & 0xff
        flags = 0
        let of = al > 0x99
        if ((al & 0x0f) > 9) || f4 {
            flags.setFlag(.AF)
            if (al < 6) || f0 {
                flags.setFlag(.CF)
            }
            al = (al &- 6) & 0xff
        }
        if of || f0 {
            al = (al &- 0x60) & 0xff
            flags.setFlag(.CF)
        }
        regs[.EAX] = (regs[.EAX] & ~0xff) | al
        if al == 0 {
            flags.setFlag(.ZF)
        }
        flags.setFlag(.PF, Free86.parityLUT[Int(al)])
        if (al & 0x80) != 0 {
            flags.setFlag(.SF)
        }
        osmSrc = flags
        osmDst = ((osmSrc >> 6) & 1) ^ 1
        osm = 24
    }
}
