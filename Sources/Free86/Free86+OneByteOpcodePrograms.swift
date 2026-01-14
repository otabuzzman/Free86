/// one-byte opcodes
extension Free86 {
    /// 0x26  ES segment override prefix
    /// 0x2e  CS segment override prefix
    /// 0x36  SS segment override prefix
    /// 0x3e  DS segment override prefix
    func Ox3e() throws -> Result<Resume, Never> {
        return .success(.goOnFetching)
    }
    /// 0x64  FS segment override prefix
    /// 0x65  GS segment override prefix
    func Ox65() throws -> Result<Resume, Never> {
        return .success(.goOnFetching)
    }
    /// 0xf0  LOCK prefix
    func Oxf0() throws -> Result<Resume, Never> {
        return .success(.goOnFetching)
    }
    /// 0xf2  REPN[EZ] repeat string operation prefix
    func Oxf2() throws -> Result<Resume, Never> {
        return .success(.goOnFetching)
    }
    /// 0xf3  REP[EZ] repeat string operation prefix
    func Oxf3() throws -> Result<Resume, Never> {
        return .success(.goOnFetching)
    }
    /// 0x66  operand-size override prefix
    func Ox66() throws -> Result<Resume, Never> {
        return .success(.goOnFetching)
    }
    /// 0x67  address-size override prefix
    func Ox67() throws -> Result<Resume, Never> {
        return .success(.goOnFetching)
    }
    /// 0xb0  MOV AL
    /// 0xb1  MOV CL
    /// 0xb2  MOV DL
    /// 0xb3  MOV BL
    /// 0xb4  MOV AH
    /// 0xb5  MOV CH
    /// 0xb6  MOV DH
    /// 0xb7  MOV BH
    func Oxb7() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xb8  MOV A
    /// 0xb9  MOV C
    /// 0xba  MOV D
    /// 0xbb  MOV B
    /// 0xbc  MOV SP
    /// 0xbd  MOV BP
    /// 0xbe  MOV SI
    /// 0xbf  MOV DI
    func Oxbf() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x88  MOV
    func Ox88() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x89  MOV
    func Ox89() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x8a  MOV
    func Ox8a() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x8b  MOV
    func Ox8b() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa0  MOV AL,
    func Oxa0() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa1  MOV AX,
    func Oxa1() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa2  MOV ,AL
    func Oxa2() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa3  MOV ,AX
    func Oxa3() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc6  MOV
    func Oxc6() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc7  MOV
    func Oxc7() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x8e  MOV
    func Ox8e() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x8c  MOV
    func Ox8c() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x86  XCHG
    func Ox86() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x87  XCHG
    func Ox87() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x91  XCHG C
    /// 0x92  XCHG D
    /// 0x93  XCHG B
    /// 0x94  XCHG SP
    /// 0x95  XCHG BP
    /// 0x96  XCHG SI
    /// 0x97  XCHG DI
    func Ox97() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xd7  XLAT
    func Oxd7() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc4  LES
    func Oxc4() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc5  LDS
    func Oxc5() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x00  ADD
    /// 0x08  OR
    /// 0x10  ADC
    /// 0x18  SBB
    /// 0x20  AND
    /// 0x28  SUB
    /// 0x30  XOR
    /// 0x38  CMP
    func Ox38() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x01  ADD
    func Ox01() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x09  OR
    /// 0x11  ADC
    /// 0x19  SBB
    /// 0x21  AND
    /// 0x29  SUB
    /// 0x31  XOR
    func Ox31() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x39  CMP
    func Ox39() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x02  ADD
    /// 0x0a  OR
    /// 0x12  ADC
    /// 0x1a  SBB
    /// 0x22  AND
    /// 0x2a  SUB
    /// 0x32  XOR
    /// 0x3a  CMP
    func Ox3a() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x03  ADD
    func Ox03() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x0b  OR
    /// 0x13  ADC
    /// 0x1b  SBB
    /// 0x23  AND
    /// 0x2b  SUB
    /// 0x33  XOR
    func Ox33() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x3b  CMP
    func Ox3b() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x04  ADD
    /// 0x0c  OR
    /// 0x14  ADC
    /// 0x1c  SBB
    /// 0x24  AND
    /// 0x2c  SUB
    /// 0x34  XOR
    /// 0x3c  CMP
    func Ox3c() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x05  ADD
    func Ox05() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x0d  OR
    /// 0x15  ADC
    /// 0x1d  SBB
    /// 0x25  AND
    /// 0x2d  SUB
    func Ox2d() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x35  XOR
    func Ox35() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x3d  CMP
    func Ox3d() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x80  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    /// 0x82  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    func Ox82() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x81  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    func Ox81() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x83  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    func Ox83() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x40  INC A
    /// 0x41  INC C
    /// 0x42  INC D
    /// 0x43  INC B
    /// 0x44  INC SP
    /// 0x45  INC BP
    /// 0x46  INC SI
    /// 0x47  INC DI
    func Ox47() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x48  DEC A
    /// 0x49  DEC C
    /// 0x4a  DEC D
    /// 0x4b  DEC B
    /// 0x4c  DEC SP
    /// 0x4d  DEC BP
    /// 0x4e  DEC SI
    /// 0x4f  DEC DI
    func Ox4f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x69  IMUL
    func Ox69() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x6b  IMUL
    func Ox6b() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x84  TEST
    func Ox84() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x85  TEST
    func Ox85() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa8  TEST
    func Oxa8() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa9  TEST
    func Oxa9() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xf6  G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
    func Oxf6() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xf7  G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
    func Oxf7() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc0  G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
    func Oxc0() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc1  G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
    func Oxc1() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xd0  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
    func Oxd0() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xd1  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
    func Oxd1() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xd2  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
    func Oxd2() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xd3  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
    func Oxd3() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x98  CBW
    func Ox98() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x99  CWD
    func Ox99() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x50  PUSH A
    /// 0x51  PUSH C
    /// 0x52  PUSH D
    /// 0x53  PUSH B
    /// 0x54  PUSH SP
    /// 0x55  PUSH BP
    /// 0x56  PUSH SI
    /// 0x57  PUSH DI
    func Ox57() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x58  POP A
    /// 0x59  POP C
    /// 0x5a  POP D
    /// 0x5b  POP B
    /// 0x5c  POP SP
    /// 0x5d  POP BP
    /// 0x5e  POP SI
    /// 0x5f  POP DI
    func Ox5f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x60  PUSHA
    func Ox60() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x61  POPA
    func Ox61() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x8f  POP
    func Ox8f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x68  PUSH
    func Ox68() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x6a  PUSH
    func Ox6a() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc8  ENTER
    func Oxc8() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc9  LEAVE
    func Oxc9() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x9c  PUSHF
    func Ox9c() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x9d  POPF
    func Ox9d() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoopOnInterrupt)
    }
    /// 0x06  PUSH
    /// 0x0e  PUSH
    /// 0x16  PUSH
    /// 0x1e  PUSH
    func Ox1e() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x07  POP
    /// 0x17  POP
    /// 0x1f  POP
    func Ox1f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x8d  LEA
    func Ox8d() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xfe  G4 (INC, DEC, -, -, -, -, -)
    func Oxfe() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xff  G5 (INC, DEC, CALL, CALL, JMP, JMP, PUSH, -)
    func Oxff() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xeb  JMP
    func Oxeb() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xe9  JMP
    func Oxe9() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xea  JMPF
    func Oxea() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x70  JO
    func Ox70() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x71  JNO
    func Ox71() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x72  JB
    func Ox72() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x73  JNB
    func Ox73() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x74  JZ
    func Ox74() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x75  JNZ
    func Ox75() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x76  JBE
    func Ox76() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x77  JNBE
    func Ox77() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x78  JS
    func Ox78() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x79  JNS
    func Ox79() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x7a  JP
    func Ox7a() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x7b  JNP
    func Ox7b() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x7c  JL
    func Ox7c() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x7d  JNL
    func Ox7d() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x7e  JLE
    func Ox7e() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x7f  JNLE
    func Ox7f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xe0  LOOPNE
    /// 0xe1  LOOPE
    /// 0xe2  LOOP
    func Oxe2() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xe3  JCXZ
    func Oxe3() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc2  RET
    func Oxc2() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc3  RET
    func Oxc3() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xe8  CALL
    func Oxe8() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x9a  CALLF
    func Ox9a() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoopOnInterrupt)
    }
    /// 0xca  RET
    func Oxca() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoopOnInterrupt)
    }
    /// 0xcb  RET
    func Oxcb() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoopOnInterrupt)
    }
    /// 0xcf  IRET
    func Oxcf() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoopOnInterrupt)
    }
    /// 0x90  NOP
    func Ox90() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xcc  INT
    func Oxcc() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xcd  INT
    func Oxcd() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xce  INTO
    func Oxce() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x62  BOUND
    func Ox62() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xf5  CMC
    func Oxf5() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xf8  CLC
    func Oxf8() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xf9  STC
    func Oxf9() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xfc  CLD
    func Oxfc() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xfd  STD
    func Oxfd() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xfa  CLI
    func Oxfa() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xfb  STI
    func Oxfb() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoopOnInterrupt)
    }
    /// 0x9e  SAHF
    func Ox9e() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x9f  LAHF
    func Ox9f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xf4  HLT
    func Oxf4() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoop)
    }
    /// 0xa4  MOVSB
    func Oxa4() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa5  MOVSW/D
    func Oxa5() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xaa  STOSB
    func Oxaa() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xab  STOSW/D
    func Oxab() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa6  CMPSB
    func Oxa6() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa7  CMPSW/D
    func Oxa7() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xac  LOSB
    func Oxac() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xad  LOSW/D
    func Oxad() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xae  SCASB
    func Oxae() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xaf  SCASW/D
    func Oxaf() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x6c  INSB
    func Ox6c() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoopOnInterrupt)
    }
    /// 0x6d  INSW/D
    func Ox6d() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoopOnInterrupt)
    }
    /// 0x6e  OUTSB
    func Ox6e() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoopOnInterrupt)
    }
    /// 0x6f  OUTSW/D
    func Ox6f() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoopOnInterrupt)
    }
    /// 0xd8  ESC (80387) 11011XXX
    /// 0xd9  ESC (80387)
    /// 0xda  ESC (80387)
    /// 0xdb  ESC (80387)
    /// 0xdc  ESC (80387)
    /// 0xdd  ESC (80387)
    /// 0xde  ESC (80387)
    /// 0xdf  ESC (80387)
    func Oxdf() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x9b  FWAIT/WAIT
    func Ox9b() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xe4  IN AL,
    func Oxe4() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoopOnInterrupt)
    }
    /// 0xe5  IN AX,
    func Oxe5() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoopOnInterrupt)
    }
    /// 0xe6  OUT ,AL
    func Oxe6() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoopOnInterrupt)
    }
    /// 0xe7  OUT ,AX
    func Oxe7() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoopOnInterrupt)
    }
    /// 0xec  IN AL,DX
    func Oxec() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoopOnInterrupt)
    }
    /// 0xed  IN AX,DX
    func Oxed() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoopOnInterrupt)
    }
    /// 0xee  OUT DX,AL
    func Oxee() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoopOnInterrupt)
    }
    /// 0xef  OUT DX,AX
    func Oxef() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoopOnInterrupt)
    }
    /// 0x27  DAA
    func Ox27() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x2f  DAS
    func Ox2f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x37  AAA
    func Ox37() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x3f  AAS
    func Ox3f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xd4  AAM
    func Oxd4() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xd5  AAD
    func Oxd5() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x63  ARPL
    func Ox63() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xd6  -
    /// 0xf1  -
    func Oxf1() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x0f  2-byte instruction escape
    mutating func Ox0f() throws -> Result<Resume, Never> {
        // opcode = fetch8()
        return try twoByteDecoder[opcode]()
    }
}
