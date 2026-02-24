extension Free86 {
    func aux8Inc(_ byte: DWord) -> DWord {
        if osm < 25 {
            osmPreserved = osm
            osmDstPreserved = osmDst
        }
        osmDst = (byte &+ 1).signExtendedByte
        osm = 25
        return osmDst
    }
    func aux16Inc(_ word: DWord) -> DWord {
        if osm < 25 {
            osmPreserved = osm
            osmDstPreserved = osmDst
        }
        osmDst = (word &+ 1).signExtendedWord
        osm = 26
        return osmDst
    }
    func aux8Dec(_ byte: DWord) -> DWord {
        if osm < 25 {
            osmPreserved = osm
            osmDstPreserved = osmDst
        }
        osmDst = (byte &- 1).signExtendedByte
        osm = 28
        return osmDst
    }
    func aux16Dec(_ word: DWord) -> DWord {
        if osm < 25 {
            osmPreserved = osm
            osmDstPreserved = osmDst
        }
        osmDst = (word &- 1).signExtendedWord
        osm = 29
        return osmDst
    }
    func aux8Div(_ divisor: DWord) throws {
        let d = divisor & 0xff
        let a = regs[.EAX] & 0xffff
        if (a >> 8) >= d {
            throw Interrupt(.DE)
        }
        let q = a / d
        let s = a % d
        regs[.EAX].lowerHalf = (q & 0xff) | (s << 8)
   }
    func aux16Div(_ divisor: DWord) throws {
        let d = divisor & 0xffff
        let a = (regs[.EDX] << 16) | (regs[.EAX] & 0xffff)
        if (a >> 16) >= d {
            throw Interrupt(.DE)
        }
        let q = a / d
        let s = a % d
        regs[.EAX].lowerHalf = q
        regs[.EDX].lowerHalf = s
    }
    func auxDiv(_ dividend: QWord, _ divisor: DWord) throws {
        let uh = DWord(truncatingIfNeeded: dividend >> 32)
        let lh = DWord(truncatingIfNeeded: dividend & 0xffffffff)
        if uh >= divisor {
            throw Interrupt(.DE)
        }
        if uh <= 0x200000 {
            let a: QWord = QWord(uh) &* 4294967296 &+ QWord(lh)
            v = DWord(truncatingIfNeeded: a % QWord(divisor))
            u = DWord(truncatingIfNeeded: a / QWord(divisor))
        } else {
            for _ in 0..<32 {
                let s = (uh >> 31) != 0
                uh = (uh << 1) | (lh >> 31)
                if s || (uh >= divisor) {
                    uh = uh &- divisor
                    lh = (lh << 1) | 1
                } else {
                    lh = lh << 1
                }
            }
            v = uh
            u = lh
        }
    }
    func aux8Idiv(_ divisor: DWord) throws {
        let d = Int32(bitPattern: divisor.signExtendedByte)
        let a = Int32(bitPattern: regs[.EAX].signExtendedWord)
        if d == 0 {
            throw Interrupt(.DE)
        }
        let q = a / d
        if q.signExtendedByte != q {
            throw Interrupt(.DE)
        }
        let s = a % d
        regs[.EAX].lowerHalf = (q & 0xff) | (s << 8)
    }
    func aux16Idiv(_ divisor: DWord) throws {
        let d = Int32(bitPattern: divisor.signExtendedWord)
        let a = (regs[.EDX] << 16) | (regs[.EAX] & 0xffff)
        if d == 0 {
            throw Interrupt(.DE)
        }
        let q = a / d
        if q.signExtendedWord != q {
            throw Interrupt(.DE)
        }
        let s = a % d
        regs[.EAX].lowerHalf = q
        regs[.EDX].lowerHalf = s
    }
    func auxIdiv(_ dividend: QWord, _ divisor: DWord) {
        let ds: DWord, d: DWord
        let uh = DWord(truncatingIfNeeded: dividend >> 32)
        let lh = DWord(truncatingIfNeeded: dividend & 0xffffffff)
        if (uh & 0x80000000) != 0 {
            ds = 1
            uh = ~uh
            lh = ~lh &+ 1
            if lh == 0 {
                uh = uh &+ 1
            }
        } else {
            ds = 0
        }
        if (divisor & 0x80000000) != 0 {
            d = ~divisor &+ 1
            rs = 1
        } else {
            rs = 0
        }
        aux_DIV(lh | (QWord(uh) << 32), d)
        rs ^= ds
        if rs != 0 {
            if (u > 0x80000000) {
                abort(0)
            }
            u = ~u &+ 1
        } else {
            if (u >= 0x80000000) {
                abort(0)
            }
        }
        if (ds) {
            v = ~v &+ 1
        }
    }
    func aux8Mul(_ multiplicand: DWord, _ multiplier: DWord) {
    }
    func aux16Mul(_ multiplicand: DWord, _ multiplier: DWord) {
    }
    func auxMul(_ multiplicand: DWord, _ multiplier: DWord) {
    }
    func aux8Imul(_ multiplicand: DWord, _ multiplier: DWord) {
    }
    func aux16Imul(_ multiplicand: DWord, _ multiplier: DWord) {
    }
    func auxImul(_ multiplicand: DWord, _ multiplier: DWord) {
    }
    func multiply(_ multiplicand: DWord, _ multiplier: DWord) {
    }
    @discardableResult
    func calculate8(_ dst: DWord, _ src: DWord) -> DWord {
        0
    }
    @discardableResult
    func calculate16(_ dst: DWord, _ src: DWord) -> DWord {
        0
    }
    @discardableResult
    func calculate(_ dst: DWord, _ src: DWord) -> DWord {
        0
    }
}
