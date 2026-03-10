/// two-byte opcode bytes count programs
extension Instruction {
    /// 0x00  G6 (SLDT, STR, LLDT, LTR, VERR, VERW, -)
    /// 0x01  G7 (SGDT, SIDT, LGDT, LIDT, SMSW, -, LMSW, -)
    /// 0x02  LAR
    /// 0x03  LSL
    /// 0x20  MOV
    /// 0x22  MOV
    /// 0x23  MOV
    /// 0x40  -
    /// 0x41  -
    /// 0x42  -
    /// 0x43  -
    /// 0x44  -
    /// 0x45  -
    /// 0x46  -
    /// 0x47  -
    /// 0x48  -
    /// 0x49  -
    /// 0x4a  -
    /// 0x4b  -
    /// 0x4c  -
    /// 0x4d  -
    /// 0x4e  -
    /// 0x4f  -
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
    /// 0xa3  BT
    /// 0xa5  SHLD
    /// 0xab  BTS
    /// 0xad  SHRD
    /// 0xaf  IMUL
    /// 0xb0  CMPXCHG (80486)
    /// 0xb1  CMPXCHG (80486)
    /// 0xb2  LSS
    /// 0xb3  BTR
    /// 0xb4  LFS
    /// 0xb5  LGS
    /// 0xb6  MOVZX
    /// 0xb7  MOVZX
    /// 0xbb  BTC
    /// 0xbc  BSF
    /// 0xbd  BSR
    /// 0xbe  MOVSX
    /// 0xbf  MOVSX
    /// 0xc0  XADD (80486)
    /// 0xc1  XADD (80486)
    func Ox0fc1() throws -> Result<Resume, Never> {
        if (_length + 1) > 15 {
            throw Interrupt(.GP, errorCode: 0)
        }
        parent.lax = parent.eipLinear &+ _length
        _length += 1
        let modRM = try parent.ld8ReadonlyCpl3()
        _length += try modRMBytesNumber(modRM)
        if _length > 15 {
            throw Interrupt(.GP, errorCode: 0)
        }
        return .success(.endFetchLoop)
    }
    /// 0x06  CLTS
    /// 0x31  -
    /// 0xa0  PUSH FS
    /// 0xa1  POP FS
    /// 0xa2  -
    /// 0xa8  PUSH GS
    /// 0xa9  POP GS
    /// 0xc8  -
    /// 0xc9  -
    /// 0xca  -
    /// 0xcb  -
    /// 0xcc  -
    /// 0xcd  -
    /// 0xce  -
    /// 0xcf  -
    func Ox0fcf() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
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
        if _length > 15 {
            throw Interrupt(.GP, errorCode: 0)
        }
        return .success(.endFetchLoop)
    }
    /// 0xa4  SHLD
    /// 0xac  SHRD
    /// 0xba  G8 (-, -, -, -, BT, BTS, BTR, BTC)
    func Ox0fba() throws -> Result<Resume, Never> {
        if (_length + 1) > 15 {
            throw Interrupt(.GP, errorCode: 0)
        }
        parent.lax = parent.eipLinear &+ _length
        _length += 1
        let modRM = try parent.ld8ReadonlyCpl3()
        _length += try modRMBytesNumber(modRM)
        if _length > 15 {
            throw Interrupt(.GP, errorCode: 0)
        }
        _length += 1
        if _length > 15 {
            throw Interrupt(.GP, errorCode: 0)
        }
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
    func Ox0fc7() throws -> Result<Resume, Never> {
        throw Interrupt(.UD)
    }
}
