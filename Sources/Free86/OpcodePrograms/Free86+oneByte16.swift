/// one-byte 16 bit opcode programs
extension Free86 {
    /// 0x189  MOV
    func Ox189() throws -> Result<Resume, Never> {
        modRM = fetch8()
        r = regs[modRM.reg]
        if modRM.mod == 3 {
            regs[modRM.rM].wordX = r
        } else {
            segmentTranslation()
            try st16WritableCpl3(word: r)
        }
        return .success(.endFetchLoop)
    }
    /// 0x18b  MOV
    func Ox18b() throws -> Result<Resume, Never> {
        modRM = fetch8()
        if modRM.mod == 3 {
            rm = regs[modRM.rM]
        } else {
            segmentTranslation()
            rm = DWord(try ld16ReadonlyCpl3())
        }
        regs[modRM.reg].wordX = rm
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
    func Ox1bf() throws -> Result<Resume, Never> {
        regs[opcode & 7].wordX = DWord(fetch16())
        return .success(.endFetchLoop)
    }
    /// 0x1a1  MOV AX,
    func Ox1a1() throws -> Result<Resume, Never> {
        try ldMemoryOffset(false)
        moffs = DWord(try ld16ReadonlyCpl3())
        regs[0].wordX = moffs
        return .success(.endFetchLoop)
    }
    /// 0x1a3  MOV ,AX
    func Ox1a3() throws -> Result<Resume, Never> {
        try ldMemoryOffset(true)
        try st16WritableCpl3(word: regs[.EAX])
        return .success(.endFetchLoop)
    }
    /// 0x1c7  MOV
    func Ox1c7() throws -> Result<Resume, Never> {
        modRM = fetch8()
        if modRM.mod == 3 {
            imm = DWord(fetch16())
            regs[modRM.rM].wordX = imm
        } else {
            segmentTranslation()
            imm = DWord(fetch16())
            try st16WritableCpl3(word: imm)
        }
        return .success(.endFetchLoop)
    }
    /// 0x191  XCHG C
    /// 0x192  XCHG D
    /// 0x193  XCHG B
    /// 0x194  XCHG SP
    /// 0x195  XCHG BP
    /// 0x196  XCHG SI
    /// 0x197  XCHG DI
    func Ox197() throws -> Result<Resume, Never> {
        reg = opcode & 7
        u = regs[.EAX]
        regs[.EAX].wordX = regs[reg]
        regs[reg].wordX = u
        return .success(.endFetchLoop)
    }
    /// 0x187  XCHG
    func Ox187() throws -> Result<Resume, Never> {
        modRM = fetch8()
        reg = modRM.reg
        if modRM.mod == 3 {
            // LOCK prefix not allowed
            rM = modRM.rM
            rm = regs[rM]
            regs[rM].wordX = regs[reg]
        } else {
            segmentTranslation()
            rm = try ld16WritableCpl3()
            try st16WritableCpl3(word: regs[reg])
        }
        regs[reg].wordX = rm
        return .success(.endFetchLoop)
    }
    /// 0x1c4  LES
    func Ox1c4() throws -> Result<Resume, Never> {
        try ldFarPointer16(0)
        return .success(.endFetchLoop)
    }
    /// 0x1c5  LDS
    func Ox1c5() throws -> Result<Resume, Never> {
        try ldFarPointer16(3)
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
    func Ox139() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = (opcode >> 3) & 7
        r = regs[modRM.reg]
        if modRM.mod == 3 {
            // LOCK prefix not allowed
            rM = modRM.rM
            regs[rM].wordX = calculate16(regs[rM], r)
        } else {
            segmentTranslation()
            if operation != 7 {
                rm = try ld16WritableCpl3()
                u = calculate16(rm, r)
                try st16WritableCpl3(word: u)
            } else {
                rm = DWord(try ld16ReadonlyCpl3())
                calculate16(rm, r)
            }
        }
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
    func Ox13b() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = (opcode >> 3) & 7
        reg = modRM.reg
        if modRM.mod == 3 {
            rm = regs[modRM.rM]
        } else {
            segmentTranslation()
            rm = DWord(try ld16ReadonlyCpl3())
        }
        regs[reg].wordX = calculate16(regs[reg], rm)
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
    func Ox13d() throws -> Result<Resume, Never> {
        imm = DWord(fetch16())
        operation = (opcode >> 3) & 7
        regs[0].wordX = calculate16(regs[.EAX], imm)
        return .success(.endFetchLoop)
    }
    /// 0x181  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    func Ox181() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        if modRM.mod == 3 {
            // LOCK prefix not allowed
            rM = modRM.rM
            imm = DWord(fetch16())
            regs[rM].wordX = calculate16(regs[rM], imm)
        } else {
            segmentTranslation()
            imm = DWord(fetch16())
            if operation != 7 {
                rm = try ld16WritableCpl3()
                u = calculate16(rm, imm)
                try st16WritableCpl3(word: u)
            } else {
                // LOCK prefix not allowed
                rm = DWord(try ld16ReadonlyCpl3())
                calculate16(rm, imm)
            }
        }
        return .success(.endFetchLoop)
    }
    /// 0x183  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    func Ox183() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        if modRM.mod == 3 {
            // LOCK prefix not allowed
            rM = modRM.rM
            u = DWord(fetch8()).signExtendedByte
            regs[rM].wordX = calculate16(regs[rM], u)
        } else {
            segmentTranslation()
            v = DWord(fetch8()).signExtendedByte
            if operation != 7 {
                rm = try ld16WritableCpl3()
                u = calculate16(rm, v)
                try st16WritableCpl3(word: u)
            } else {
                // LOCK prefix not allowed
                rm = DWord(try ld16ReadonlyCpl3())
                calculate16(rm, v)
            }
        }
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
    func Ox147() throws -> Result<Resume, Never> {
        reg = opcode & 7
        regs[reg].wordX = aux16_INC(regs[reg])
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
    func Ox14f() throws -> Result<Resume, Never> {
        reg = opcode & 7
        regs[reg].wordX = aux16_DEC(regs[reg])
        return .success(.endFetchLoop)
    }
    /// 0x16b  IMUL
    func Ox16b() throws -> Result<Resume, Never> {
        modRM = fetch8()
        reg = modRM.reg
        if modRM.mod == 3 {
            rm = regs[modRM.rM]
        } else {
            segmentTranslation()
            rm = DWord(try ld16ReadonlyCpl3())
        }
        v = DWord(fetch8()).signExtendedByte
        aux16_IMUL(rm, v)
        regs[reg].wordX = u
        return .success(.endFetchLoop)
    }
    /// 0x169  IMUL
    func Ox169() throws -> Result<Resume, Never> {
        modRM = fetch8()
        reg = modRM.reg
        if modRM.mod == 3 {
            rm = regs[modRM.rM]
        } else {
            segmentTranslation()
            rm = DWord(try ld16ReadonlyCpl3())
        }
        imm = DWord(fetch16())
        aux16_IMUL(rm, imm)
        regs[reg].wordX = u
        return .success(.endFetchLoop)
    }
    /// 0x185  TEST
    func Ox185() throws -> Result<Resume, Never> {
        modRM = fetch8()
        if modRM.mod == 3 {
            rm = regs[modRM.rM]
        } else {
            segmentTranslation()
            rm = DWord(try ld16ReadonlyCpl3())
        }
        r = regs[modRM.reg]
        osmDst = (rm & r).signExtendedWord
        osm = 13
        return .success(.endFetchLoop)
    }
    /// 0x1a9  TEST
    func Ox1a9() throws -> Result<Resume, Never> {
        imm = DWord(fetch16())
        osmDst = (regs[.EAX] & imm).signExtendedWord
        osm = 13
        return .success(.endFetchLoop)
    }
    /// 0x1f7  G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
    func Ox1f7() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        // LOCK prefix not allowed if operation != 2 && operation != 3
        switch operation {
        case 0:  // TEST
            if modRM.mod == 3 {
                rm = regs[modRM.rM]
            } else {
                segmentTranslation()
                rm = DWord(try ld16ReadonlyCpl3())
            }
            imm = DWord(fetch16())
            osmDst = (rm & imm).signExtendedWord
            osm = 13
            break
        case 2:  // NOT
            if modRM.mod == 3 {
                // LOCK prefix not allowed
                rM = modRM.rM
                regs[rM].wordX = ~regs[rM]
            } else {
                segmentTranslation()
                rm = try ld16WritableCpl3()
                try st16WritableCpl3(word: ~rm)
            }
            break
        case 3:  // NEG
            operation = 5
            if modRM.mod == 3 {
                // LOCK prefix not allowed
                rM = modRM.rM
                regs[rM].wordX = calculate16(0, regs[rM])
            } else {
                operation = 5
                segmentTranslation()
                rm = try ld16WritableCpl3()
                u = calculate16(0, rm)
                try st16WritableCpl3(word: u)
            }
            break
        case 4:  // MUL AL/X
            if modRM.mod == 3 {
                rm = regs[modRM.rM]
            } else {
                segmentTranslation()
                rm = DWord(try ld16ReadonlyCpl3())
            }
            aux16_MUL(regs[.EAX], rm)
            regs[.EAX].wordX = u
            regs[.EDX].wordX = u >> 16
            break
        case 5:  // IMUL AL/X
            if modRM.mod == 3 {
                rm = regs[modRM.rM]
            } else {
                segmentTranslation()
                rm = DWord(try ld16ReadonlyCpl3())
            }
            aux16_IMUL(regs[.EAX], rm)
            regs[.EAX].wordX = u
            regs[.EDX].wordX = u >> 16
            break
        case 6:  // DIV AL/X
            if modRM.mod == 3 {
                rm = regs[modRM.rM]
            } else {
                segmentTranslation()
                rm = DWord(try ld16ReadonlyCpl3())
            }
            aux16_DIV(rm)
            break
        case 7:  // IDIV AL/X
            if modRM.mod == 3 {
                rm = regs[modRM.rM]
            } else {
                segmentTranslation()
                rm = DWord(try ld16ReadonlyCpl3())
            }
            aux16_IDIV(rm)
            break
        default:
            throw Interrupt(.UD)
        }
        return .success(.endFetchLoop)
    }
    /// 0x1c1  G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
    func Ox1c1() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        if modRM.mod == 3 {
            imm = DWord(fetch8())
            rM = modRM.rM
            regs[rM].wordX = shift16(regs[rM], imm)
        } else {
            segmentTranslation()
            imm = DWord(fetch8())
            rm = try ld16WritableCpl3()
            u = shift16(rm, imm)
            try st16WritableCpl3(word: u)
        }
        return .success(.endFetchLoop)
    }
    /// 0x1d1  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
    func Ox1d1() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        if modRM.mod == 3 {
            rM = modRM.rM
            regs[rM].wordX = shift16(regs[rM], 1)
        } else {
            segmentTranslation()
            rm = try ld16WritableCpl3()
            u = shift16(rm, 1)
            try st16WritableCpl3(word: u)
        }
        return .success(.endFetchLoop)
    }
    /// 0x1d3  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
    func Ox1d3() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        if modRM.mod == 3 {
            rM = modRM.rM
            regs[rM].wordX = shift16(regs[rM], regs[.ECX] & 0xff)
        } else {
            segmentTranslation()
            rm = try ld16WritableCpl3()
            u = shift16(rm, regs[.ECX] & 0xff)
            try st16WritableCpl3(word: u)
        }
        return .success(.endFetchLoop)
    }
    /// 0x198  CBW
    func Ox198() throws -> Result<Resume, Never> {
        regs[.EAX].wordX = regs[.EAX].signExtendedByte
        return .success(.endFetchLoop)
    }
    /// 0x199  CWD
    func Ox199() throws -> Result<Resume, Never> {
        regs[.EDX].wordX = (regs[.EAX] << 16).signedShiftRight(count: 31)
        return .success(.endFetchLoop)
    }
    /// 0x190  NOP
    func Ox190() throws -> Result<Resume, Never> {
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
    func Ox157() throws -> Result<Resume, Never> {
        push16(regs[opcode & 7])
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
    func Ox15f() throws -> Result<Resume, Never> {
        m = pop16()
        regs[opcode & 7].wordX = m
        return .success(.endFetchLoop)
    }
    /// 0x160  PUSHA
    func Ox160() throws -> Result<Resume, Never> {
        aux16_PUSHA()
        return .success(.endFetchLoop)
    }
    /// 0x161  POPA
    func Ox161() throws -> Result<Resume, Never> {
        aux16_POPA()
        return .success(.endFetchLoop)
    }
    /// 0x18f  POP
    func Ox18f() throws -> Result<Resume, Never> {
        modRM = fetch8()
        if modRM.mod == 3 {
            m = pop16()
            regs[modRM.rM].wordX = m
        } else {
            u = regs[.ESP]
            m = pop16()
            v = regs[.ESP]
            segmentTranslation()
            regs[.ESP] = u
            try st16WritableCpl3(word: m)
            regs[.ESP] = v
        }
        return .success(.endFetchLoop)
    }
    /// 0x168  PUSH
    func Ox168() throws -> Result<Resume, Never> {
        imm = DWord(fetch16())
        push16(imm)
        return .success(.endFetchLoop)
    }
    /// 0x16a  PUSH
    func Ox16a() throws -> Result<Resume, Never> {
        u = DWord(fetch8()).signExtendedByte
        push16(u)
        return .success(.endFetchLoop)
    }
    /// 0x1c8  ENTER
    func Ox1c8() throws -> Result<Resume, Never> {
        aux16_ENTER()
        return .success(.endFetchLoop)
    }
    /// 0x1c9  LEAVE
    func Ox1c9() throws -> Result<Resume, Never> {
        aux16_LEAVE()
        return .success(.endFetchLoop)
    }
    /// 0x106  PUSH
    /// 0x10e  PUSH
    /// 0x116  PUSH
    /// 0x11e  PUSH
    func Ox11e() throws -> Result<Resume, Never> {
        push16(segs[(opcode >> 3) & 3].selector)
        return .success(.endFetchLoop)
    }
    /// 0x107  POP
    /// 0x117  POP
    /// 0x11f  POP
    func Ox11f() throws -> Result<Resume, Never> {
        m = pop16()
        let sreg = SegmentRegister.Name(rawValue: (opcode >> 3) & 3)!
        setSegmentRegister(sreg, m)
        return .success(.endFetchLoop)
    }
    /// 0x18d  LEA
    func Ox18d() throws -> Result<Resume, Never> {
        modRM = fetch8()
        if modRM.mod == 3 {
            throw Interrupt(.UD)
        }
        ipr.segmentRegister = SegmentRegister.Name.LDT.rawValue
        segmentTranslation()
        regs[modRM.reg].wordX = lax
        return .success(.endFetchLoop)
    }
    /// 0x1ff  G5 (INC, DEC, CALL, CALL, JMP, JMP, PUSH, -)
    func Ox1ff() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        // LOCK prefix not allowed if operation != 0 && operation != 1
        switch operation {
        case 0:  // INC
            if modRM.mod == 3 {
                // LOCK prefix not allowed
                rM = modRM.rM
                regs[rM].wordX = aux16_INC(regs[rM])
            } else {
                segmentTranslation()
                rm = try ld16WritableCpl3()
                u = aux16_INC(rm)
                try st16WritableCpl3(word: u)
            }
            break
        case 1:  // DEC
            if modRM.mod == 3 {
                // LOCK prefix not allowed
                rM = modRM.rM
                regs[rM].wordX = aux16_DEC(regs[rM])
            } else {
                segmentTranslation()
                rm = try ld16WritableCpl3()
                u = aux16_DEC(rm)
                try st16WritableCpl3(word: u)
            }
            break
        case 2:  // CALL
            if modRM.mod == 3 {
                rm = regs[modRM.rM].lowerHalf
            } else {
                segmentTranslation()
                rm = DWord(try ld16ReadonlyCpl3())
            }
            push16((eip &+ far &- farStart))
            eip = rm
            far = 0
            farStart = 0
            break
        case 3:  // CALL
        case 5:  // JMP
            if modRM.mod == 3 {
                throw Interrupt(.UD)
            }
            segmentTranslation()
            m = DWord(try ld16ReadonlyCpl3())
            lax = lax &+ 2
            m16 = try ld16ReadonlyCpl3()
            if operation == 3 {
                aux_CALLF(0, m16, m, (eip &+ far &- farStart))
            } else {
                aux_JMPF(m16, m)
            }
            break
        case 6:  // PUSH
            if modRM.mod == 3 {
                rm = regs[modRM.rM]
            } else {
                segmentTranslation()
                rm = DWord(try ld16ReadonlyCpl3())
            }
            push16(rm)
            break
        case 4:  // JMP
            if modRM.mod == 3 {
                rm = regs[modRM.rM].lowerHalf
            } else {
                segmentTranslation()
                rm = DWord(try ld16ReadonlyCpl3())
            }
            eip = rm
            far = 0
            farStart = 0
            break
        default:
            throw Interrupt(.UD)
        }
        return .success(.endFetchLoop)
    }
    /// 0x1eb  JMP
    func Ox1eb() throws -> Result<Resume, Never> {
        u = DWord(fetch8()).signExtendedByte
        eip = (eip &+ far &- farStart &+ u).lowerHalf
        far = 0
        farStart = 0
        return .success(.endFetchLoop)
    }
    /// 0x1e9  JMP
    func Ox1e9() throws -> Result<Resume, Never> {
        imm = DWord(fetch16())
        eip = (eip &+ far &- farStart &+ imm).lowerHalf
        far = 0
        farStart = 0
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
    func Ox17f() throws -> Result<Resume, Never> {
        u = DWord(fetch8()).signExtendedByte
        if canJmp(Int(opcode & 0xf)) {
            eip = (eip &+ far &- farStart &+ u).lowerHalf
            far = 0
            farStart = 0
        }
        return .success(.endFetchLoop)
    }
    /// 0x1c2  RET
    func Ox1c2() throws -> Result<Resume, Never> {
        u = DWord(fetch16()).signExtendedWord
        m = try ld16Stack()
        regs[.ESP] = (regs[.ESP] & ~ssMask) | ((regs[.ESP] &+ 2 &+ u) & ssMask)
        eip = m
        far = 0
        farStart = 0
        return .success(.endFetchLoop)
    }
    /// 0x1c3  RET
    func Ox1c3() throws -> Result<Resume, Never> {
        m = pop16()
        eip = m
        far = 0
        farStart = 0
        return .success(.endFetchLoop)
    }
    /// 0x1e8  CALL
    func Ox1e8() throws -> Result<Resume, Never> {
        imm = DWord(fetch16())
        push16((eip &+ far &- farStart))
        eip = (eip &+ far &- farStart &+ imm).lowerHalf
        far = 0
        farStart = 0
        return .success(.endFetchLoop)
    }
    /// 0x162  BOUND
    func Ox162() throws -> Result<Resume, Never> {
        aux16_BOUND()
        return .success(.endFetchLoop)
    }
    /// 0x1a5  MOVSW/D
    func Ox1a5() throws -> Result<Resume, Never> {
        aux16_MOVS()
        return .success(.endFetchLoop)
    }
    /// 0x1a7  CMPSW/D
    func Ox1a7() throws -> Result<Resume, Never> {
        aux16_CMPS()
        return .success(.endFetchLoop)
    }
    /// 0x1ad  LOSW/D
    func Ox1ad() throws -> Result<Resume, Never> {
        aux16_LODS()
        return .success(.endFetchLoop)
    }
    /// 0x1af  SCASW/D
    func Ox1af() throws -> Result<Resume, Never> {
        aux16_SCAS()
        return .success(.endFetchLoop)
    }
    /// 0x1ab  STOSW/D
    func Ox1ab() throws -> Result<Resume, Never> {
        aux16_STOS()
        return .success(.endFetchLoop)
    }
    /// 0x16d  INSW/D
    func Ox16d() throws -> Result<Resume, Never> {
        aux16_INS()
        return .success(.endOnInterrupt)
    }
    /// 0x16f  OUTSW/D
    func Ox16f() throws -> Result<Resume, Never> {
        aux16_OUTS()
        return .success(.endOnInterrupt)
    }
    /// 0x1e5  IN AX,
    func Ox1e5() throws -> Result<Resume, Never> {
        if cpl > eflags.iopl {
            throw Interrupt(.GP, errorCode: 0)
        }
        imm = DWord(fetch8())
        regs[.EAX].wordX = io?[imm]
        return .success(.endOnInterrupt)
    }
    /// 0x1e7  OUT ,AX
    func Ox1e7() throws -> Result<Resume, Never> {
        if cpl > eflags.iopl {
            throw Interrupt(.GP, errorCode: 0)
        }
        imm = DWord(fetch8())
        io?[imm] = regs[.EAX].lowerHalf
        return .success(.endOnInterrupt)
    }
    /// 0x1ed  IN AX,DX
    func Ox1ed() throws -> Result<Resume, Never> {
        if cpl > eflags.iopl {
            throw Interrupt(.GP, errorCode: 0)
        }
        regs[.EAX].wordX = io?[regs[.EDX].lowerHalf]
        return .success(.endOnInterrupt)
    }
    /// 0x1ef  OUT DX,AX
    func Ox1ef() throws -> Result<Resume, Never> {
        if cpl > eflags.iopl {
            throw Interrupt(.GP, errorCode: 0)
        }
        io?[regs[.EDX].lowerHalf] = regs[.EAX].lowerHalf
        return .success(.endOnInterrupt)
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
    /// 0x1fe  G4 (INC, DEC, -, -, -, -, -, -)
    func Ox1fe() throws -> Result<Resume, Never> {
        opcode.setBit(InstructionPrefixRegisterFlag.operandSizeOverride.rawValue, .zero)
        return .success(.goOnFetching)
    }
    /// 0x163  ARPL
    /// 0x1d6  -
    /// 0x1f1  -
    /// 0x10f  2-byte instruction escape
    func Ox10f() throws -> Result<Resume, Never> {
        opcode = DWord(fetch8())
        opcode.setBit(InstructionPrefixRegisterFlag.operandSizeOverride.rawValue)
        if ipr.isBitRaised(.lockSignal) {
            switch opcode {
                case 0x1a3,  // BT
                    0x1ab,  // BTS
                    0x1b1,  // CMPXCHG
                    0x1b3,  // BTR
                    0x1ba,  // G8 (-, -, -, -, BT, BTS, BTR, BTC)
                    0x1bb,  // BTC
                    0x1c1:  // XADD
                    break
                default:
                    throw Interrupt(.UD)
            }
        }
        return try twoByte16Decoder[Int(opcode)]()
    }
}
