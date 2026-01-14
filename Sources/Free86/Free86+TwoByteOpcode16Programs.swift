/// two-byte 16 bit opcodes
extension Free86 {
    /// 0x180  JO
    /// 0x181  JNO
    /// 0x182  JB
    /// 0x183  JNB
    /// 0x184  JZ
    /// 0x185  JNZ
    /// 0x186  JBE
    /// 0x187  JNBE
    /// 0x188  JS
    /// 0x189  JNS
    /// 0x18a  JP
    /// 0x18b  JNP
    /// 0x18c  JL
    /// 0x18d  JNL
    /// 0x18e  JLE
    /// 0x18f  JNLE
    func tOx18f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x140  CMOVx (80486)
    /// 0x141  CMOVx (80486)
    /// 0x142  CMOVx (80486)
    /// 0x143  CMOVx (80486)
    /// 0x144  CMOVx (80486)
    /// 0x145  CMOVx (80486)
    /// 0x146  CMOVx (80486)
    /// 0x147  CMOVx (80486)
    /// 0x148  CMOVx (80486)
    /// 0x149  CMOVx (80486)
    /// 0x14a  CMOVx (80486)
    /// 0x14b  CMOVx (80486)
    /// 0x14c  CMOVx (80486)
    /// 0x14d  CMOVx (80486)
    /// 0x14e  CMOVx (80486)
    /// 0x14f  CMOVx (80486)
    func tOx14f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1b6  MOVZX
    func tOx1b6() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1be  MOVSX
    func tOx1be() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1af  IMUL
    func tOx1af() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1c1  XADD (80486)
    func tOx1c1() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1a0  PUSH FS
    /// 0x1a8  PUSH GS
    func tOx1a8() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1a1  POP FS
    /// 0x1a9  POP GS
    func tOx1a9() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1b2  LSS
    /// 0x1b4  LFS
    /// 0x1b5  LGS
    func tOx1b5() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1a4  SHLD
    /// 0x1ac  SHRD
    func tOx1ac() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1a5  SHLD
    /// 0x1ad  SHRD
    func tOx1ad() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1ba  G8 (-, -, -, -, BT, BTS, BTR, BTC)
    func tOx1ba() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1a3  BT
    func tOx1a3() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1ab  BTS
    /// 0x1b3  BTR
    /// 0x1bb  BTC
    func tOx1bb() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1bc  BSF
    /// 0x1bd  BSR
    func tOx1bd() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1b1  CMPXCHG (40486)
    func tOx1b1() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x100  G6 (SLDT, STR, LLDT, LTR, VERR, VERW, -)
    /// 0x101  G7 (SGDT, SIDT, LGDT, LIDT, SMSW, -, LMSW, -)
    /// 0x102  LAR
    /// 0x103  LSL
    /// 0x106  CLTS
    /// 0x120  MOV
    /// 0x122  MOV
    /// 0x123  MOV
    /// 0x131  -
    /// 0x190  SETO
    /// 0x191  SETNO
    /// 0x192  SETB
    /// 0x193  SETNB
    /// 0x194  SETZ
    /// 0x195  SETNZ
    /// 0x196  SETBE
    /// 0x197  SETNBE
    /// 0x198  SETS
    /// 0x199  SETNS
    /// 0x19a  SETP
    /// 0x19b  SETNP
    /// 0x19c  SETL
    /// 0x19d  SETNL
    /// 0x19e  SETLE
    /// 0x19f  SETNLE
    /// 0x1a2  -
    /// 0x1b0  CMPXCHG (80486)
    func tOx1b0() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x104  -
    /// 0x105  -
    /// 0x107  -
    /// 0x108  -
    /// 0x109  -
    /// 0x10a  -
    /// 0x10b  -
    /// 0x10c  -
    /// 0x10d  -
    /// 0x10e  -
    /// 0x10f  -
    /// 0x110  -
    /// 0x111  -
    /// 0x112  -
    /// 0x113  -
    /// 0x114  -
    /// 0x115  -
    /// 0x116  -
    /// 0x117  -
    /// 0x118  -
    /// 0x119  -
    /// 0x11a  -
    /// 0x11b  -
    /// 0x11c  -
    /// 0x11d  -
    /// 0x11e  -
    /// 0x11f  -
    /// 0x121  MOV
    /// 0x124  MOV
    /// 0x125  -
    /// 0x126  MOV
    /// 0x127  -
    /// 0x128  -
    /// 0x129  -
    /// 0x12a  -
    /// 0x12b  -
    /// 0x12c  -
    /// 0x12d  -
    /// 0x12e  -
    /// 0x12f  -
    /// 0x130  -
    /// 0x132  -
    /// 0x133  -
    /// 0x134  -
    /// 0x135  -
    /// 0x136  -
    /// 0x137  -
    /// 0x138  -
    /// 0x139  -
    /// 0x13a  -
    /// 0x13b  -
    /// 0x13c  -
    /// 0x13d  -
    /// 0x13e  -
    /// 0x13f  -
    /// 0x150  -
    /// 0x151  -
    /// 0x152  -
    /// 0x153  -
    /// 0x154  -
    /// 0x155  -
    /// 0x156  -
    /// 0x157  -
    /// 0x158  -
    /// 0x159  -
    /// 0x15a  -
    /// 0x15b  -
    /// 0x15c  -
    /// 0x15d  -
    /// 0x15e  -
    /// 0x15f  -
    /// 0x160  -
    /// 0x161  -
    /// 0x162  -
    /// 0x163  -
    /// 0x164  -
    /// 0x165  -
    /// 0x166  -
    /// 0x167  -
    /// 0x168  -
    /// 0x169  -
    /// 0x16a  -
    /// 0x16b  -
    /// 0x16c  -
    /// 0x16d  -
    /// 0x16e  -
    /// 0x16f  -
    /// 0x170  -
    /// 0x171  -
    /// 0x172  -
    /// 0x173  -
    /// 0x174  -
    /// 0x175  -
    /// 0x176  -
    /// 0x177  -
    /// 0x178  -
    /// 0x179  -
    /// 0x17a  -
    /// 0x17b  -
    /// 0x17c  -
    /// 0x17d  -
    /// 0x17e  -
    /// 0x17f  -
    /// 0x1a6  -
    /// 0x1a7  -
    /// 0x1aa  -
    /// 0x1ae  -
    /// 0x1b7  MOVZX
    /// 0x1b8  -
    /// 0x1b9  -
    /// 0x1bf  MOVSX
    /// 0x1c0  -
    func tOx1c0() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
}
