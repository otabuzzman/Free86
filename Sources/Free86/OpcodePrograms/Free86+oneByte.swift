/// one-byte opcode programs
extension Free86 {
    /// 0x26  ES segment override prefix
    /// 0x2e  CS segment override prefix
    /// 0x36  SS segment override prefix
    /// 0x3e  DS segment override prefix
    func Ox3e() throws -> Result<Resume, Never> {
        if ipr == iprDefault {
            try instruction.length()
        }
        ipr.segmentRegister = opcode.encoded(.standardSegmentRegister) & 3
        opcode = DWord(fetch8())
        opcode.override = ipr.isFlagRaised(.operandSizeOverride)
        return .success(.goOnFetching)
    }
    /// 0x64  FS segment override prefix
    /// 0x65  GS segment override prefix
    func Ox65() throws -> Result<Resume, Never> {
        if ipr == iprDefault {
            try instruction.length()
        }
        ipr.segmentRegister = opcode.encoded(.segmentRegister)
        opcode = DWord(fetch8())
        opcode.override = ipr.isFlagRaised(.operandSizeOverride)
        return .success(.goOnFetching)
    }
    /// 0xf0  LOCK prefix
    func Oxf0() throws -> Result<Resume, Never> {
        if ipr == iprDefault {
            try instruction.length()
        }
        ipr.setFlag(.lockSignal)
        opcode = DWord(fetch8())
        opcode.override = ipr.isFlagRaised(.operandSizeOverride)
        return .success(.goOnFetching)
    }
    /// 0xf2  REPN[EZ] repeat string operation prefix
    func Oxf2() throws -> Result<Resume, Never> {
        if ipr == iprDefault {
            try instruction.length()
        }
        ipr.setFlag(.repnzStringOperation)
        opcode = DWord(fetch8())
        opcode.override = ipr.isFlagRaised(.operandSizeOverride)
        return .success(.goOnFetching)
    }
    /// 0xf3  REP[EZ] repeat string operation prefix
    func Oxf3() throws -> Result<Resume, Never> {
        if ipr == iprDefault {
            try instruction.length()
        }
        ipr.setFlag(.repzStringOperation)
        opcode = DWord(fetch8())
        opcode.override = ipr.isFlagRaised(.operandSizeOverride)
       return .success(.goOnFetching)
    }
    /// 0x66  operand-size override prefix
    func Ox66() throws -> Result<Resume, Never> {
        if ipr == iprDefault {
            try instruction.length()
        }
        if iprDefault.isFlagRaised(.operandSizeOverride) {
            ipr.setFlag(.operandSizeOverride, .zero)
        } else {
            ipr.setFlag(.operandSizeOverride)
        }
        opcode = DWord(fetch8())
        opcode.override = ipr.isFlagRaised(.operandSizeOverride)
        return .success(.goOnFetching)
    }
    /// 0x67  address-size override prefix
    func Ox67() throws -> Result<Resume, Never> {
        if ipr == iprDefault {
            try instruction.length()
        }
        if iprDefault.isFlagRaised(.addressSizeOverride) {
            ipr.setFlag(.addressSizeOverride, .zero)
        } else {
            ipr.setFlag(.addressSizeOverride)
        }
        opcode = DWord(fetch8())
        opcode.override = ipr.isFlagRaised(.operandSizeOverride)
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
        imm = DWord(fetch8())
        reg = opcode.encoded(.generalRegister)
        setEncodedByte(in: reg, to: imm)
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
        imm = fetch()
        reg = opcode.encoded(.generalRegister)
        regs[reg] = imm
        return .success(.endFetchLoop)
    }
    /// 0x88  MOV
    func Ox88() throws -> Result<Resume, Never> {
        modRM = fetch8()
        reg = modRM.reg
        r = getEncodedByte(from: reg)
        if modRM.mod == 3 {
            rM = modRM.rM
            setEncodedByte(in: rM, to: r)
        } else {
            segmentTranslation()
            try st8WritableCpl3(byte: r)
        }
        return .success(.endFetchLoop)
    }
    /// 0x89  MOV
    func Ox89() throws -> Result<Resume, Never> {
        modRM = fetch8()
        r = regs[modRM.reg]
        if modRM.mod == 3 {
            regs[modRM.rM] = r
        } else {
            segmentTranslation()
            try stWritableCpl3(dword: r)
        }
        return .success(.endFetchLoop)
    }
    /// 0x8a  MOV
    func Ox8a() throws -> Result<Resume, Never> {
        modRM = fetch8()
        if modRM.mod == 3 {
            rM = modRM.rM
            rm = getEncodedByte(from: rM)
        } else {
            segmentTranslation()
            rm = DWord(try ld8ReadonlyCpl3())
        }
        reg = modRM.reg
        setEncodedByte(in: reg, to: rm)
        return .success(.endFetchLoop)
    }
    /// 0x8b  MOV
    func Ox8b() throws -> Result<Resume, Never> {
        modRM = fetch8()
        if modRM.mod == 3 {
            rm = regs[modRM.rM]
        } else {
            segmentTranslation()
            rm = try ldReadonlyCpl3()
        }
        regs[modRM.reg] = rm
        return .success(.endFetchLoop)
    }
    /// 0xa0  MOV AL,
    func Oxa0() throws -> Result<Resume, Never> {
        try ldMemoryOffset(false)
        moffs = DWord(try ld8ReadonlyCpl3())
        regs[.EAX].byteL = moffs
        return .success(.endFetchLoop)
    }
    /// 0xa1  MOV AX,
    func Oxa1() throws -> Result<Resume, Never> {
        try ldMemoryOffset(false)
        moffs = try ldReadonlyCpl3()
        regs[.EAX] = moffs
        return .success(.endFetchLoop)
    }
    /// 0xa2  MOV ,AL
    func Oxa2() throws -> Result<Resume, Never> {
        try ldMemoryOffset(true)
        try st8WritableCpl3(byte: regs[.EAX])
        return .success(.endFetchLoop)
    }
    /// 0xa3  MOV ,AX
    func Oxa3() throws -> Result<Resume, Never> {
        try ldMemoryOffset(true)
        try stWritableCpl3(dword: regs[.EAX])
        return .success(.endFetchLoop)
    }
    /// 0xc6  MOV
    func Oxc6() throws -> Result<Resume, Never> {
        modRM = fetch8()
        if modRM.mod == 3 {
            imm = DWord(fetch8())
            setEncodedByte(in: modRM.rM, to: imm)
        } else {
            segmentTranslation()
            imm = DWord(fetch8())
            try st8WritableCpl3(byte: imm)
        }
        return .success(.endFetchLoop)
    }
    /// 0xc7  MOV
    func Oxc7() throws -> Result<Resume, Never> {
        modRM = fetch8()
        if modRM.mod == 3 {
            imm = fetch()
            regs[modRM.rM] = imm
        } else {
            segmentTranslation()
            imm = fetch()
            try stWritableCpl3(dword: imm)
        }
        return .success(.endFetchLoop)
    }
    /// 0x8e  MOV
    func Ox8e() throws -> Result<Resume, Never> {
        modRM = fetch8()
        reg = modRM.reg
        if reg.isGeneralRegister(.ESI) || reg.isGeneralRegister(.EDI) || reg.isGeneralRegister(.ECX) {
            throw Interrupt(.UD)
        }
        if modRM.mod == 3 {
            rm = regs[modRM.rM].lowerHalf
        } else {
            segmentTranslation()
            rm = DWord(try ld16ReadonlyCpl3())
        }
        let sreg = SegmentRegister.Name(rawValue: reg)!
        try setSegmentRegister(sreg, SegmentSelector(rm))
        return .success(.endFetchLoop)
    }
    /// 0x8c  MOV
    func Ox8c() throws -> Result<Resume, Never> {
        modRM = fetch8()
        reg = modRM.reg
        if reg.isGeneralRegister(.ESI) || reg.isGeneralRegister(.EDI) {
            throw Interrupt(.UD)
        }
        u = DWord(segs[reg].selector)
        if modRM.mod == 3 {
            if !ipr.isFlagRaised(.operandSizeOverride) {
                regs[modRM.rM] = u
            } else {
                regs[modRM.rM].lowerHalf = u
            }
        } else {
            segmentTranslation()
            try st16WritableCpl3(word: u)
        }
        return .success(.endFetchLoop)
    }
    /// 0x86  XCHG
    func Ox86() throws -> Result<Resume, Never> {
        modRM = fetch8()
        reg = modRM.reg
        if modRM.mod == 3 {
            // LOCK prefix not allowed
            rM = modRM.rM
            rm = getEncodedByte(from: rM)
            setEncodedByte(in: rM, to: getEncodedByte(from: reg))
        } else {
            segmentTranslation()
            rm = DWord(try ld8WritableCpl3())
            try st8WritableCpl3(byte: getEncodedByte(from: reg))
        }
        setEncodedByte(in: reg, to: rm)
        return .success(.endFetchLoop)
    }
    /// 0x87  XCHG
    func Ox87() throws -> Result<Resume, Never> {
        modRM = fetch8()
        reg = modRM.reg
        if modRM.mod == 3 {
            // LOCK prefix not allowed
            rM = modRM.rM
            rm = regs[rM]
            regs[rM] = regs[reg]
        } else {
            segmentTranslation()
            rm = try ldWritableCpl3()
            try stWritableCpl3(dword: regs[reg])
        }
        regs[reg] = rm
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
        reg = opcode.encoded(.generalRegister)
        u = regs[.EAX]
        regs[.EAX] = regs[reg]
        regs[reg] = u
        return .success(.endFetchLoop)
    }
    /// 0xd7  XLAT
    func Oxd7() throws -> Result<Resume, Never> {
        lax = regs[.EBX] &+ (regs[.EAX] & 0xff)
        if ipr.isFlagRaised(.addressSizeOverride) {
            lax &= 0xffff
        }
        lax = segs[ipr.segmentRegister].shadow.base &+ lax
        m = DWord(try ld8ReadonlyCpl3())
        regs[.EAX].byteL = m
        return .success(.endFetchLoop)
    }
    /// 0xc4  LES
    func Oxc4() throws -> Result<Resume, Never> {
        try ldFarPointer(.ES)
        return .success(.endFetchLoop)
    }
    /// 0xc5  LDS
    func Oxc5() throws -> Result<Resume, Never> {
        try ldFarPointer(.DS)
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
        modRM = fetch8()
        operation = opcode.encoded(.operation)
        reg = modRM.reg
        r = getEncodedByte(from: reg)
        if modRM.mod == 3 {
            // LOCK prefix not allowed
            rM = modRM.rM
            setEncodedByte(in: rM, to: calculate8(getEncodedByte(from: rM), r))
        } else {
            segmentTranslation()
            if operation != 7 {
                rm = DWord(try ld8WritableCpl3())
                u = calculate8(rm, r)
                try st8WritableCpl3(byte: u)
            } else {
                // LOCK prefix not allowed
                rm = DWord(try ld8ReadonlyCpl3())
                calculate8(rm, r)
            }
        }
        return .success(.endFetchLoop)
    }
    /// 0x01  ADD
    func Ox01() throws -> Result<Resume, Never> {
        modRM = fetch8()
        r = regs[modRM.reg]
        if modRM.mod == 3 {
            // LOCK prefix not allowed
            rM = modRM.rM
            osmSrc = r
            regs[rM] = regs[rM] &+ r
            osmDst = regs[rM]
            osm = 2
        } else {
            segmentTranslation()
            rm = try ldWritableCpl3()
            osmSrc = r
            rm = rm &+ r
            osmDst = rm
            osm = 2
            try stWritableCpl3(dword: rm)
        }
        return .success(.endFetchLoop)
    }
    /// 0x09  OR
    /// 0x11  ADC
    /// 0x19  SBB
    /// 0x21  AND
    /// 0x29  SUB
    /// 0x31  XOR
    func Ox31() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = opcode.encoded(.operation)
        r = regs[modRM.reg]
        if modRM.mod == 3 {
            // LOCK prefix not allowed
            rM = modRM.rM
            regs[rM] = calculate(regs[rM], r)
        } else {
            segmentTranslation()
            rm = try ldWritableCpl3()
            u = calculate(rm, r)
            try stWritableCpl3(dword: u)
        }
        return .success(.endFetchLoop)
    }
    /// 0x39  CMP
    func Ox39() throws -> Result<Resume, Never> {
        modRM = fetch8()
        r = regs[modRM.reg]
        if modRM.mod == 3 {
            rM = modRM.rM
            osmSrc = r
            osmDst = regs[rM] &- r
            osm = 8
        } else {
            segmentTranslation()
            rm = try ldReadonlyCpl3()
            osmSrc = r
            osmDst = rm &- r
            osm = 8
        }
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
        modRM = fetch8()
        operation = opcode.encoded(.operation)
        reg = modRM.reg
        if modRM.mod == 3 {
            rM = modRM.rM
            rm = getEncodedByte(from: rM)
        } else {
            segmentTranslation()
            rm = DWord(try ld8ReadonlyCpl3())
        }
        setEncodedByte(in: reg, to: calculate8(getEncodedByte(from: reg), rm))
        return .success(.endFetchLoop)
    }
    /// 0x03  ADD
    func Ox03() throws -> Result<Resume, Never> {
        modRM = fetch8()
        reg = modRM.reg
        if modRM.mod == 3 {
            rm = regs[modRM.rM]
        } else {
            segmentTranslation()
            rm = try ldReadonlyCpl3()
        }
        osmSrc = rm
        regs[reg] = regs[reg] &+ rm
        osmDst = regs[reg]
        osm = 2
        return .success(.endFetchLoop)
    }
    /// 0x0b  OR
    /// 0x13  ADC
    /// 0x1b  SBB
    /// 0x23  AND
    /// 0x2b  SUB
    /// 0x33  XOR
    func Ox33() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = opcode.encoded(.operation)
        reg = modRM.reg
        if modRM.mod == 3 {
            rm = regs[modRM.rM]
        } else {
            segmentTranslation()
            rm = try ldReadonlyCpl3()
        }
        regs[reg] = calculate(regs[reg], rm)
        return .success(.endFetchLoop)
    }
    /// 0x3b  CMP
    func Ox3b() throws -> Result<Resume, Never> {
        modRM = fetch8()
        reg = modRM.reg
        if modRM.mod == 3 {
            rm = regs[modRM.rM]
        } else {
            segmentTranslation()
            rm = try ldReadonlyCpl3()
        }
        osmSrc = rm
        osmDst = regs[reg] &- rm
        osm = 8
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
        imm = DWord(fetch8())
        operation = opcode.encoded(.operation)
        regs[.EAX].byteL = calculate8(regs[.EAX] & 0xff, imm)
        return .success(.endFetchLoop)
    }
    /// 0x05  ADD
    func Ox05() throws -> Result<Resume, Never> {
        imm = fetch()
        osmSrc = imm
        regs[.EAX] = regs[.EAX] &+ imm
        osmDst = regs[.EAX]
        osm = 2
        return .success(.endFetchLoop)
    }
    /// 0x0d  OR
    /// 0x15  ADC
    /// 0x1d  SBB
    /// 0x25  AND
    /// 0x2d  SUB
    func Ox2d() throws -> Result<Resume, Never> {
        imm = fetch()
        operation = opcode.encoded(.operation)
        regs[.EAX] = calculate(regs[.EAX], imm)
        return .success(.endFetchLoop)
    }
    /// 0x35  XOR
    func Ox35() throws -> Result<Resume, Never> {
        imm = fetch()
        regs[.EAX] = regs[.EAX] ^ imm
        osmDst = regs[.EAX]
        osm = 14
        return .success(.endFetchLoop)
    }
    /// 0x3d  CMP
    func Ox3d() throws -> Result<Resume, Never> {
        imm = fetch()
        osmSrc = imm
        osmDst = regs[.EAX] &- imm
        osm = 8
        return .success(.endFetchLoop)
    }
    /// 0x80  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    /// 0x82  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    func Ox82() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        if modRM.mod == 3 {
            // LOCK prefix not allowed
            rM = modRM.rM
            imm = DWord(fetch8())
            setEncodedByte(in: rM, to: calculate8(getEncodedByte(from: rM), imm))
        } else {
            segmentTranslation()
            imm = DWord(fetch8())
            if operation != 7 {
                rm = DWord(try ld8WritableCpl3())
                u = calculate8(rm, imm)
                try st8WritableCpl3(byte: u)
            } else {
                // LOCK prefix not allowed
                rm = DWord(try ld8ReadonlyCpl3())
                calculate8(rm, imm)
            }
        }
        return .success(.endFetchLoop)
    }
    /// 0x81  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    func Ox81() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        if operation == 7 {
            // LOCK prefix not allowed
            if modRM.mod == 3 {
                rm = regs[modRM.rM]
            } else {
                segmentTranslation()
                rm = try ldReadonlyCpl3()
            }
            imm = fetch()
            osmSrc = imm
            osmDst = rm &- imm
            osm = 8
        } else {
            if modRM.mod == 3 {
                // LOCK prefix not allowed
                rM = modRM.rM
                imm = fetch()
                regs[rM] = calculate(regs[rM], imm)
            } else {
                segmentTranslation()
                imm = fetch()
                rm = try ldWritableCpl3()
                u = calculate(rm, imm)
                try stWritableCpl3(dword: u)
            }
        }
        return .success(.endFetchLoop)
    }
    /// 0x83  G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, CMP)
    func Ox83() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        if operation == 7 {
            // LOCK prefix not allowed
            if modRM.mod == 3 {
                rm = regs[modRM.rM]
            } else {
                segmentTranslation()
                rm = try ldReadonlyCpl3()
            }
            u = DWord(fetch8()).signExtendedByte
            osmSrc = u
            osmDst = rm &- u
            osm = 8
        } else {
            if modRM.mod == 3 {
                // LOCK prefix not allowed
                rM = modRM.rM
                u = DWord(fetch8()).signExtendedByte
                regs[rM] = calculate(regs[rM], u)
            } else {
                segmentTranslation()
                u = DWord(fetch8()).signExtendedByte
                rm = try ldWritableCpl3()
                v = calculate(rm, u)
                try stWritableCpl3(dword: v)
            }
        }
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
        reg = opcode.encoded(.generalRegister)
        if osm < 25 {
            osmPreserved = osm
            osmDstPreserved = osmDst
        }
        regs[reg] = regs[reg] &+ 1
        osmDst = regs[reg]
        osm = 27
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
        reg = opcode.encoded(.generalRegister)
        if osm < 25 {
            osmPreserved = osm
            osmDstPreserved = osmDst
        }
        regs[reg] = regs[reg] &- 1
        osmDst = regs[reg]
        osm = 30
        return .success(.endFetchLoop)
    }
    /// 0x69  IMUL
    func Ox69() throws -> Result<Resume, Never> {
        modRM = fetch8()
        reg = modRM.reg
        if modRM.mod == 3 {
            rm = regs[modRM.rM]
        } else {
            segmentTranslation()
            rm = try ldReadonlyCpl3()
        }
        imm = fetch()
        auxImul(rm, imm)
        regs[reg] = u
        return .success(.endFetchLoop)
    }
    /// 0x6b  IMUL
    func Ox6b() throws -> Result<Resume, Never> {
        modRM = fetch8()
        reg = modRM.reg
        if modRM.mod == 3 {
            rm = regs[modRM.rM]
        } else {
            segmentTranslation()
            rm = try ldReadonlyCpl3()
        }
        v = DWord(fetch8()).signExtendedByte
        auxImul(rm, v)
        regs[reg] = u
        return .success(.endFetchLoop)
    }
    /// 0x84  TEST
    func Ox84() throws -> Result<Resume, Never> {
        modRM = fetch8()
        if modRM.mod == 3 {
            rM = modRM.rM
            rm = getEncodedByte(from: rM)
        } else {
            segmentTranslation()
            rm = DWord(try ld8ReadonlyCpl3())
        }
        reg = modRM.reg
        r = getEncodedByte(from: reg)
        osmDst = (rm & r).signExtendedByte
        osm = 12
        return .success(.endFetchLoop)
    }
    /// 0x85  TEST
    func Ox85() throws -> Result<Resume, Never> {
        modRM = fetch8()
        if modRM.mod == 3 {
            rm = regs[modRM.rM]
        } else {
            segmentTranslation()
            rm = try ldReadonlyCpl3()
        }
        r = regs[modRM.reg]
        osmDst = rm & r
        osm = 14
        return .success(.endFetchLoop)
    }
    /// 0xa8  TEST
    func Oxa8() throws -> Result<Resume, Never> {
        imm = DWord(fetch8())
        osmDst = (regs[.EAX] & imm).signExtendedByte
        osm = 12
        return .success(.endFetchLoop)
    }
    /// 0xa9  TEST
    func Oxa9() throws -> Result<Resume, Never> {
        imm = fetch()
        osmDst = regs[.EAX] & imm
        osm = 14
        return .success(.endFetchLoop)
    }
    /// 0xf6  G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
    func Oxf6() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        // LOCK prefix not allowed if operation != 2 && operation != 3
        switch operation {
        case 0:  // TEST
            if modRM.mod == 3 {
                rM = modRM.rM
                rm = getEncodedByte(from: rM)
            } else {
                segmentTranslation()
                rm = DWord(try ld8ReadonlyCpl3())
            }
            imm = DWord(fetch8())
            osmDst = (rm & imm).signExtendedByte
            osm = 12
            break
        case 2:  // NOT
            if modRM.mod == 3 {
                // LOCK prefix not allowed
                rM = modRM.rM
                setEncodedByte(in: rM, to: ~(getEncodedByte(from: rM)))
            } else {
                segmentTranslation()
                rm = DWord(try ld8WritableCpl3())
                try st8WritableCpl3(byte: ~rm)
            }
            break
        case 3:  // NEG
            operation = 5
            if modRM.mod == 3 {
                // LOCK prefix not allowed
                rM = modRM.rM
                setEncodedByte(in: rM, to: calculate8(0, getEncodedByte(from: rM)))
            } else {
                segmentTranslation()
                rm = DWord(try ld8WritableCpl3())
                u = calculate8(0, rm)
                try st8WritableCpl3(byte: u)
            }
            break
        case 4:  // MUL AL/X
            if modRM.mod == 3 {
                rM = modRM.rM
                rm = getEncodedByte(from: rM)
            } else {
                segmentTranslation()
                rm = DWord(try ld8ReadonlyCpl3())
            }
            aux8Mul(regs[.EAX], rm)
            regs[.EAX].lowerHalf = u
            break
        case 5:  // IMUL AL/X
            if modRM.mod == 3 {
                rM = modRM.rM
                rm = getEncodedByte(from: rM)
            } else {
                segmentTranslation()
                rm = DWord(try ld8ReadonlyCpl3())
            }
            aux8Imul(regs[.EAX], rm)
            regs[.EAX].lowerHalf = u
            break
        case 6:  // DIV AL/X
            if modRM.mod == 3 {
                rM = modRM.rM
                rm = getEncodedByte(from: rM)
            } else {
                segmentTranslation()
                rm = DWord(try ld8ReadonlyCpl3())
            }
            try aux8Div(rm)
            break
        case 7:  // IDIV AL/X
            if modRM.mod == 3 {
                rM = modRM.rM
                rm = getEncodedByte(from: rM)
            } else {
                segmentTranslation()
                rm = DWord(try ld8ReadonlyCpl3())
            }
            try aux8Idiv(rm)
            break
        default:
            throw Interrupt(.UD)
        }
        return .success(.endFetchLoop)
    }
    /// 0xf7  G3 (TEST, -, NOT, NEG, MUL AL/X, IMUL AL/X, DIV AL/X, IDIV AL/X)
    func Oxf7() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        // LOCK prefix not allowed if operation != 2 && operation != 3
        switch operation {
        case 0:  // TEST
            if modRM.mod == 3 {
                rm = regs[modRM.rM]
            } else {
                segmentTranslation()
                rm = try ldReadonlyCpl3()
            }
            imm = fetch()
            osmDst = rm & imm
            osm = 14
            break
        case 2:  // NOT
            if modRM.mod == 3 {
                // LOCK prefix not allowed
                rM = modRM.rM
                regs[rM] = ~regs[rM]
            } else {
                segmentTranslation()
                rm = try ldWritableCpl3()
                try stWritableCpl3(dword: ~rm)
            }
            break
        case 3:  // NEG
            operation = 5
            if modRM.mod == 3 {
                // LOCK prefix not allowed
                rM = modRM.rM
                regs[rM] = calculate(0, regs[rM])
            } else {
                segmentTranslation()
                rm = try ldWritableCpl3()
                u = calculate(0, rm)
                try stWritableCpl3(dword: u)
            }
            break
        case 4:  // MUL AL/X
            if modRM.mod == 3 {
                rm = regs[modRM.rM]
            } else {
                segmentTranslation()
                rm = try ldReadonlyCpl3()
            }
            auxMul(regs[.EAX], rm)
            regs[.EAX] = u
            regs[.EDX] = v
            break
        case 5:  // IMUL AL/X
            if modRM.mod == 3 {
                rm = regs[modRM.rM]
            } else {
                segmentTranslation()
                rm = try ldReadonlyCpl3()
            }
            auxImul(regs[.EAX], rm)
            regs[.EAX] = u
            regs[.EDX] = v
            break
        case 6:  // DIV AL/X
            if modRM.mod == 3 {
                rm = regs[modRM.rM]
            } else {
                segmentTranslation()
                rm = try ldReadonlyCpl3()
            }
            try auxDiv(QWord(regs[.EAX]) | QWord(regs[.EDX]) << 32, rm)
            regs[.EAX] = u
            regs[.EDX] = v
            break
        case 7:  // IDIV AL/X
            if modRM.mod == 3 {
                rm = regs[modRM.rM]
            } else {
                segmentTranslation()
                rm = try ldReadonlyCpl3()
            }
            try auxIdiv(QWord(regs[.EAX]) | QWord(regs[.EDX]) << 32, rm)
            regs[.EAX] = u
            regs[.EDX] = v
            break
        default:
            throw Interrupt(.UD)
        }
        return .success(.endFetchLoop)
    }
    /// 0xc0  G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
    func Oxc0() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        if modRM.mod == 3 {
            imm = DWord(fetch8())
            rM = modRM.rM
            setEncodedByte(in: rM, to: shift8(getEncodedByte(from: rM), imm))
        } else {
            segmentTranslation()
            imm = DWord(fetch8())
            rm = DWord(try ld8WritableCpl3())
            u = shift8(rm, imm)
            try st8WritableCpl3(byte: u)
        }
        return .success(.endFetchLoop)
    }
    /// 0xc1  G2 (ROL ROR RCL RCR SHL SHR SAL SAR)
    func Oxc1() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        if modRM.mod == 3 {
            imm = DWord(fetch8())
            rM = modRM.rM
            regs[rM] = shift(regs[rM], imm)
        } else {
            segmentTranslation()
            imm = DWord(fetch8())
            rm = try ldWritableCpl3()
            u = shift(rm, imm)
            try stWritableCpl3(dword: u)
        }
        return .success(.endFetchLoop)
    }
    /// 0xd0  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
    func Oxd0() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        if modRM.mod == 3 {
            rM = modRM.rM
            setEncodedByte(in: rM, to: shift8(getEncodedByte(from: rM), 1))
        } else {
            segmentTranslation()
            rm = DWord(try ld8WritableCpl3())
            u = shift8(rm, 1)
            try st8WritableCpl3(byte: u)
        }
        return .success(.endFetchLoop)
    }
    /// 0xd1  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),1
    func Oxd1() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        if modRM.mod == 3 {
            rM = modRM.rM
            regs[rM] = shift(regs[rM], 1)
        } else {
            segmentTranslation()
            rm = try ldWritableCpl3()
            u = shift(rm, 1)
            try stWritableCpl3(dword: u)
        }
        return .success(.endFetchLoop)
    }
    /// 0xd2  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
    func Oxd2() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        if modRM.mod == 3 {
            rM = modRM.rM
            setEncodedByte(in: rM, to: shift8(getEncodedByte(from: rM), regs[.ECX] & 0xff))
        } else {
            segmentTranslation()
            rm = DWord(try ld8WritableCpl3())
            u = shift8(rm, regs[.ECX] & 0xff)
            try st8WritableCpl3(byte: u)
        }
        return .success(.endFetchLoop)
    }
    /// 0xd3  G2 (ROL ROR RCL RCR SHL SHR SAL SAR),CL
    func Oxd3() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        if modRM.mod == 3 {
            rM = modRM.rM
            regs[rM] = shift(regs[rM], regs[.ECX] & 0xff)
        } else {
            segmentTranslation()
            rm = try ldWritableCpl3()
            u = shift(rm, regs[.ECX] & 0xff)
            try stWritableCpl3(dword: u)
        }
        return .success(.endFetchLoop)
    }
    /// 0x98  CBW
    func Ox98() throws -> Result<Resume, Never> {
        regs[.EAX] = regs[.EAX].signExtendedWord
        return .success(.endFetchLoop)
    }
    /// 0x99  CWD
    func Ox99() throws -> Result<Resume, Never> {
        regs[.EDX] = regs[.EAX].signedShiftRight(count: 31)
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
        reg = opcode.encoded(.generalRegister)
        r = regs[reg]
        if x8664LongMode {
            lax = regs[.ESP] &- 4
            try stWritableCpl3(dword: r)
            regs[.ESP] = lax
        } else {
            try push(r)
        }
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
        if x8664LongMode {
            lax = regs[.ESP]
            m = try ldReadonlyCpl3()
            regs[.ESP] = lax &+ 4
        } else {
            m = try pop()
        }
        reg = opcode.encoded(.generalRegister)
        regs[reg] = m
        return .success(.endFetchLoop)
    }
    /// 0x60  PUSHA
    func Ox60() throws -> Result<Resume, Never> {
        try auxPusha()
        return .success(.endFetchLoop)
    }
    /// 0x61  POPA
    func Ox61() throws -> Result<Resume, Never> {
        try auxPopa()
        return .success(.endFetchLoop)
    }
    /// 0x8f  POP
    func Ox8f() throws -> Result<Resume, Never> {
        modRM = fetch8()
        if modRM.mod == 3 {
            m = try pop()
            regs[modRM.rM] = m
        } else {
            u = regs[.ESP]
            m = try pop()
            v = regs[.ESP]
            segmentTranslation()
            regs[.ESP] = u
            try stWritableCpl3(dword: m)
            regs[.ESP] = v
        }
        return .success(.endFetchLoop)
    }
    /// 0x68  PUSH
    func Ox68() throws -> Result<Resume, Never> {
        imm = fetch()
        if x8664LongMode {
            lax = regs[.ESP] &- 4
            try stWritableCpl3(dword: imm)
            regs[.ESP] = lax
        } else {
            try push(imm)
        }
        return .success(.endFetchLoop)
    }
    /// 0x6a  PUSH
    func Ox6a() throws -> Result<Resume, Never> {
        u = DWord(fetch8()).signExtendedByte
        if x8664LongMode {
            lax = regs[.ESP] &- 4
            try stWritableCpl3(dword: u)
            regs[.ESP] = lax
        } else {
            try push(u)
        }
        return .success(.endFetchLoop)
    }
    /// 0xc8  ENTER
    func Oxc8() throws -> Result<Resume, Never> {
        try auxEnter()
        return .success(.endFetchLoop)
    }
    /// 0xc9  LEAVE
    func Oxc9() throws -> Result<Resume, Never> {
        if x8664LongMode {
            lax = regs[.EBP]
            regs[.EBP] = try ldReadonlyCpl3()
            regs[.ESP] = lax &+ 4
        } else {
            try auxLeave()
        }
        return .success(.endFetchLoop)
    }
    /// 0x9c  PUSHF
    func Ox9c() throws -> Result<Resume, Never> {
        if eflags.isFlagRaised(.VM) && eflags.iopl != 3 {
            throw Interrupt(.GP, errorCode: 0)
        }
        var mask: Eflags = 0
        mask |= Eflags.flagMask(for: .RF)
        mask |= Eflags.flagMask(for: .VM)
        u = getEflags() & ~mask
        if !ipr.isFlagRaised(.operandSizeOverride) {
            try push(u)
        } else {
            try push16(u)
        }
        return .success(.endFetchLoop)
    }
    /// 0x9d  POPF
    func Ox9d() throws -> Result<Resume, Never> {
        if eflags.isFlagRaised(.VM) && eflags.iopl != 3 {
            throw Interrupt(.GP, errorCode: 0)
        }
        if !ipr.isFlagRaised(.operandSizeOverride) {
            m = try pop()
            u = 0xffffffff
        } else {
            m = DWord(try pop16())
            u = 0xffff
        }
        var mask: Eflags = 0
        mask.setFlag(.TF)
        mask.setFlag(.NT)
        mask.setFlag(.AC)
        mask.setFlag(.ID)
        if cpl == 0 {
            mask.setFlag(.IF)
            mask.iopl = 3
        } else {
            if cpl <= eflags.iopl {
                mask.setFlag(.IF)
            }
        }
        setEflags(m, u & mask)
        return .success(.endOnInterrupt)
    }
    /// 0x06  PUSH
    /// 0x0e  PUSH
    /// 0x16  PUSH
    /// 0x1e  PUSH
    func Ox1e() throws -> Result<Resume, Never> {
        let sreg = opcode.encoded(.standardSegmentRegister)
        try push(segs[sreg].selector)
        return .success(.endFetchLoop)
    }
    /// 0x07  POP
    /// 0x17  POP
    /// 0x1f  POP
    func Ox1f() throws -> Result<Resume, Never> {
        m = try pop()
        let sreg = SegmentRegister.Name(rawValue: opcode.encoded(.standardSegmentRegister))!
        try setSegmentRegister(sreg, SegmentSelector(truncatingIfNeeded: m))
        return .success(.endFetchLoop)
    }
    /// 0x8d  LEA
    func Ox8d() throws -> Result<Resume, Never> {
        modRM = fetch8()
        if modRM.mod == 3 {
            throw Interrupt(.UD)
        }
        ipr.segmentRegister = SegmentRegister.Name.LDT.rawValue
        segmentTranslation()
        regs[modRM.reg] = lax
        return .success(.endFetchLoop)
    }
    /// 0xfe  G4 (INC, DEC, -, -, -, -, -, -)
    func Oxfe() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        switch operation {
        case 0:  // INC
            if modRM.mod == 3 {
                // LOCK prefix not allowed
                rM = modRM.rM
                setEncodedByte(in: rM, to: aux8Inc(getEncodedByte(from: rM)))
            } else {
                segmentTranslation()
                rm = DWord(try ld8WritableCpl3())
                u = aux8Inc(rm)
                try st8WritableCpl3(byte: u)
            }
            break
        case 1:  // DEC
            if modRM.mod == 3 {
                // LOCK prefix not allowed
                rM = modRM.rM
                setEncodedByte(in: rM, to: aux8Dec(getEncodedByte(from: rM)))
            } else {
                segmentTranslation()
                rm = DWord(try ld8WritableCpl3())
                u = aux8Dec(rm)
                try st8WritableCpl3(byte: u)
            }
            break
        default:
            throw Interrupt(.UD)
        }
        return .success(.endFetchLoop)
    }
    /// 0xff  G5 (INC, DEC, CALL, CALL, JMP, JMP, PUSH, -)
    func Oxff() throws -> Result<Resume, Never> {
        modRM = fetch8()
        operation = modRM.opcode
        // LOCK prefix not allowed if operation != 0 && operation != 1
        switch operation {
        case 0:  // INC
            if modRM.mod == 3 {
                // LOCK prefix not allowed
                rM = modRM.rM
                if osm < 25 {
                    osmPreserved = osm
                    osmDstPreserved = osmDst
                }
                regs[rM] = regs[rM] &+ 1
                osmDst = regs[rM]
                osm = 27
            } else {
                segmentTranslation()
                rm = try ldWritableCpl3()
                if osm < 25 {
                    osmPreserved = osm
                    osmDstPreserved = osmDst
                }
                rm = rm &+ 1
                osmDst = rm
                osm = 27
                try stWritableCpl3(dword: rm)
            }
            break
        case 1:  // DEC
            if modRM.mod == 3 {
                // LOCK prefix not allowed
                rM = modRM.rM
                if osm < 25 {
                    osmPreserved = osm
                    osmDstPreserved = osmDst
                }
                regs[rM] = regs[rM] &- 1
                osmDst = regs[rM]
                osm = 30
            } else {
                segmentTranslation()
                rm = try ldWritableCpl3()
                if osm < 25 {
                    osmPreserved = osm
                    osmDstPreserved = osmDst
                }
                rm = rm &- 1
                osmDst = rm
                osm = 30
                try stWritableCpl3(dword: rm)
            }
            break
        case 2:  // CALL
            if modRM.mod == 3 {
                rm = regs[modRM.rM]
            } else {
                segmentTranslation()
                rm = try ldReadonlyCpl3()
            }
            u = eip &+ far &- farStart
            if x8664LongMode {
                lax = regs[.ESP] &- 4
                try stWritableCpl3(dword: u)
                regs[.ESP] = lax
            } else {
                try push(u)
            }
            eip = rm
            far = 0
            farStart = 0
            break
        case 3:  // CALLF
            fallthrough
        case 5:  // JMPF
            if modRM.mod == 3 {
                throw Interrupt(.UD)
            }
            segmentTranslation()
            m = try ldReadonlyCpl3()
            lax = lax &+ 4
            m16 = try ld16ReadonlyCpl3()
            if operation == 3 {
                try auxCallf(true, m16, m, eip &+ far &- farStart)
            } else {
                try auxJmpf(m16, m)
            }
            break
        case 4:  // JMP
            if modRM.mod == 3 {
                rm = regs[modRM.rM]
            } else {
                segmentTranslation()
                rm = try ldReadonlyCpl3()
            }
            eip = rm
            far = 0
            farStart = 0
            break
        case 6:  // PUSH
            if modRM.mod == 3 {
                rm = regs[modRM.rM]
            } else {
                segmentTranslation()
                rm = try ldReadonlyCpl3()
            }
            if x8664LongMode {
                lax = regs[.ESP] &- 4
                try stWritableCpl3(dword: rm)
                regs[.ESP] = lax
            } else {
                try push(rm)
            }
            break
        default:
            throw Interrupt(.UD)
        }
        return .success(.endFetchLoop)
    }
    /// 0xeb  JMP
    func Oxeb() throws -> Result<Resume, Never> {
        u = DWord(fetch8()).signExtendedByte
        far = far &+ u
        return .success(.endFetchLoop)
    }
    /// 0xe9  JMP
    func Oxe9() throws -> Result<Resume, Never> {
        imm = fetch()
        far = far &+ imm
        return .success(.endFetchLoop)
    }
    /// 0xea  JMPF
    func Oxea() throws -> Result<Resume, Never> {
        if !ipr.isFlagRaised(.operandSizeOverride) {
            imm = fetch()
        } else {
            imm = DWord(fetch16())
        }
        imm16 = fetch16()
        try auxJmpf(imm16, imm)
        return .success(.endFetchLoop)
    }
    /// 0x70  JO
    func Ox70() throws -> Result<Resume, Never> {
        if isOF() {
            u = DWord(fetch8()).signExtendedByte
            far = far &+ u
        } else {
            far = far &+ 1
        }
        return .success(.endFetchLoop)
    }
    /// 0x71  JNO
    func Ox71() throws -> Result<Resume, Never> {
        if !isOF() {
            u = DWord(fetch8()).signExtendedByte
            far = far &+ u
        } else {
            far = far &+ 1
        }
        return .success(.endFetchLoop)
    }
    /// 0x72  JB
    func Ox72() throws -> Result<Resume, Never> {
        if isCF() {
            u = DWord(fetch8()).signExtendedByte
            far = far &+ u
        } else {
            far = far &+ 1
        }
        return .success(.endFetchLoop)
    }
    /// 0x73  JNB
    func Ox73() throws -> Result<Resume, Never> {
        if !isCF() {
            u = DWord(fetch8()).signExtendedByte
            far = far &+ u
        } else {
            far = far &+ 1
        }
        return .success(.endFetchLoop)
    }
    /// 0x74  JZ
    func Ox74() throws -> Result<Resume, Never> {
        if osmDst == 0 {
            u = DWord(fetch8()).signExtendedByte
            far = far &+ u
        } else {
            far = far &+ 1
        }
        return .success(.endFetchLoop)
    }
    /// 0x75  JNZ
    func Ox75() throws -> Result<Resume, Never> {
        if !(osmDst == 0) {
            u = DWord(fetch8()).signExtendedByte
            far = far &+ u
        } else {
            far = far &+ 1
        }
        return .success(.endFetchLoop)
    }
    /// 0x76  JBE
    func Ox76() throws -> Result<Resume, Never> {
        if isBE() {
            u = DWord(fetch8()).signExtendedByte
            far = far &+ u
        } else {
            far = far &+ 1
        }
        return .success(.endFetchLoop)
    }
    /// 0x77  JNBE
    func Ox77() throws -> Result<Resume, Never> {
        if !isBE() {
            u = DWord(fetch8()).signExtendedByte
            far = far &+ u
        } else {
            far = far &+ 1
        }
        return .success(.endFetchLoop)
    }
    /// 0x78  JS
    func Ox78() throws -> Result<Resume, Never> {
        if (osm == 24 ? (osmSrc >> 7) & 1 : (osmDst >> 31) & 1) != 0 {
            u = DWord(fetch8()).signExtendedByte
            far = far &+ u
        } else {
            far = far &+ 1
        }
        return .success(.endFetchLoop)
    }
    /// 0x79  JNS
    func Ox79() throws -> Result<Resume, Never> {
        if (osm == 24 ? (osmSrc >> 7) & 1 : (osmDst >> 31) & 1) == 0 {
            u = DWord(fetch8()).signExtendedByte
            far = far &+ u
        } else {
            far = far &+ 1
        }
        return .success(.endFetchLoop)
    }
    /// 0x7a  JP
    func Ox7a() throws -> Result<Resume, Never> {
        if isPF() {
            u = DWord(fetch8()).signExtendedByte
            far = far &+ u
        } else {
            far = far &+ 1
        }
        return .success(.endFetchLoop)
    }
    /// 0x7b  JNP
    func Ox7b() throws -> Result<Resume, Never> {
        if !isPF() {
            u = DWord(fetch8()).signExtendedByte
            far = far &+ u
        } else {
            far = far &+ 1
        }
        return .success(.endFetchLoop)
    }
    /// 0x7c  JL
    func Ox7c() throws -> Result<Resume, Never> {
        if isLT() {
            u = DWord(fetch8()).signExtendedByte
            far = far &+ u
        } else {
            far = far &+ 1
        }
        return .success(.endFetchLoop)
    }
    /// 0x7d  JNL
    func Ox7d() throws -> Result<Resume, Never> {
        if !isLT() {
            u = DWord(fetch8()).signExtendedByte
            far = far &+ u
        } else {
            far = far &+ 1
        }
        return .success(.endFetchLoop)
    }
    /// 0x7e  JLE
    func Ox7e() throws -> Result<Resume, Never> {
        if isLE() {
            u = DWord(fetch8()).signExtendedByte
            far = far &+ u
        } else {
            far = far &+ 1
        }
        return .success(.endFetchLoop)
    }
    /// 0x7f  JNLE
    func Ox7f() throws -> Result<Resume, Never> {
        if !isLE() {
            u = DWord(fetch8()).signExtendedByte
            far = far &+ u
        } else {
            far = far &+ 1
        }
        return .success(.endFetchLoop)
    }
    /// 0xe0  LOOPNE
    /// 0xe1  LOOPE
    /// 0xe2  LOOP
    func Oxe2() throws -> Result<Resume, Never> {
        w = DWord(fetch8()).signExtendedByte
        u = (regs[.ECX] &- 1) & ipr.addressSizeMask
        regs[.ECX] = (regs[.ECX] & ~ipr.addressSizeMask) | u
        let b: Bool
        switch opcode & 3 {
        case 0:
            b = osmDst != 0
            break
        case 1:
            b = osmDst == 0
            break
        default:
            b = true
            break
        }
        if (u != 0) && b {
            if ipr.isFlagRaised(.operandSizeOverride) {
                eip = (eip &+ far &- farStart &+ w).lowerHalf
                far = 0
                farStart = 0
            } else {
                far = far &+ w
            }
        }
        return .success(.endFetchLoop)
    }
    /// 0xe3  JCXZ
    func Oxe3() throws -> Result<Resume, Never> {
        u = DWord(fetch8()).signExtendedByte
        if (regs[.ECX] & ipr.addressSizeMask) == 0 {
            if ipr.isFlagRaised(.operandSizeOverride) {
                eip = (eip &+ far &- farStart &+ u).lowerHalf
                far = 0
                farStart = 0
            } else {
                far = far &+ u
            }
        }
        return .success(.endFetchLoop)
    }
    /// 0xc2  RET
    func Oxc2() throws -> Result<Resume, Never> {
        u = DWord(fetch16()).signExtendedWord
        m = try ldStack()
        regs[.ESP] = (regs[.ESP] & ~ssMask) | ((regs[.ESP] &+ 4 &+ u) & ssMask)
        eip = m
        far = 0
        farStart = 0
        return .success(.endFetchLoop)
    }
    /// 0xc3  RET
    func Oxc3() throws -> Result<Resume, Never> {
        if x8664LongMode {
            lax = regs[.ESP]
            m = try ldReadonlyCpl3()
            regs[.ESP] = regs[.ESP] &+ 4
        } else {
            m = try pop()
        }
        eip = m
        far = 0
        farStart = 0
        return .success(.endFetchLoop)
    }
    /// 0xe8  CALL
    func Oxe8() throws -> Result<Resume, Never> {
        imm = fetch()
        u = eip &+ far &- farStart
        if x8664LongMode {
            lax = regs[.ESP] &- 4
            try stWritableCpl3(dword: u)
            regs[.ESP] = lax
        } else {
            try push(u)
        }
        far = far &+ imm
        return .success(.endFetchLoop)
    }
    /// 0x9a  CALLF
    func Ox9a() throws -> Result<Resume, Never> {
        let o32 = !ipr.isFlagRaised(.operandSizeOverride)
        if o32 {
            imm = fetch()
        } else {
            imm = DWord(fetch16())
        }
        imm16 = fetch16()
        try auxCallf(o32, imm16, imm, eip &+ far &- farStart)
        return .success(.endOnInterrupt)
    }
    /// 0xca  RET
    func Oxca() throws -> Result<Resume, Never> {
        u = DWord(fetch16()).signExtendedWord
        try auxRetf(!ipr.isFlagRaised(.operandSizeOverride), u)
        return .success(.endOnInterrupt)
    }
    /// 0xcb  RET
    func Oxcb() throws -> Result<Resume, Never> {
        try auxRetf(!ipr.isFlagRaised(.operandSizeOverride), 0)
        return .success(.endOnInterrupt)
    }
    /// 0xcf  IRET
    func Oxcf() throws -> Result<Resume, Never> {
        try auxIret(!ipr.isFlagRaised(.operandSizeOverride))
        return .success(.endOnInterrupt)
    }
    /// 0x90  NOP
    func Ox90() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xcc  INT
    func Oxcc() throws -> Result<Resume, Never> {
        u = eip &+ far &- farStart
        try raiseInterrupt(3, 0, true, u)
        return .success(.endFetchLoop)
    }
    /// 0xcd  INT
    func Oxcd() throws -> Result<Resume, Never> {
        imm = DWord(fetch8())
        if eflags.isFlagRaised(.VM) && eflags.iopl != 3 {
            throw Interrupt(.GP, errorCode: 0)
        }
        u = eip &+ far &- farStart
        try raiseInterrupt(Byte(imm), 0, true, u)
        return .success(.endFetchLoop)
    }
    /// 0xce  INTO
    func Oxce() throws -> Result<Resume, Never> {
        if isOF() {
            u = eip &+ far &- farStart
            try raiseInterrupt(4, 0, true, u)
        }
        return .success(.endFetchLoop)
    }
    /// 0x62  BOUND
    func Ox62() throws -> Result<Resume, Never> {
        try auxBound()
        return .success(.endFetchLoop)
    }
    /// 0xf5  CMC
    func Oxf5() throws -> Result<Resume, Never> {
        osmSrc = compileEflags() ^ 0x0001
        osmDst = ((osmSrc >> 6) & 1) ^ 1
        osm = 24
        return .success(.endFetchLoop)
    }
    /// 0xf8  CLC
    func Oxf8() throws -> Result<Resume, Never> {
        osmSrc = compileEflags() & ~0x0001
        osmDst = ((osmSrc >> 6) & 1) ^ 1
        osm = 24
        return .success(.endFetchLoop)
    }
    /// 0xf9  STC
    func Oxf9() throws -> Result<Resume, Never> {
        osmSrc = compileEflags() | 0x0001
        osmDst = ((osmSrc >> 6) & 1) ^ 1
        osm = 24
        return .success(.endFetchLoop)
    }
    /// 0xfc  CLD
    func Oxfc() throws -> Result<Resume, Never> {
        df = 1
        return .success(.endFetchLoop)
    }
    /// 0xfd  STD
    func Oxfd() throws -> Result<Resume, Never> {
        df = -1
        return .success(.endFetchLoop)
    }
    /// 0xfa  CLI
    func Oxfa() throws -> Result<Resume, Never> {
        if cpl > eflags.iopl {
            throw Interrupt(.GP, errorCode: 0)
        }
        eflags.setFlag(.IF, .zero)
        return .success(.endFetchLoop)
    }
    /// 0xfb  STI
    func Oxfb() throws -> Result<Resume, Never> {
        if cpl > eflags.iopl {
            throw Interrupt(.GP, errorCode: 0)
        }
        eflags.setFlag(.IF)
        return .success(.endOnInterrupt)
    }
    /// 0x9e  SAHF
    func Ox9e() throws -> Result<Resume, Never> {
        var mask: Eflags = 0
        mask.setFlag(.CF)
        mask.setFlag(.PF)
        mask.setFlag(.AF)
        mask.setFlag(.ZF)
        mask.setFlag(.SF)
        osmSrc = ((regs[.EAX] >> 8) & mask) | ((isOF() ? 1 : 0) << 11)
        osmDst = ((osmSrc >> 6) & 1) ^ 1
        osm = 24
        return .success(.endFetchLoop)
    }
    /// 0x9f  LAHF
    func Ox9f() throws -> Result<Resume, Never> {
        u = getEflags()
        regs[.EAX].byteH = u
        return .success(.endFetchLoop)
    }
    /// 0xf4  HLT
    func Oxf4() throws -> Result<Resume, Never> {
        if cpl != 0 {
            throw Interrupt(.GP, errorCode: 0)
        }
        halted = true
        return .success(.endCyclesLoop)
    }
    /// 0xa4  MOVSB
    func Oxa4() throws -> Result<Resume, Never> {
        try auxMovsb()
        return .success(.endFetchLoop)
    }
    /// 0xa5  MOVSW/D
    func Oxa5() throws -> Result<Resume, Never> {
        ipr.isFlagRaised(.operandSizeOverride) ? try auxMovsw() : try auxMovsd()
        return .success(.endFetchLoop)
    }
    /// 0xaa  STOSB
    func Oxaa() throws -> Result<Resume, Never> {
        try auxStosb()
        return .success(.endFetchLoop)
    }
    /// 0xab  STOSW/D
    func Oxab() throws -> Result<Resume, Never> {
        ipr.isFlagRaised(.operandSizeOverride) ? try auxStosw() : try auxStosd()
        return .success(.endFetchLoop)
    }
    /// 0xa6  CMPSB
    func Oxa6() throws -> Result<Resume, Never> {
        try auxCmpsb()
        return .success(.endFetchLoop)
    }
    /// 0xa7  CMPSW/D
    func Oxa7() throws -> Result<Resume, Never> {
        ipr.isFlagRaised(.operandSizeOverride) ? try auxCmpsw() : try auxCmpsd()
        return .success(.endFetchLoop)
    }
    /// 0xac  LOSB
    func Oxac() throws -> Result<Resume, Never> {
        try auxLodsb()
        return .success(.endFetchLoop)
    }
    /// 0xad  LOSW/D
    func Oxad() throws -> Result<Resume, Never> {
        ipr.isFlagRaised(.operandSizeOverride) ? try auxLodsw() : try auxLodsd()
        return .success(.endFetchLoop)
    }
    /// 0xae  SCASB
    func Oxae() throws -> Result<Resume, Never> {
        try auxScasb()
        return .success(.endFetchLoop)
    }
    /// 0xaf  SCASW/D
    func Oxaf() throws -> Result<Resume, Never> {
        ipr.isFlagRaised(.operandSizeOverride) ? try auxScasw() : try auxScasd()
        return .success(.endFetchLoop)
    }
    /// 0x6c  INSB
    func Ox6c() throws -> Result<Resume, Never> {
        try auxInsb()
        return .success(.endOnInterrupt)
    }
    /// 0x6d  INSW/D
    func Ox6d() throws -> Result<Resume, Never> {
        ipr.isFlagRaised(.operandSizeOverride) ? try auxInsw() : try auxInsd()
        return .success(.endOnInterrupt)
    }
    /// 0x6e  OUTSB
    func Ox6e() throws -> Result<Resume, Never> {
        try auxOutsb()
        return .success(.endOnInterrupt)
    }
    /// 0x6f  OUTSW/D
    func Ox6f() throws -> Result<Resume, Never> {
        ipr.isFlagRaised(.operandSizeOverride) ? try auxOutsw() : try auxOutsd()
        return .success(.endOnInterrupt)
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
        if cr0.isFlagRaised(.EM) || cr0.isFlagRaised(.TS) {
            throw Interrupt(.NM)
        }
        modRM = fetch8()
        operation = (opcode.encoded(.coProcessorOpcode) << 3) | modRM.opcode
        regs[.EAX].lowerHalf = 0xffff
        if modRM.mod == 3 {
        } else {
            segmentTranslation()
        }
        return .success(.endFetchLoop)
    }
    /// 0x9b  FWAIT/WAIT
    func Ox9b() throws -> Result<Resume, Never> {
        return .success(.endFetchLoop)
    }
    /// 0xe4  IN AL,
    func Oxe4() throws -> Result<Resume, Never> {
        if cpl > eflags.iopl {
            throw Interrupt(.GP, errorCode: 0)
        }
        imm = DWord(fetch8())
        regs[.EAX].byteL = io?[imm] ?? 0
        return .success(.endOnInterrupt)
    }
    /// 0xe5  IN AX,
    func Oxe5() throws -> Result<Resume, Never> {
        if cpl > eflags.iopl {
            throw Interrupt(.GP, errorCode: 0)
        }
        imm = DWord(fetch8())
        regs[.EAX] = io?[imm] ?? 0
        return .success(.endOnInterrupt)
    }
    /// 0xe6  OUT ,AL
    func Oxe6() throws -> Result<Resume, Never> {
        if cpl > eflags.iopl {
            throw Interrupt(.GP, errorCode: 0)
        }
        imm = DWord(fetch8())
        io?[imm] = Byte(regs[.EAX].byteL)
        return .success(.endOnInterrupt)
    }
    /// 0xe7  OUT ,AX
    func Oxe7() throws -> Result<Resume, Never> {
        if cpl > eflags.iopl {
            throw Interrupt(.GP, errorCode: 0)
        }
        imm = DWord(fetch8())
        io?[imm] = Word(regs[.EAX].lowerHalf)
        return .success(.endOnInterrupt)
    }
    /// 0xec  IN AL,DX
    func Oxec() throws -> Result<Resume, Never> {
        if cpl > eflags.iopl {
            throw Interrupt(.GP, errorCode: 0)
        }
        regs[.EAX].byteL = io?[regs[.EDX].lowerHalf] ?? 0
        return .success(.endOnInterrupt)
    }
    /// 0xed  IN AX,DX
    func Oxed() throws -> Result<Resume, Never> {
        if cpl > eflags.iopl {
            throw Interrupt(.GP, errorCode: 0)
        }
        regs[.EAX] = io?[regs[.EDX].lowerHalf] ?? 0
        return .success(.endOnInterrupt)
    }
    /// 0xee  OUT DX,AL
    func Oxee() throws -> Result<Resume, Never> {
        if cpl > eflags.iopl {
            throw Interrupt(.GP, errorCode: 0)
        }
        io?[regs[.EDX].lowerHalf] = Byte(regs[.EAX].byteL)
        return .success(.endOnInterrupt)
    }
    /// 0xef  OUT DX,AX
    func Oxef() throws -> Result<Resume, Never> {
        if cpl > eflags.iopl {
            throw Interrupt(.GP, errorCode: 0)
        }
        io?[regs[.EDX].lowerHalf] = Word(regs[.EAX].lowerHalf)
        return .success(.endOnInterrupt)
    }
    /// 0x27  DAA
    func Ox27() throws -> Result<Resume, Never> {
        auxDaa()
        return .success(.endFetchLoop)
    }
    /// 0x2f  DAS
    func Ox2f() throws -> Result<Resume, Never> {
        auxDas()
        return .success(.endFetchLoop)
    }
    /// 0x37  AAA
    func Ox37() throws -> Result<Resume, Never> {
        auxAaa()
        return .success(.endFetchLoop)
    }
    /// 0x3f  AAS
    func Ox3f() throws -> Result<Resume, Never> {
        auxAas()
        return .success(.endFetchLoop)
    }
    /// 0xd4  AAM
    func Oxd4() throws -> Result<Resume, Never> {
        imm = DWord(fetch8())
        try auxAam(imm)
        return .success(.endFetchLoop)
    }
    /// 0xd5  AAD
    func Oxd5() throws -> Result<Resume, Never> {
        imm = DWord(fetch8())
        auxAad(imm)
        return .success(.endFetchLoop)
    }
    /// 0x63  ARPL
    func Ox63() throws -> Result<Resume, Never> {
        try auxArpl()
        return .success(.endFetchLoop)
    }
    /// 0xd6  -
    /// 0xf1  -
    func Oxf1() throws -> Result<Resume, Never> {
        throw Interrupt(.UD)
    }
    /// 0x0f  2-byte instruction escape
    func Ox0f() throws -> Result<Resume, Never> {
        opcode = DWord(fetch8())
        if ipr.isFlagRaised(.lockSignal) {
            switch opcode {
                case 0xa3,  // BT
                    0xab,  // BTS
                    0xb0,  // CMPXCHG
                    0xb1,  // CMPXCHG
                    0xb3,  // BTR
                    0xba,  // G8 (-, -, -, -, BT, BTS, BTR, BTC)
                    0xbb,  // BTC
                    0xc0,  // XADD
                    0xc1:  // XADD
                    break
                default:
                    throw Interrupt(.UD)
            }
        }
        return try twoByteDecoder[opcode]()
    }
}
