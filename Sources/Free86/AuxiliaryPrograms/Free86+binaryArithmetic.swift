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
        var uh = DWord(truncatingIfNeeded: dividend >> 32)
        var lh = DWord(truncatingIfNeeded: dividend & 0xffffffff)
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
        if ((q << 24) >> 24) != q {
            throw Interrupt(.DE)
        }
        let s = a % d
        regs[.EAX].lowerHalf = DWord((q & 0xff) | (s << 8))
    }
    func aux16Idiv(_ divisor: DWord) throws {
        let d = Int32(bitPattern: divisor.signExtendedWord)
        let a = Int32((regs[.EDX] << 16) | (regs[.EAX] & 0xffff))
        if d == 0 {
            throw Interrupt(.DE)
        }
        let q = a / d
        if ((q << 24) >> 24) != q {
            throw Interrupt(.DE)
        }
        let s = a % d
        regs[.EAX].lowerHalf = DWord(bitPattern: q)
        regs[.EDX].lowerHalf = DWord(bitPattern: s)
    }
    func auxIdiv(_ dividend: QWord, _ divisor: DWord) throws {
        let ds: DWord, d: DWord
        var rs: DWord
        var uh = DWord(truncatingIfNeeded: dividend >> 32)
        var lh = DWord(truncatingIfNeeded: dividend & 0xffffffff)
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
            d = divisor
            rs = 0
        }
        try auxDiv(QWord(lh) | (QWord(uh) << 32), d)
        rs ^= ds
        if rs != 0 {
            if u > 0x80000000 {
                throw Interrupt(.DE)
            }
            u = ~u &+ 1
        } else {
            if u >= 0x80000000 {
                throw Interrupt(.DE)
            }
        }
        if ds != 0 {
            v = ~v &+ 1
        }
    }
    func aux8Mul(_ multiplicand: DWord, _ multiplier: DWord) {
        u = (multiplicand & 0xff) &* (multiplier & 0xff)
        osmSrc = u >> 8
        osmDst = u.signExtendedByte
        osm = 21
    }
    func aux16Mul(_ multiplicand: DWord, _ multiplier: DWord) {
        u = (multiplicand & 0xffff) &* (multiplier & 0xffff)
        osmSrc = u >> 16
        osmDst = u.signExtendedWord
        osm = 22
    }
    func auxMul(_ multiplicand: DWord, _ multiplier: DWord) {
        multiply(multiplicand, multiplier)
        osmDst = u
        osmSrc = v
        osm = 23
    }
    func aux8Imul(_ multiplicand: DWord, _ multiplier: DWord) {
        let md = multiplicand.signExtendedByte
        let mr = multiplier.signExtendedByte
        u = md &* mr
        osmDst = u.signExtendedByte
        osmSrc = (u != osmDst) ? 1 : 0
        osm = 21
    }
    func aux16Imul(_ multiplicand: DWord, _ multiplier: DWord) {
        let md = multiplicand.signExtendedWord
        let mr = multiplier.signExtendedWord
        u = md &* mr
        osmDst = u.signExtendedWord
        osmSrc = (u != osmDst) ? 1 : 0
        osm = 22
    }
    func auxImul(_ multiplicand: DWord, _ multiplier: DWord) {
        var md = multiplicand
        var mr = multiplier
        var s: DWord = 0
        if (md & 0x80000000) != 0 {
            md = ~md &+ 1
            s = 1
        }
        if (mr & 0x80000000) != 0 {
            mr = ~mr &+ 1
            s ^= 1
        }
        multiply(md, mr)
        if s != 0 {
            v = ~v
            u = ~u &+ 1
            if u == 0 {
                v = v &+ 1
            }
    }
    osmDst = u
        osmSrc = v - u.signedShiftRight(count: 31)
    osm = 23
    }
    func multiply(_ multiplicand: DWord, _ multiplier: DWord) {
        var x = QWord(multiplicand) &* QWord(multiplier)
        if x <= 0xffffffff {
            v = 0
        } else {
            let dl = multiplicand & 0xffff
            let du = multiplicand >> 16
            let rl = multiplier & 0xffff
            let ru = multiplier >> 16
            x = QWord(dl) &* QWord(rl)
            v = du &* ru
            w = dl &* ru
            x = x &+ QWord((w & 0xffff) << 16)
            v = v &+ (w >> 16)
            if x >= 4294967296 {
                x -= 4294967296
                v = v &+ 1
            }
            w = du &* rl
            x = x &+ QWord((w & 0xffff) << 16)
            v = v &+ (w >> 16)
            if x >= 4294967296 {
                x -= 4294967296
                v = v &+ 1
            }
        }
        u = DWord(truncatingIfNeeded: x)
    }
    @discardableResult
    func calculate8(_ dst: DWord, _ src: DWord) -> DWord {
        let cf: DWord
        var res = dst
        switch operation & 7 {
        case 0:
            osmSrc = src
            res = (res &+ src).signExtendedByte
            osmDst = res
            osm = 0
            break
        case 1:
            res = (res | src).signExtendedByte
            osmDst = res
            osm = 12
            break
        case 2:
            cf = isCF() ? 1 : 0
            osmSrc = src
            res = (res &+ src &+ cf).signExtendedByte
            osmDst = res
            osm = cf != 0 ? 3 : 0
            break
        case 3:
            cf = isCF() ? 1 : 0
            osmSrc = src
            res = (res &- src &- cf).signExtendedByte
            osmDst = res
            osm = cf != 0 ? 9 : 6
            break
        case 4:
            res = (res & src).signExtendedByte
            osmDst = res
            osm = 12
            break
        case 5:
            osmSrc = src
            res = (res &- src).signExtendedByte
            osmDst = res
            osm = 6
            break
        case 6:
            res = (res ^ src).signExtendedByte
            osmDst = res
            osm = 12
            break
        case 7:
            osmSrc = src
            osmDst = (dst &- src).signExtendedByte
            osm = 6
            break
        default:
            break  // exhaustive
        }
        return res
    }
    @discardableResult
    func calculate16(_ dst: DWord, _ src: DWord) -> DWord {
        let cf: DWord
        var res = dst
        switch operation & 7 {
        case 0:
            osmSrc = src
            res = (res &+ src).signExtendedWord
            osmDst = res
            osm = 1
            break
        case 1:
            res = (res | src).signExtendedWord
            osmDst = res
            osm = 13
            break
        case 2:
            cf = isCF() ? 1 : 0
            osmSrc = src
            res = (res &+ src &+ cf).signExtendedWord
            osmDst = res
            osm = cf != 0 ? 4 : 1
            break
        case 3:
            cf = isCF() ? 1 : 0
            osmSrc = src
            res = (res &- src &- cf).signExtendedWord
            osmDst = res
            osm = cf != 0 ? 10 : 7
            break
        case 4:
            res = (res & src).signExtendedWord
            osmDst = res
            osm = 13
            break
        case 5:
            osmSrc = src
            res = (res &- src).signExtendedWord
            osmDst = res
            osm = 7
            break
        case 6:
            res = (res ^ src).signExtendedWord
            osmDst = res
            osm = 13
            break
        case 7:
            osmSrc = src
            osmDst = (res &- src).signExtendedWord
            osm = 7
            break
        default:
            break  // exhaustive
        }
        return res
    }
    @discardableResult
    func calculate(_ dst: DWord, _ src: DWord) -> DWord {
        let cf: DWord
        var res = dst
        switch operation & 7 {
        case 0:
            osmSrc = src
            res = res &+ src
            osmDst = res
            osm = 2
            break
        case 1:
            res = res | src
            osmDst = res
            osm = 14
            break
        case 2:
            cf = isCF() ? 1 : 0
            osmSrc = src
            res = res &+ src &+ cf
            osmDst = res
            osm = cf != 0 ? 5 : 2
            break
        case 3:
            cf = isCF() ? 1 : 0
            osmSrc = src
            res = res &- src &- cf
            osmDst = res
            osm = cf != 0 ? 11 : 8
            break
        case 4:
            res = res & src
            osmDst = res
            osm = 14
            break
        case 5:
            osmSrc = src
            res = res &- src
            osmDst = res
            osm = 8
            break
        case 6:
            res = res ^ src
            osmDst = res
            osm = 14
            break
        case 7:
            osmSrc = src
            osmDst = res &- src
            osm = 8
            break
        default:
            break  // exhaustive
        }
        return res
    }
}
