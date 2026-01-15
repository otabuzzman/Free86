/// one-byte opcodes
extension Instruction {
    /// 0x26  ES segment override prefix
    /// 0x2e  CS segment override prefix
    /// 0x36  SS segment override prefix
    /// 0x3e  DS segment override prefix
    /// 0x64  FS segment override prefix
    /// 0x65  GS segment override prefix
    /// 0xf0  LOCK prefix
    /// 0xf2  REPN[EZ] repeat string operation prefix
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
    /// 0x00  ADD
    /// 0x01  ADD
    /// 0x02  ADD
    /// 0x03  ADD
    /// 0x08  OR
    /// 0x09  OR
    /// 0x0a  OR
    /// 0x0b  OR
    /// 0x10  ADC
    /// 0x11  ADC
    /// 0x12  ADC
    /// 0x13  ADC
    /// 0x18  SBB
    /// 0x19  SBB
    /// 0x1a  SBB
    /// 0x1b  SBB
    /// 0x20  AND
    /// 0x21  AND
    /// 0x22  AND
    /// 0x23  AND
    /// 0x28  SUB
    /// 0x29  SUB
    /// 0x2a  SUB
    /// 0x2b  SUB
    /// 0x30  XOR
    /// 0x31  XOR
    /// 0x32  XOR
    /// 0x33  XOR
    /// 0x38  CMP
    /// 0x39  CMP
    /// 0x3a  CMP
    /// 0x3b  CMP
    /// 0x62  BOUND
    /// 0x63  ARPL
    /// 0x84  TEST
    /// 0x85  TEST
    /// 0x86  XCHG
    /// 0x87  XCHG
    /// 0x88  MOV
    /// 0x89  MOV
    /// 0x8a  MOV
    /// 0x8b  MOV
    /// 0x8c  MOV
    /// 0x8d  LEA
    /// 0x8e  MOV
    /// 0x8f  POP
    /// 0xc4  LES
    /// 0xc5  LDS
    /// 0xd0  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
    /// 0xd1  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
    /// 0xd2  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
    /// 0xd3  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
    /// 0xd8  ESC (80387)
    /// 0xd9  ESC (80387)
    /// 0xda  ESC (80387)
    /// 0xdb  ESC (80387)
    /// 0xdc  ESC (80387)
    /// 0xdd  ESC (80387)
    /// 0xde  ESC (80387)
    /// 0xdf  ESC (80387)
    /// 0xfe  G4 (INC, DEC, -, -, -, -, -)
    /// 0xff  G5 (INC, DEC, CALL, CALL, JMP, JMP, PUSH, -)
    func Oxff() throws -> Result<Resume, Never> {
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
    /// 0x6a  PUSH
    /// 0x70  JO
    /// 0x71  JNO
    /// 0x72  JB
    /// 0x73  JNB
    /// 0x74  JZ
    /// 0x75  JNZ
    /// 0x76  JBE
    /// 0x77  JNBE
    /// 0x78  JS
    /// 0x79  JNS
    /// 0x7a  JP
    /// 0x7b  JNP
    /// 0x7c  JL
    /// 0x7d  JNL
    /// 0x7e  JLE
    /// 0x7f  JNLE
    /// 0xa8  TEST
    /// 0xb0  MOV AL
    /// 0xb1  MOV CL
    /// 0xb2  MOV DL
    /// 0xb3  MOV BL
    /// 0xb4  MOV AH
    /// 0xb5  MOV CH
    /// 0xb6  MOV DH
    /// 0xb7  MOV BH
    /// 0xcd  INT
    /// 0xd4  AAM
    /// 0xd5  AAD
    /// 0xe0  LOOPNE
    /// 0xe1  LOOPE
    /// 0xe2  LOOP
    /// 0xe3  JCXZ
    /// 0xe4  IN AL,
    /// 0xe5  IN AX,
    /// 0xe6  OUT ,AL
    /// 0xe7  OUT ,AX
    /// 0xeb  JMP
    func Oxeb() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x05  ADD
    /// 0x0d  OR
    /// 0x15  ADC
    /// 0x1d  SBB
    /// 0x25  AND
    /// 0x2d  SUB
    /// 0x35  XOR
    /// 0x3d  CMP
    /// 0x68  PUSH
    /// 0xa9  TEST
    /// 0xb8  MOV A
    /// 0xb9  MOV C
    /// 0xba  MOV D
    /// 0xbb  MOV B
    /// 0xbc  MOV SP
    /// 0xbd  MOV BP
    /// 0xbe  MOV SI
    /// 0xbf  MOV DI
    /// 0xe8  CALL
    /// 0xe9  JMP
    func Oxe9() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x06  PUSH
    /// 0x07  POP
    /// 0x0e  PUSH
    /// 0x16  PUSH
    /// 0x17  POP
    /// 0x1e  PUSH
    /// 0x1f  POP
    /// 0x27  DAA
    /// 0x2f  DAS
    /// 0x37  AAA
    /// 0x3f  AAS
    /// 0x40  INC A
    /// 0x41  INC C
    /// 0x42  INC D
    /// 0x43  INC B
    /// 0x44  INC SP
    /// 0x45  INC BP
    /// 0x46  INC SI
    /// 0x47  INC DI
    /// 0x48  DEC A
    /// 0x49  DEC C
    /// 0x4a  DEC D
    /// 0x4b  DEC B
    /// 0x4c  DEC SP
    /// 0x4d  DEC BP
    /// 0x4e  DEC SI
    /// 0x4f  DEC DI
    /// 0x50  PUSH A
    /// 0x51  PUSH C
    /// 0x52  PUSH D
    /// 0x53  PUSH B
    /// 0x54  PUSH SP
    /// 0x55  PUSH BP
    /// 0x56  PUSH SI
    /// 0x57  PUSH DI
    /// 0x58  POP A
    /// 0x59  POP C
    /// 0x5a  POP D
    /// 0x5b  POP B
    /// 0x5c  POP SP
    /// 0x5d  POP BP
    /// 0x5e  POP SI
    /// 0x5f  POP DI
    /// 0x60  PUSHA
    /// 0x61  POPA
    /// 0x6c  INSB
    /// 0x6d  INSW/D
    /// 0x6e  OUTSB
    /// 0x6f  OUTSW/D
    /// 0x90  NOP
    /// 0x91  XCHG C
    /// 0x92  XCHG D
    /// 0x93  XCHG B
    /// 0x94  XCHG SP
    /// 0x95  XCHG BP
    /// 0x96  XCHG SI
    /// 0x97  XCHG DI
    /// 0x98  CBW
    /// 0x99  CWD
    /// 0x9b  FWAIT/WAIT
    /// 0x9c  PUSHF
    /// 0x9d  POPF
    /// 0x9e  SAHF
    /// 0x9f  LAHF
    /// 0xa4  MOVSB
    /// 0xa5  MOVSW/D
    /// 0xa6  CMPSB
    /// 0xa7  CMPSW/D
    /// 0xaa  STOSB
    /// 0xab  STOSW/D
    /// 0xac  LOSB
    /// 0xad  LOSW/D
    /// 0xae  SCASB
    /// 0xaf  SCASW/D
    /// 0xc3  RET
    /// 0xc9  LEAVE
    /// 0xcb  RET
    /// 0xcc  INT
    /// 0xce  INTO
    /// 0xcf  IRET
    /// 0xd7  XLAT
    /// 0xec  IN AL,DX
    /// 0xed  IN AX,DX
    /// 0xee  OUT DX,AL
    /// 0xef  OUT DX,AX
    /// 0xf4  HLT
    /// 0xf5  CMC
    /// 0xf8  CLC
    /// 0xf9  STC
    /// 0xfa  CLI
    /// 0xfb  STI
    /// 0xfc  CLD
    /// 0xfd  STD
    func Oxfd() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x69  IMUL
    /// 0x81  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    /// 0xc7  MOV
    func Oxc7() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x6b  IMUL
    /// 0x80  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    /// 0x82  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    /// 0x83  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    /// 0xc0  G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
    /// 0xc1  G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
    /// 0xc6  MOV
    func Oxc6() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x9a  CALLF
    /// 0xea  JMPF
    func Oxea() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa0  MOV AL,
    /// 0xa1  MOV AX,
    /// 0xa2  MOV ,AL
    /// 0xa3  MOV ,AX
    func Oxa3() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc2  RET
    /// 0xca  RET
    func Oxca() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc8  ENTER
    func Oxc8() throws -> Result<Resume, Never> {
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
    /// 0xd6  -
    /// 0xf1  -
    func Oxf1() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x0f  2-byte instruction escape
    func Ox0f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
}
