/// two-byte opcode programs
extension Free86 {
    /// 0x80  JO
    /// 0x81  JNO
    /// 0x82  JB
    /// 0x83  JNB
    /// 0x84  JZ
    /// 0x85  JNZ
    /// 0x86  JBE
    /// 0x87  JNBE
    /// 0x88  JS
    /// 0x89  JNS
    /// 0x8a  JP
    /// 0x8b  JNP
    /// 0x8c  JL
    /// 0x8d  JNL
    /// 0x8e  JLE
    /// 0x8f  JNLE
    func Ox0f8f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x90  SETO
    /// 0x91  SETNO
    /// 0x92  SETB
    /// 0x93  SETNB
    /// 0x94  SETZ
    /// 0x95  SETNZ
    /// 0x96  SETBE
    /// 0x97  SETNBE
    /// 0x98  SETS
    /// 0x99  SETNS
    /// 0x9a  SETP
    /// 0x9b  SETNP
    /// 0x9c  SETL
    /// 0x9d  SETNL
    /// 0x9e  SETLE
    /// 0x9f  SETNLE
    func Ox0f9f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x40  CMOVx conditional move (80486) - overflow (OF == 1)
    /// 0x41  CMOVx   - not overflow (OF == 0)
    /// 0x42  CMOVx   - below/not above or equal/carry (CF == 1)
    /// 0x43  CMOVx   - not below/above or equal/not carry (CF == 0)
    /// 0x44  CMOVx   - zero/equal (ZF == 1)
    /// 0x45  CMOVx   - not zero/not equal (ZF == 0)
    /// 0x46  CMOVx   - below or equal/not above (CF == 1 OR ZF == 1)
    /// 0x47  CMOVx   - not below or equal/above (CF == 0 AND ZF == 0)
    /// 0x48  CMOVx   - sign (SF == 1)
    /// 0x49  CMOVx   - not sign (SF == 0)
    /// 0x4a  CMOVx   - parity/parity even (PF == 1)
    /// 0x4b  CMOVx   - not parity/parity odd (PF == 0)
    /// 0x4c  CMOVx   - less/not greater (SF != OF)
    /// 0x4d  CMOVx   - not less/greater or equal (SF == OF)
    /// 0x4e  CMOVx   - less or equal/not greater ((ZF == 1) OR (SF != OF))
    /// 0x4f  CMOVx   - not less nor equal/greater ((ZF == 0) AND (SF == OF))
    func Ox0f4f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xb6  MOVZX
    func Ox0fb6() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xb7  MOVZX
    func Ox0fb7() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xbe  MOVSX
    func Ox0fbe() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xbf  MOVSX
    func Ox0fbf() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x00  G6 (SLDT, STR, LLDT, LTR, VERR, VERW, -)
    func Ox0f00() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x01  G7 (SGDT, SIDT, LGDT, LIDT, SMSW, -, LMSW, -)
    func Ox0f01() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x02  LAR
    /// 0x03  LSL
    func Ox0f03() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x20  MOV
    func Ox0f20() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x22  MOV
    func Ox0f22() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x06  CLTS
    func Ox0f06() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x23  MOV
    func Ox0f23() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xb2  LSS
    /// 0xb4  LFS
    /// 0xb5  LGS
    func Ox0fb5() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa2  CPUID (80486)
    func Ox0fa2() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa4  SHLD
    func Ox0fa4() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa5  SHLD
    func Ox0fa5() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xac  SHRD
    func Ox0fac() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xad  SHRD
    func Ox0fad() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xba  G8 (-, -, -, -, BT, BTS, BTR, BTC)
    func Ox0fba() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa3  BT
    func Ox0fa3() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xab  BTS
    /// 0xb3  BTR
    /// 0xbb  BTC
    func Ox0fbb() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xbc  BSF
    /// 0xbd  BSR
    func Ox0fbd() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xaf  IMUL
    func Ox0faf() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x31  RDTSC (80486)
    func Ox0f31() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc0  XADD (80486)
    func Ox0fc0() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc1  XADD (80486)
    func Ox0fc1() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xb0  CMPXCHG (80486)
    func Ox0fb0() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xb1  CMPXCHG (80486)
    func Ox0fb1() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa0  PUSH FS
    /// 0xa8  PUSH GS
    func Ox0fa8() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa1  POP FS
    /// 0xa9  POP GS
    func Ox0fa9() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc8  -
    /// 0xc9  -
    /// 0xca  -
    /// 0xcb  -
    /// 0xcc  -
    /// 0xcd  -
    /// 0xce  -
    /// 0xcf  BSWAP (80486)
    func Ox0fcf() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x04  -
    /// 0x05  -
    /// 0x07  -
    /// 0x08  -
    /// 0x09  -
    /// 0x0a  -
    /// 0x0b  -
    /// 0x0c  -
    /// 0x0d  -
    /// 0x0e  -
    /// 0x0f  -
    /// 0x10  -
    /// 0x11  -
    /// 0x12  -
    /// 0x13  -
    /// 0x14  -
    /// 0x15  -
    /// 0x16  -
    /// 0x17  -
    /// 0x18  -
    /// 0x19  -
    /// 0x1a  -
    /// 0x1b  -
    /// 0x1c  -
    /// 0x1d  -
    /// 0x1e  -
    /// 0x1f  -
    /// 0x21  MOV
    /// 0x24  MOV
    /// 0x25  -
    /// 0x26  MOV
    /// 0x27  -
    /// 0x28  -
    /// 0x29  -
    /// 0x2a  -
    /// 0x2b  -
    /// 0x2c  -
    /// 0x2d  -
    /// 0x2e  -
    /// 0x2f  -
    /// 0x30  -
    /// 0x32  -
    /// 0x33  -
    /// 0x34  -
    /// 0x35  -
    /// 0x36  -
    /// 0x37  -
    /// 0x38  -
    /// 0x39  -
    /// 0x3a  -
    /// 0x3b  -
    /// 0x3c  -
    /// 0x3d  -
    /// 0x3e  -
    /// 0x3f  -
    /// 0x50  -
    /// 0x51  -
    /// 0x52  -
    /// 0x53  -
    /// 0x54  -
    /// 0x55  -
    /// 0x56  -
    /// 0x57  -
    /// 0x58  -
    /// 0x59  -
    /// 0x5a  -
    /// 0x5b  -
    /// 0x5c  -
    /// 0x5d  -
    /// 0x5e  -
    /// 0x5f  -
    /// 0x60  -
    /// 0x61  -
    /// 0x62  -
    /// 0x63  -
    /// 0x64  -
    /// 0x65  -
    /// 0x66  -
    /// 0x67  -
    /// 0x68  -
    /// 0x69  -
    /// 0x6a  -
    /// 0x6b  -
    /// 0x6c  -
    /// 0x6d  -
    /// 0x6e  -
    /// 0x6f  -
    /// 0x70  -
    /// 0x71  -
    /// 0x72  -
    /// 0x73  -
    /// 0x74  -
    /// 0x75  -
    /// 0x76  -
    /// 0x77  -
    /// 0x78  -
    /// 0x79  -
    /// 0x7a  -
    /// 0x7b  -
    /// 0x7c  -
    /// 0x7d  -
    /// 0x7e  -
    /// 0x7f  -
    /// 0xa6  -
    /// 0xa7  -
    /// 0xaa  -
    /// 0xae  -
    /// 0xb8  -
    /// 0xb9  -
    /// 0xc2  -
    /// 0xc3  -
    /// 0xc4  -
    /// 0xc5  -
    /// 0xc6  -
    /// 0xc7  -
    /// 0xd0  -
    /// 0xd1  -
    /// 0xd2  -
    /// 0xd3  -
    /// 0xd4  -
    /// 0xd5  -
    /// 0xd6  -
    /// 0xd7  -
    /// 0xd8  -
    /// 0xd9  -
    /// 0xda  -
    /// 0xdb  -
    /// 0xdc  -
    /// 0xdd  -
    /// 0xde  -
    /// 0xdf  -
    /// 0xe0  -
    /// 0xe1  -
    /// 0xe2  -
    /// 0xe3  -
    /// 0xe4  -
    /// 0xe5  -
    /// 0xe6  -
    /// 0xe7  -
    /// 0xe8  -
    /// 0xe9  -
    /// 0xea  -
    /// 0xeb  -
    /// 0xec  -
    /// 0xed  -
    /// 0xee  -
    /// 0xef  -
    /// 0xf0  -
    /// 0xf1  -
    /// 0xf2  -
    /// 0xf3  -
    /// 0xf4  -
    /// 0xf5  -
    /// 0xf6  -
    /// 0xf7  -
    /// 0xf8  -
    /// 0xf9  -
    /// 0xfa  -
    /// 0xfb  -
    /// 0xfc  -
    /// 0xfd  -
    /// 0xfe  -
    /// 0xff  -
    func Ox0fff() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
}
