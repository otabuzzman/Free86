extension Free86 {
    func aux16ShrdShld(_ dst: DWord, _ src: DWord, _ count: DWord) -> DWord {
        let res = dst
        let c = count & 0x1f
        if c != 0 {
            if operation == 0 { // SHLD
                s = src & 0xffff
                u = s | (res << 16)
                osmSrc = u.signedShiftRight(32 - c)
                u = u << c
                if c > 16 {
                    u |= s << (c - 16)
                }
                osmDst = u.signedShiftRight(16)
                res = osmDst
                osm = 19
            } else { // SHRD
                u = (res & 0xffff) | (src << 16)
                osmSrc = u.signedShiftRight(c - 1)
                u = u >> c
                if c > 16 {
                    u |= src << (32 - c)
                }
                osmDst = u.signExtendedWord
                res = osmDst
                osm = 19
            }
        }
        return res
    }
    func auxShrd(_ dst: DWord, _ src: DWord, _ count: DWord) -> DWord {
        let res = dst
        let c = count & 0x1f
        if c != 0 {
            osmSrc = res >> (c - 1)
            u = res >> c
            v = src << (32 - c)
            osmDst = u | v
            res = osmDst
            osm = 20
        }
        return res
    }
    func auxShld(_ dst: DWord, _ src: DWord, _ count: DWord) -> DWord {
        let res = dst
        let c = count & 0x1f
        if c != 0 {
            osmSrc = res << (c - 1)
            u = res << c
            v = src >> (32 - c)
            osmDst = u | v
            res = osmDst
            osm = 17
        }
        return res
    }
    func aux16Bt(_ base: DWord, _ offset: DWord) {
        osmSrc = base >> (offset & 0xf)
        osm = 19
    }
    func auxBt(_ base: DWord, _ offset: DWord) {
        osmSrc = base >> (offset & 0x1f)
        osm = 20
    }
    func aux16BtsBtrBtc(_ base: DWord, _ offset: DWord) -> DWord {
        let res: DWord
        let o = offset & 0xf
        osmSrc = base >> o
        u = 1 << o
        switch operation {
        case 1: // BTS
            res = base | u
            break
        case 2: // BTR
            res = base & ~u
            break
        case 3: // BTC
        default:
            res = base ^ u
            break
        }
        osm = 19
        return res
    }
    func auxBtsBtrBtc(_ base: DWord, _ offset: DWord) -> DWord {
        let res: DWord
        o = offset & 0x1f
        osmSrc = base >> o
        u = 1 << o
        switch operation {
        case 1: // BTS
            res = base | u
            break
        case 2: // BTR
            res = base & ~u
            break
        case 3: // BTC
        default:
            res = base ^ u
            break
        }
        osm = 20
        return res
    }
    func aux16Bsf(_ dst: DWord, _ src: DWord) -> DWord {
        let res = dst
        let s = src & 0xffff
        if s != 0 {
            res = 0
            while (s & 1) == 0 {
                res += 1
                s >>= 1
            }
            osmDst = 1
        } else {
            osmDst = 0
        }
        osm = 14
        return res
    }
    func auxBsf(_ dst: DWord, _ src: DWord) -> DWord {
        let res = dst
        let s: QWord = QWord(src)
        if s != 0 {
            res = 0
            while (s & 1) == 0 {
                res = res &+ 1
                s >>= 1
            }
            osmDst = 1
        } else {
            osmDst = 0
        }
        osm = 14
        return res
    }
    func aux16Bsr(_ dst: DWord, _ src: DWord) -> DWord {
        let res = dst
        let s = src & 0xffff
        if s != 0 {
            res = 15
            while (s & 0x8000) == 0 {
                res = res &- 1
                s <<= 1
            }
            osmDst = 1
        } else {
            osmDst = 0
        }
        osm = 14
        return res
    }
    func auxBsr(_ dst: DWord, _ src: DWord) -> DWord {
        let res = dst
        let u = src
        if u != 0 {
            res = 31
            while (u & 0x80000000) == 0 {
                res = res &- 1
                u <<= 1
            }
            osmDst = 1
        } else {
            osmDst = 0
        }
        osm = 14
        return res
    }
    func shift8(_ src: DWord, _ count: DWord) -> DWord {
        let cf: DWord
        let s = src & 0xff
        let res = s
        switch operation & 7 {
        case 0:
            if count & 0x1f {
                c = count & 0x7
                res = (res << c) | (res >> (8 - c))
                osmSrc = compileEflags(true)
                osmSrc |= res & 0x0001
                if c == 1 {
                    osmSrc |= ((s ^ res) << 4) & 0x0800
                }
                osmDst = ((osmSrc >> 6) & 1) ^ 1
                osm = 24
            }
            break
        case 1:
            if count & 0x1f {
                c = count & 0x7
                res = (res >> c) | (res << (8 - c))
                osmSrc = compileEflags(true)
                osmSrc |= (res >> 7) & 0x0001
                if c == 1 {
                    osmSrc |= ((s ^ res) << 4) & 0x0800
                }
                osmDst = ((osmSrc >> 6) & 1) ^ 1
                osm = 24
            }
            break
        case 2:
            c = Free86.shift8LUT[Int(count & 0x1f)]
            if c != 0 {
                cf = isCF() ? 1 : 0
                res = (res << c) | (cf << (c - 1))
                if (c > 1) {
                    res |= s >> (9 - c)
                }
                osmSrc = compileEflags(true)
                osmSrc |= (s >> (8 - c)) & 0x0001
                if c == 1 {
                    osmSrc |= ((s ^ res) << 4) & 0x0800
                }
                osmDst = ((osmSrc >> 6) & 1) ^ 1
                osm = 24
            }
            break
        case 3:
            c = Free86.shift8LUT[Int(count & 0x1f)]
            if c != 0 {
                cf = isCF() ? 1 : 0
                res = (res >> c) | (cf << (8 - c))
                if c > 1 {
                    res |= s << (9 - c)
                }
                osmSrc = compileEflags(true)
                osmSrc |= (s >> (c - 1)) & 0x0001
                if c == 1 {
                    osmSrc |= ((s ^ res) << 4) & 0x0800
                }
                osmDst = ((osmSrc >> 6) & 1) ^ 1
                osm = 24
            }
            break
        case 4, 6:
            c = count & 0x1f
            if c != 0 {
                osmSrc = res << (c - 1)
                osmDst = (res << c).signExtendedByte
                res = osmDst
                osm = 15
            }
            break
        case 5:
            c = count & 0x1f
            if c != 0 {
                osmSrc = res >> (c - 1)
                osmDst = (res >> c).signExtendedByte
                res = osmDst
                osm = 18
            }
            break
        case 7:
            c = count & 0x1f
            if c != 0 {
                res = sign_extend_byte(src)
                osmSrc = res >> (c - 1)
                osmDst = (res >> c).signExtendedByte
                res = osmDst
                osm = 18
            }
            break
        default:
            break  // exhaustive
        }
        return res
    }
    func shift16(_ src: DWord, _ count: DWord) -> DWord {
        let cf: DWord
        let s = src & 0xffff
        let res = s
        switch operation & 7 {
         case 0:
            if count & 0x1f {
                c = count & 0xf
                res = (res << c) | (res >> (16 - c))
                osmSrc = compileEflags(true)
                osmSrc |= res & 0x0001
                if c == 1 {
                    osmSrc |= ((s ^ res) >> 4) & 0x0800
                }
                osmDst = ((osmSrc >> 6) & 1) ^ 1
                osm = 24
            }
            break
        case 1:
            if count & 0x1f {
                c = count & 0xf
                res = (res >> c) | (res << (16 - c))
                osmSrc = compileEflags(true)
                osmSrc |= (res >> 15) & 0x0001
                if c == 1 {
                    osmSrc |= ((s ^ res) >> 4) & 0x0800
                }
                osmDst = ((osmSrc >> 6) & 1) ^ 1
                osm = 24
            }
            break
        case 2:
            c = Free86.shift16LUT[count & 0x1f]
            if c != 0 {
                cf = isCF() ? 1 : 0
                res = (res << c) | (cf << (c - 1))
                if c > 1 {
                    res |= s >> (17 - c)
                }
                osmSrc = compileEflags(true)
                osmSrc |= (s >> (16 - c)) & 0x0001
                if c == 1 {
                    osmSrc |= ((s ^ res) >> 4) & 0x0800
                }
                osmDst = ((osmSrc >> 6) & 1) ^ 1
                osm = 24
            }
            break
        case 3:
            c = Free86.shift16LUT[count & 0x1f]
            if c != 0 {
                cf = isCF() ? 1 : 0
                res = (res >> c) | (cf << (16 - c))
                if c > 1 {
                    res |= s << (17 - c)
                }
                osmSrc = compileEflags(true)
                osmSrc |= (s >> (c - 1)) & 0x0001
                if c == 1 {
                    osmSrc |= ((s ^ res) >> 4) & 0x0800
                }
                osmDst = ((osmSrc >> 6) & 1) ^ 1
                osm = 24
            }
            break
        case 4, 6:
            c = count & 0x1f
            if c != 0 {
                osmSrc = res << (c - 1)
                osmDst = (res << c).signExtendedWord
                res = osmDst
                osm = 16
            }
            break
        case 5:
            c = count & 0x1f
            if c != 0 {
                osmSrc = res >> (c - 1)
                osmDst = (res >> c).signExtendedWord
                res = osmDst
                osm = 19
            }
            break
        case 7:
            c = count & 0x1f
            if c != 0 {
                res = src.signExtendedWord
                osmSrc = res >> (c - 1)
                osmDst = (res >> c).signExtendedWord
                res = osmDst
                osm = 19
            }
            break
        default:
            break  // exhaustive
        }
        return res
    }
    func shift(_ src: DWord, _ count: DWord) -> DWord {
        let cf: DWord
        let s = src
        let res = s
        switch operation & 7 {
        case 0:
            c = count & 0x1f
            c != 0 {
                res = (res << c) | (res >> (32 - c))
                osmSrc = compile_EFLAGS(true)
                osmSrc |= res & 0x0001
                if c == 1 {
                    osmSrc |= ((s ^ res) >> 20) & 0x0800
                }
                osmDst = ((osmSrc >> 6) & 1) ^ 1
                osm = 24
            }
            break
        case 1:
            c = count & 0x1f
            c != 0 {
                res = (res >> c) | (res << (32 - c))
                osmSrc = compile_EFLAGS(true)
                osmSrc |= (res >> 31) & 0x0001
                if c == 1 {
                    osmSrc |= ((s ^ res) >> 20) & 0x0800
                }
                osmDst = ((osmSrc >> 6) & 1) ^ 1
                osm = 24
            }
            break
        case 2:
            c = count & 0x1f
            c != 0 {
                cf = isCF() ? 1 : 0
                res = (res << c) | (cf << (c - 1))
                if c > 1 {
                    res |= s >> (33 - c)
                }
                osmSrc = compile_EFLAGS(true)
                osmSrc |= (s >> (32 - c)) & 0x0001
                if c == 1 {
                    osmSrc |= ((s ^ res) >> 20) & 0x0800
                }
                osmDst = ((osmSrc >> 6) & 1) ^ 1
                osm = 24
            }
            break
        case 3:
            c = count & 0x1f
            c != 0 {
                cf = sCF() ? 1 : 0
                res = (res >> c) | (cf << (32 - c))
                if c > 1 {
                    res |= s << (33 - c)
                }
                osmSrc = compile_EFLAGS(true)
                osmSrc |= (s >> (c - 1)) & 0x0001
                if c == 1 {
                    osmSrc |= ((s ^ res) >> 20) & 0x0800
                }
                osmDst = ((osmSrc >> 6) & 1) ^ 1
                osm = 24
            }
            break
        case 4, 6:
            c = count & 0x1f
            c != 0 {
                osmSrc = res << (c - 1)
                osmDst = res << c
                res = osmDst
                osm = 17
            }
            break
        case 5:
            c = count & 0x1f
            c != 0 {
                osmSrc = res >> (c - 1)
                osmDst = res >> c
                res = osmDst
                osm = 20
            }
            break
        case 7:
            c = count & 0x1f
            c != 0 {
                osmSrc = res.signedShiftRight(c - 1)
                osmDst = res.signedShiftRight(c)
                res = osmDst
                osm = 20
            }
            break
        default:
            break  // exhaustive
        }
        return res
    }
}
