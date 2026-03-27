extension Free86 {
    func isCF() -> Bool {  // carry (bit 0)
        let f: Bool
        let osm: Int
        let osmDst: DWord
        if self.osm >= 25 {
            osm = osmPreserved
            osmDst = osmDstPreserved
        } else {
            osm = self.osm
            osmDst = self.osmDst
        }
        switch osm % 25 {
        case 0:
            f = (osmDst & 0xff) < (osmSrc & 0xff)
            break
        case 1:
            f = (osmDst & 0xffff) < (osmSrc & 0xffff)
            break
        case 2:
            f = osmDst < osmSrc
            break
        case 3:
            f = (osmDst & 0xff) <= (osmSrc & 0xff)
            break
        case 4:
            f = (osmDst & 0xffff) <= (osmSrc & 0xffff)
            break
        case 5:
            f = osmDst <= osmSrc
            break
        case 6:
            f = ((osmDst &+ osmSrc) & 0xff) < (osmSrc & 0xff)
            break
        case 7:
            f = ((osmDst &+ osmSrc) & 0xffff) < (osmSrc & 0xffff)
            break
        case 8:
            f = (osmDst &+ osmSrc) < osmSrc
            break
        case 9:
            f = ((osmDst &+ osmSrc &+ 1) & 0xff) <= (osmSrc & 0xff)
            break
        case 10:
            f = ((osmDst &+ osmSrc &+ 1) & 0xffff) <= (osmSrc & 0xffff)
            break
        case 11:
            f = ((osmDst &+ osmSrc &+ 1) <= osmSrc)
            break
        case 12, 13, 14:
            f = false
            break
        case 15:
            f = ((osmSrc >> 7) & 1) != 0
            break
        case 16:
            f = ((osmSrc >> 15) & 1) != 0
            break
        case 17:
            f = ((osmSrc >> 31) & 1) != 0
            break
        case 18, 19, 20:
            f = (osmSrc & 1) != 0
            break
        case 21, 22, 23:
            f = osmSrc != 0
            break
        case 24:
            f = (osmSrc & 1) != 0
            break
        default:
            f = false
            break
        }
        return f
    }
    func isPF() -> Bool {  // parity (bit 2)
        if osm == 24 {
            return ((osmSrc >> 2) & 1) != 0
        } else {
            return Free86.parityLUT[osmDst & 0xff] != 0
        }
    }
    func isAF() -> Bool {  // adjust (bit 4)
        let f: Bool
        switch osm % 0x1f {
        case 0, 1, 2:
            u = osmDst &- osmSrc
            f = ((osmDst ^ u ^ osmSrc) & 0x10) != 0
            break
        case 3, 4, 5:
            u = osmDst &- osmSrc &- 1
            f = ((osmDst ^ u ^ osmSrc) & 0x10) != 0
            break
        case 6, 7, 8:
            u = osmDst &+ osmSrc
            f = ((osmDst ^ u ^ osmSrc) & 0x10) != 0
            break
        case 9, 10, 11:
            u = osmDst &+ osmSrc &+ 1
            f = ((osmDst ^ u ^ osmSrc) & 0x10) != 0
            break
        case 12, 13, 14:
            f = false 
            break
        case 15, 16, 17, 18, 19, 20, 21, 22, 23:
            f = false
            break
        case 24:
            f = (osmSrc & 0x10) != 0
            break
        case 25, 26, 27:
            f = ((osmDst ^ (osmDst &- 1)) & 0x10) != 0
            break
        case 28, 29, 30:
            f = ((osmDst ^ (osmDst &+ 1)) & 0x10) != 0
            break
        default:
            f = false
            break
        }
        return f
    }
    func isOF() -> Bool {  // overflow (bit 11)
        let f: Bool
        switch osm % 0x1f {
        case 0:
            u = osmDst &- osmSrc
            f = ((((u ^ osmSrc ^ 0xffffffff) & (u ^ osmDst)) >> 7) & 1) != 0
            break
        case 1:
            u = osmDst &- osmSrc
            f = ((((u ^ osmSrc ^ 0xffffffff) & (u ^ osmDst)) >> 15) & 1) != 0
            break
        case 2:
            u = osmDst &- osmSrc
            f = ((((u ^ osmSrc ^ 0xffffffff) & (u ^ osmDst)) >> 31) & 1) != 0
            break
        case 3:
            u = osmDst &- osmSrc &- 1
            f = ((((u ^ osmSrc ^ 0xffffffff) & (u ^ osmDst)) >> 7) & 1) != 0
            break
        case 4:
            u = osmDst &- osmSrc &- 1
            f = ((((u ^ osmSrc ^ 0xffffffff) & (u ^ osmDst)) >> 15) & 1) != 0
            break
        case 5:
            u = osmDst &- osmSrc &- 1
            f = ((((u ^ osmSrc ^ 0xffffffff) & (u ^ osmDst)) >> 31) & 1) != 0
            break
        case 6:
            u = osmDst &+ osmSrc
            f = ((((u ^ osmSrc) & (u ^ osmDst)) >> 7) & 1) != 0
            break
        case 7:
            u = osmDst &+ osmSrc
            f = ((((u ^ osmSrc) & (u ^ osmDst)) >> 15) & 1) != 0
            break
        case 8:
            u = osmDst &+ osmSrc
            f = ((((u ^ osmSrc) & (u ^ osmDst)) >> 31) & 1) != 0
            break
        case 9:
            u = osmDst &+ osmSrc &+ 1
            f = ((((u ^ osmSrc) & (u ^ osmDst)) >> 7) & 1) != 0
            break
        case 10:
            u = osmDst &+ osmSrc &+ 1
            f = ((((u ^ osmSrc) & (u ^ osmDst)) >> 15) & 1) != 0
            break
        case 11:
            u = osmDst &+ osmSrc &+ 1
            f = ((((u ^ osmSrc) & (u ^ osmDst)) >> 31) & 1) != 0
            break
        case 12, 13, 14:
            f = false
            break
        case 15, 18:
            f = (((osmSrc ^ osmDst) >> 7) & 1) != 0
            break
        case 16, 19:
            f = (((osmSrc ^ osmDst) >> 15) & 1) != 0
            break
        case 17, 20:
            f = (((osmSrc ^ osmDst) >> 31) & 1) != 0
            break
        case 21, 22, 23:
            f = osmSrc != 0
            break
        case 24:
            f = ((osmSrc >> 11) & 1) != 0
            break
        case 25:
            f = (osmDst & 0xff) == 0x80
            break
        case 26:
            f = (osmDst & 0xffff) == 0x8000
            break
        case 27:
            f = osmDst == 0x80000000
            break
        case 28:
            f = (osmDst & 0xff) == 0x7f
            break
        case 29:
            f = (osmDst & 0xffff) == 0x7fff
            break
        case 30:
            f = osmDst == 0x7fffffff
            break
        default:
            f = false
            break
        }
        return f
    }
    func isBE() -> Bool {  // `below' for signed comparison, PM p. 317
        let f: Bool
        switch osm {
        case 6:
            f = ((osmDst &+ osmSrc) & 0xff) <= (osmSrc & 0xff)
            break
        case 7:
            f = ((osmDst &+ osmSrc) & 0xffff) <= (osmSrc & 0xffff)
            break
        case 8:
            f = (osmDst &+ osmSrc) <= osmSrc
            break
        case 24:
            f = (osmSrc & (0x0040 | 0x0001)) != 0
            break
        default:
            f = isCF() || (osmDst == 0)
            break
        }
        return f
    }
    func isLE() -> Bool {  // `less' for unsigned comparison, PM p. 317
        let osmDst = Int32(bitPattern: self.osmDst)  // signed arithmetic easier here
        let osmSrc = Int32(bitPattern: self.osmSrc)
        let f: Bool
        switch osm {
        case 6:
            f = ((osmDst + osmSrc) << 24) <= (osmSrc << 24)
            break
        case 7:
            f = ((osmDst + osmSrc) << 16) <= (osmSrc << 16)
            break
        case 8:
            f = (osmDst + osmSrc) <= osmSrc
            break
        case 12, 13, 14, 25, 26, 27, 28, 29, 30:
            f = osmDst <= 0
            break
        case 24:
            f = ((((osmSrc >> 7) ^ (osmSrc >> 11)) | (osmSrc >> 6)) & 1) != 0
            break
        default:
            f = (((osmDst < 0 ? 1 : 0) ^ (isOF() ? 1 : 0)) | (osmDst == 0 ? 1 : 0)) != 0
            break
        }
        return f
    }
    func isLT() -> Bool {  // less than
        let osmDst = Int32(bitPattern: self.osmDst)  // signed arithmetic easier here
        let osmSrc = Int32(bitPattern: self.osmSrc)
        let f: Bool
        switch osm {
        case 6:
            f = ((osmDst + osmSrc) << 24) < (osmSrc << 24)
            break
        case 7:
            f = ((osmDst + osmSrc) << 16) < (osmSrc << 16)
            break
        case 8:
            f = (osmDst + osmSrc) < osmSrc
            break
        case 12, 13, 14, 25, 26, 27, 28, 29, 30:
            f = osmDst < 0
            break
        case 24:
            f = (((osmSrc >> 7) ^ (osmSrc >> 11)) & 1) != 0
            break
        default:
            f = ((osmDst < 0 ? 1 : 0) ^ (isOF() ? 1 :0)) != 0
            break
        }
        return f
    }
    func canJmp(condition: Int) -> Bool {
        let f: Bool
        switch (condition >> 1) & 7 {
        case 0:
            f = isOF()
            break
        case 1:
            f = isCF()
            break
        case 2:
            f = osmDst == 0
            break
        case 3:
            f = isBE()
            break
        case 4:
            f = (osm == 24 ? ((osmSrc >> 7) & 1) : (osmDst & 0x80000000)) != 0
            break
        case 5:
            f = isPF()
            break
        case 6:
            f = isLT()
            break
        case 7:
            f = isLE()
            break
        default:
            f = false
            break
        }
        return ((f ? 1 : 0) ^ (condition & 1)) != 0
    }
    func compileEflags(_ shift: Bool = false) -> DWord {
        var f0: DWord = 0, f11: DWord = 0
        if !shift {
            f0 = (isCF() ? 1 : 0) << 0
            f11 = (isOF() ? 1 : 0) << 11
        }
        let f2: DWord = (isPF() ? 1 : 0) << 2
        let f4: DWord = (isAF() ? 1 : 0) << 4
        let f6: DWord = (osmDst == 0 ? 1 : 0) << 6
        let f7: DWord = (osm == 24 ? ((osmSrc >> 7) & 1) : (osmDst >> 31) & 1) << 7
        return f0 | f2 | f4 | f6 | f7 | f11
    }
    func getEflags() -> DWord {
        var bits = compileEflags()
        bits |= DWord(bitPattern: df) & Eflags.flagMask(for: .DF)
        bits |= eflags
        return bits
    }
    func setEflags(_ bits: DWord, _ mask: DWord) {
        var _mask: DWord = 0
        _mask.setFlag(.OF)
        _mask.setFlag(.SF)
        _mask.setFlag(.ZF)
        _mask.setFlag(.AF)
        _mask.setFlag(.PF)
        _mask.setFlag(.CF)
        osmSrc = bits & _mask
        osmDst = ((osmSrc >> 6) & 1) ^ 1
        osm = 24
        df = 1 - Int32(2 * ((bits >> 10) & 1))
        eflags = (eflags & ~mask) | (bits & mask)
    }
}
