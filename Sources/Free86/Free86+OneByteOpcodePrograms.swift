/// one-byte opcodes
extension Free86 {
    /// 0x26  ES segment override prefix
    /// 0x2e  CS segment override prefix
    /// 0x36  SS segment override prefix
    /// 0x3e  DS segment override prefix
    func oOx3e() throws -> Result<Resume, Never> {
        return .success(.goOnFetching)
    }
    /// 0x64  FS segment override prefix
    /// 0x65  GS segment override prefix
    func oOx65() throws -> Result<Resume, Never> {
        return .success(.goOnFetching)
    }
    /// 0xf0  LOCK prefix
    func oOxf0() throws -> Result<Resume, Never> {
        return .success(.goOnFetching)
    }
    /// 0xf2  REPN[EZ] repeat string operation prefix
    func oOxf2() throws -> Result<Resume, Never> {
        return .success(.goOnFetching)
    }
    /// 0xf3  REP[EZ] repeat string operation prefix
    func oOxf3() throws -> Result<Resume, Never> {
        return .success(.goOnFetching)
    }
    /// 0x66  operand-size override prefix
    func oOx66() throws -> Result<Resume, Never> {
        return .success(.goOnFetching)
    }
    /// 0x67  address-size override prefix
    func oOx67() throws -> Result<Resume, Never> {
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
    func oOxb7() throws -> Result<Resume, Never> {
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
    func oOxbf() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x88  MOV
    func oOx88() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x89  MOV
    func oOx89() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x8a  MOV
    func oOx8a() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x8b  MOV
    func oOx8b() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa0  MOV AL,
    func oOxa0() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa1  MOV AX,
    func oOxa1() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa2  MOV ,AL
    func oOxa2() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa3  MOV ,AX
    func oOxa3() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc6  MOV
    func oOxc6() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc7  MOV
    func oOxc7() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x8e  MOV
    func oOx8e() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x8c  MOV
    func oOx8c() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x86  XCHG
    func oOx86() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x87  XCHG
    func oOx87() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x91  XCHG C
    /// 0x92  XCHG D
    /// 0x93  XCHG B
    /// 0x94  XCHG SP
    /// 0x95  XCHG BP
    /// 0x96  XCHG SI
    /// 0x97  XCHG DI
    func oOx97() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xd7  XLAT
    func oOxd7() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc4  LES
    func oOxc4() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc5  LDS
    func oOxc5() throws -> Result<Resume, Never> {
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
    func oOx38() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x01  ADD
    func oOx01() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x09  OR
    /// 0x11  ADC
    /// 0x19  SBB
    /// 0x21  AND
    /// 0x29  SUB
    /// 0x31  XOR
    func oOx31() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x39  CMP
    func oOx39() throws -> Result<Resume, Never> {
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
    func oOx3a() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x03  ADD
    func oOx03() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x0b  OR
    /// 0x13  ADC
    /// 0x1b  SBB
    /// 0x23  AND
    /// 0x2b  SUB
    /// 0x33  XOR
    func oOx33() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x3b  CMP
    func oOx3b() throws -> Result<Resume, Never> {
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
    func oOx3c() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x05  ADD
    func oOx05() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x0d  OR
    /// 0x15  ADC
    /// 0x1d  SBB
    /// 0x25  AND
    /// 0x2d  SUB
    func oOx2d() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x35  XOR
    func oOx35() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x3d  CMP
    func oOx3d() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x80  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    /// 0x82  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    func oOx82() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x81  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    func oOx81() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x83  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    func oOx83() throws -> Result<Resume, Never> {
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
    func oOx47() throws -> Result<Resume, Never> {
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
    func oOx4f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x69  IMUL
    func oOx69() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x6b  IMUL
    func oOx6b() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x84  TEST
    func oOx84() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x85  TEST
    func oOx85() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa8  TEST
    func oOxa8() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa9  TEST
    func oOxa9() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xf6  G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
    func oOxf6() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xf7  G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
    func oOxf7() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc0  G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
    func oOxc0() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc1  G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
    func oOxc1() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xd0  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
    func oOxd0() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xd1  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
    func oOxd1() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xd2  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
    func oOxd2() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xd3  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
    func oOxd3() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x98  CBW
    func oOx98() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x99  CWD
    func oOx99() throws -> Result<Resume, Never> {
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
    func oOx57() throws -> Result<Resume, Never> {
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
    func oOx5f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x60  PUSHA
    func oOx60() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x61  POPA
    func oOx61() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x8f  POP
    func oOx8f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x68  PUSH
    func oOx68() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x6a  PUSH
    func oOx6a() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc8  ENTER
    func oOxc8() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc9  LEAVE
    func oOxc9() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x9c  PUSHF
    func oOx9c() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x9d  POPF
    func oOx9d() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0x06  PUSH
    /// 0x0e  PUSH
    /// 0x16  PUSH
    /// 0x1e  PUSH
    func oOx1e() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x07  POP
    /// 0x17  POP
    /// 0x1f  POP
    func oOx1f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x8d  LEA
    func oOx8d() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xfe  G4 (INC, DEC, -, -, -, -, -)
    func oOxfe() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xff  G5 (INC, DEC, CALL, CALL, JMP, JMP, PUSH, -)
    func oOxff() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xeb  JMP
    func oOxeb() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xe9  JMP
    func oOxe9() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xea  JMPF
    func oOxea() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x70  JO
    func oOx70() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x71  JNO
    func oOx71() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x72  JB
    func oOx72() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x73  JNB
    func oOx73() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x74  JZ
    func oOx74() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x75  JNZ
    func oOx75() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x76  JBE
    func oOx76() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x77  JNBE
    func oOx77() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x78  JS
    func oOx78() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x79  JNS
    func oOx79() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x7a  JP
    func oOx7a() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x7b  JNP
    func oOx7b() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x7c  JL
    func oOx7c() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x7d  JNL
    func oOx7d() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x7e  JLE
    func oOx7e() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x7f  JNLE
    func oOx7f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xe0  LOOPNE
    /// 0xe1  LOOPE
    /// 0xe2  LOOP
    func oOxe2() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xe3  JCXZ
    func oOxe3() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc2  RET
    func oOxc2() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xc3  RET
    func oOxc3() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xe8  CALL
    func oOxe8() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x9a  CALLF
    mutating func oOx9a() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0xca  RET
    mutating func oOxca() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0xcb  RET
    mutating func oOxcb() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0xcf  IRET
    mutating func oOxcf() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0x90  NOP
    func oOx90() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xcc  INT
    func oOxcc() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xcd  INT
    func oOxcd() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xce  INTO
    func oOxce() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x62  BOUND
    func oOx62() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xf5  CMC
    func oOxf5() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xf8  CLC
    func oOxf8() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xf9  STC
    func oOxf9() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xfc  CLD
    func oOxfc() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xfd  STD
    func oOxfd() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xfa  CLI
    func oOxfa() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xfb  STI
    mutating func oOxfb() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0x9e  SAHF
    func oOx9e() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x9f  LAHF
    func oOx9f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xf4  HLT
    func oOxf4() throws -> Result<Resume, Never> {
        return .success(.endCyclesLoop)
    }
    /// 0xa4  MOVSB
    func oOxa4() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa5  MOVSW/D
    func oOxa5() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xaa  STOSB
    func oOxaa() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xab  STOSW/D
    func oOxab() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa6  CMPSB
    func oOxa6() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xa7  CMPSW/D
    func oOxa7() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xac  LOSB
    func oOxac() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xad  LOSW/D
    func oOxad() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xae  SCASB
    func oOxae() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xaf  SCASW/D
    func oOxaf() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x6c  INSB
    mutating func oOx6c() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0x6d  INSW/D
    mutating func oOx6d() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0x6e  OUTSB
    mutating func oOx6e() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0x6f  OUTSW/D
    mutating func oOx6f() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0xd8  ESC (80387) 11011XXX
    /// 0xd9  ESC (80387)
    /// 0xda  ESC (80387)
    /// 0xdb  ESC (80387)
    /// 0xdc  ESC (80387)
    /// 0xdd  ESC (80387)
    /// 0xde  ESC (80387)
    /// 0xdf  ESC (80387)
    func oOxdf() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x9b  FWAIT/WAIT
    func oOx9b() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xe4  IN AL,
    mutating func oOxe4() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0xe5  IN AX,
    mutating func oOxe5() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0xe6  OUT ,AL
    mutating func oOxe6() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0xe7  OUT ,AX
    mutating func oOxe7() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0xec  IN AL,DX
    mutating func oOxec() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0xed  IN AX,DX
    mutating func oOxed() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0xee  OUT DX,AL
    mutating func oOxee() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0xef  OUT DX,AX
    mutating func oOxef() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0x27  DAA
    func oOx27() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x2f  DAS
    func oOx2f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x37  AAA
    func oOx37() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x3f  AAS
    func oOx3f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xd4  AAM
    func oOxd4() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xd5  AAD
    func oOxd5() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x63  ARPL
    func oOx63() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xd6  -
    /// 0xf1  -
    func oOxf1() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x0f  2-byte instruction escape
    mutating func oOx0f() throws -> Result<Resume, Never> {
        // opcode = fetch8()
        return try twoByteDecoder[opcode]()
    }
}
