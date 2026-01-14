/// one-byte 16 bit opcodes
extension Free86 {
    /// 0x189  MOV
    func oOx189() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x18b  MOV
    func oOx18b() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1b8  MOV A
    /// 0x1b9  MOV C
    /// 0x1ba  MOV D
    /// 0x1bb  MOV B
    /// 0x1bc  MOV SP
    /// 0x1bd  MOV BP
    /// 0x1be  MOV SI
    /// 0x1bf  MOV DI
    func oOx1bf() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1a1  MOV AX,
    func oOx1a1() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1a3  MOV ,AX
    func oOx1a3() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1c7  MOV
    func oOx1c7() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x191  XCHG C
    /// 0x192  XCHG D
    /// 0x193  XCHG B
    /// 0x194  XCHG SP
    /// 0x195  XCHG BP
    /// 0x196  XCHG SI
    /// 0x197  XCHG DI
    func oOx197() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x187  XCHG
    func oOx187() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1c4  LES
    func oOx1c4() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1c5  LDS
    func oOx1c5() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x101  ADD
    /// 0x109  OR
    /// 0x111  ADC
    /// 0x119  SBB
    /// 0x121  AND
    /// 0x129  SUB
    /// 0x131  XOR
    /// 0x139  CMP
    func oOx139() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x103  ADD
    /// 0x10b  OR
    /// 0x113  ADC
    /// 0x11b  SBB
    /// 0x123  AND
    /// 0x12b  SUB
    /// 0x133  XOR
    /// 0x13b  CMP
    func oOx13b() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x105  ADD
    /// 0x10d  OR
    /// 0x115  ADC
    /// 0x11d  SBB
    /// 0x125  AND
    /// 0x12d  SUB
    /// 0x135  XOR
    /// 0x13d  CMP
    func oOx13d() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x181  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    func oOx181() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x183  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    func oOx183() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x140  INC A
    /// 0x141  INC C
    /// 0x142  INC D
    /// 0x143  INC B
    /// 0x144  INC SP
    /// 0x145  INC BP
    /// 0x146  INC SI
    /// 0x147  INC DI
    func oOx147() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x148  DEC A
    /// 0x149  DEC C
    /// 0x14a  DEC D
    /// 0x14b  DEC B
    /// 0x14c  DEC SP
    /// 0x14d  DEC BP
    /// 0x14e  DEC SI
    /// 0x14f  DEC DI
    func oOx14f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x16b  IMUL
    func oOx16b() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x169  IMUL
    func oOx169() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x185  TEST
    func oOx185() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1a9  TEST
    func oOx1a9() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1f7  G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
    func oOx1f7() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1c1  G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
    func oOx1c1() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1d1  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
    func oOx1d1() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1d3  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
    func oOx1d3() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x198  CBW
    func oOx198() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x199  CWD
    func oOx199() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x190  NOP
    func oOx190() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x150  PUSH A
    /// 0x151  PUSH C
    /// 0x152  PUSH D
    /// 0x153  PUSH B
    /// 0x154  PUSH SP
    /// 0x155  PUSH BP
    /// 0x156  PUSH SI
    /// 0x157  PUSH DI
    func oOx157() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x158  POP A
    /// 0x159  POP C
    /// 0x15a  POP D
    /// 0x15b  POP B
    /// 0x15c  POP SP
    /// 0x15d  POP BP
    /// 0x15e  POP SI
    /// 0x15f  POP DI
    func oOx15f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x160  PUSHA
    func oOx160() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x161  POPA
    func oOx161() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x18f  POP
    func oOx18f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x168  PUSH
    func oOx168() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x16a  PUSH
    func oOx16a() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1c8  ENTER
    func oOx1c8() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1c9  LEAVE
    func oOx1c9() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x106  PUSH
    /// 0x10e  PUSH
    /// 0x116  PUSH
    /// 0x11e  PUSH
    func oOx11e() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x107  POP
    /// 0x117  POP
    /// 0x11f  POP
    func oOx11f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x18d  LEA
    func oOx18d() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1ff  G5 (INC, DEC, CALL, CALL, JMP, JMP, PUSH, -)
    func oOx1ff() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1eb  JMP
    func oOx1eb() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1e9  JMP
    func oOx1e9() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x170  JO
    /// 0x171  JNO
    /// 0x172  JB
    /// 0x173  JNB
    /// 0x174  JZ
    /// 0x175  JNZ
    /// 0x176  JBE
    /// 0x177  JNBE
    /// 0x178  JS
    /// 0x179  JNS
    /// 0x17a  JP
    /// 0x17b  JNP
    /// 0x17c  JL
    /// 0x17d  JNL
    /// 0x17e  JLE
    /// 0x17f  JNLE
    func oOx17f() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1c2  RET
    func oOx1c2() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1c3  RET
    func oOx1c3() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1e8  CALL
    func oOx1e8() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x162  BOUND
    func oOx162() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1a5  MOVSW/D
    func oOx1a5() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1a7  CMPSW/D
    func oOx1a7() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1ad  LOSW/D
    func oOx1ad() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1af  SCASW/D
    func oOx1af() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x1ab  STOSW/D
    func oOx1ab() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0x16d  INSW/D
    mutating func oOx16d() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoopLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0x16f  OUTSW/D
    mutating func oOx16f() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoopLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0x1e5  IN AX,
    mutating func oOx1e5() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoopLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0x1e7  OUT ,AX
    mutating func oOx1e7() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoopLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0x1ed  IN AX,DX
    mutating func oOx1ed() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoopLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0x1ef  OUT DX,AX
    mutating func oOx1ef() throws -> Result<Resume, Never> {
        if (eflags.isFlagRaised(.IF) && INTR.probe()) {
            return .success(.endCyclesLoopLoop)
        }
        return .success(.endFetchLoop)
    }
    /// 0x126  ES segment override prefix
    /// 0x12e  CS segment override prefix
    /// 0x136  SS segment override prefix
    /// 0x13e  DS segment override prefix
    /// 0x164  FS segment override prefix
    /// 0x165  GS segment override prefix
    /// 0x1f0  LOCK prefix
    /// 0x1f2  REPN[EZ] repeat string operation prefix
    /// 0x1f3  REP[EZ] repeat string operation prefix
    /// 0x166  operand-size override prefix
    /// 0x167  address-size override prefix
    /// 0x100  ADD
    /// 0x102  ADD
    /// 0x104  ADD
    /// 0x108  OR
    /// 0x10a  OR
    /// 0x10c  OR
    /// 0x110  ADC
    /// 0x112  ADC
    /// 0x114  ADC
    /// 0x118  SBB
    /// 0x11a  SBB
    /// 0x11c  SBB
    /// 0x120  AND
    /// 0x122  AND
    /// 0x124  AND
    /// 0x127  DAA
    /// 0x128  SUB
    /// 0x12a  SUB
    /// 0x12c  SUB
    /// 0x12f  DAS
    /// 0x130  XOR
    /// 0x132  XOR
    /// 0x134  XOR
    /// 0x137  AAA
    /// 0x138  CMP
    /// 0x13a  CMP
    /// 0x13c  CMP
    /// 0x13f  AAS
    /// 0x16c  INSB
    /// 0x16e  OUTSB
    /// 0x180  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    /// 0x182  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    /// 0x184  TEST
    /// 0x186  XCHG
    /// 0x188  MOV
    /// 0x18a  MOV
    /// 0x18c  MOV
    /// 0x18e  MOV
    /// 0x19a  CALLF
    /// 0x19b  FWAIT/WAIT
    /// 0x19c  PUSHF
    /// 0x19d  POPF
    /// 0x19e  SAHF
    /// 0x19f  LAHF
    /// 0x1a0  MOV AL,
    /// 0x1a2  MOV ,AL
    /// 0x1a4  MOVSB
    /// 0x1a6  CMPSB
    /// 0x1a8  TEST
    /// 0x1aa  STOSB
    /// 0x1ac  LOSB
    /// 0x1ae  SCASB
    /// 0x1b0  MOV AL
    /// 0x1b1  MOV CL
    /// 0x1b2  MOV DL
    /// 0x1b3  MOV BL
    /// 0x1b4  MOV AH
    /// 0x1b5  MOV CH
    /// 0x1b6  MOV DH
    /// 0x1b7  MOV BH
    /// 0x1c0  G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
    /// 0x1c6  MOV
    /// 0x1ca  RET
    /// 0x1cb  RET
    /// 0x1cc  INT
    /// 0x1cd  INT
    /// 0x1ce  INTO
    /// 0x1cf  IRET
    /// 0x1d0  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
    /// 0x1d2  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
    /// 0x1d4  AAM
    /// 0x1d5  AAD
    /// 0x1d7  XLAT
    /// 0x1d8  ESC (80387)
    /// 0x1d9  ESC (80387)
    /// 0x1da  ESC (80387)
    /// 0x1db  ESC (80387)
    /// 0x1dc  ESC (80387)
    /// 0x1dd  ESC (80387)
    /// 0x1de  ESC (80387)
    /// 0x1df  ESC (80387)
    /// 0x1e0  LOOPNE
    /// 0x1e1  LOOPE
    /// 0x1e2  LOOP
    /// 0x1e3  JCXZ
    /// 0x1e4  IN AL,
    /// 0x1e6  OUT ,AL
    /// 0x1ea  JMPF
    /// 0x1ec  IN AL,DX
    /// 0x1ee  OUT DX,AL
    /// 0x1f4  HLT
    /// 0x1f5  CMC
    /// 0x1f6  G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
    /// 0x1f8  CLC
    /// 0x1f9  STC
    /// 0x1fa  CLI
    /// 0x1fb  STI
    /// 0x1fc  CLD
    /// 0x1fd  STD
    /// 0x1fe  G4 (INC, DEC, -, -, -, -, -)
    mutating func oOx1fe() throws -> Result<Resume, Never> {
        opcode.clearBit(InstructionPrefixRegisterFlag.operandSizeOverride.rawValue)
        return .success(.goOnFetching)
    }
    /// 0x163  ARPL
    /// 0x1d6  -
    /// 0x1f1  -
    /// 0x10f  2-byte instruction escape
    mutating func oOx10f() throws -> Result<Resume, Never> {
        // opcode = fetch8()
        opcode.raiseBit(InstructionPrefixRegisterFlag.operandSizeOverride.rawValue)
        return try twoByteDecoder[opcode]()
    }
}
