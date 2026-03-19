public class Free86 {
    /// interrupt pins and data bus low byte
    let INTR = PinIO<Bool>()
    let DB8 = PinIO<Byte>()  // INTR vector
    let NMI = PinIO<Bool>()
    let RESET = PinIO<Bool>()

    var _interrupt: Interrupt?
    public var interrupt: Interrupt? {
        get {
            let oldValue = _interrupt
            _interrupt = nil
            return oldValue
        }
        set { _interrupt = newValue }
    }
    let io: IsolatedIO<DWord>?

    /// EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    public internal(set) var regs: [GeneralRegister] = .init(repeating: .zero, count: 8)
    public internal(set) var eflags = Eflags(0)

    public internal(set) var eip = LinearAddress(0)
    var eipLinear = LinearAddress(0)

    /// ES, CS, SS, DS, FS, GS, LDT, TR
    public internal(set) var segs: [SegmentRegister] = .init(repeating: .init(0, .init(0)), count: 7)
    var gdt = SegmentRegister(0, .init(0))  // GDT register
    var ldt = SegmentRegister(0, .init(0))  // LDT register
    var tr = SegmentRegister(0, .init(0))  // task register
    var idt = SegmentRegister(0, .init(0))  // IDT register

    var _cr0 = CR0(0)
    public internal(set) var cr0: CR0 {
        get { _cr0 }
        set {
            /// must flush tlb on change of flags 31, 16, or 0
            var mask: CR0 = 0
            mask.setFlag(.PG)
            mask.setFlag(.WP)
            mask.setFlag(.PE)
            if newValue & mask != _cr0 & mask {
                tlbFlush()
            }
            _cr0 = newValue
            _cr0.setFlag(.ET)  // keep bit 4 raised (80387 present)
        }
    }
    public internal(set) var cr2 = LinearAddress(0)
    var _cr3 = CR3(0)
    public internal(set) var cr3: CR3 {
        get { _cr3 }
        set {
            if cr0.isPagingEnabled {
                tlbFlush()
            }
            _cr3 = newValue
        }
    }
    var cr4 = CR4(0)  // 80486

    var _cpl: DWord = 0  // current privilege level register
    var cpl: DWord {
        get { _cpl }
        set {
            if newValue == 3 {
                tlbReadonly = tlbReadonlyCpl3
                tlbWritable = tlbWritableCpl3
            } else {
                tlbReadonly = tlbReadonlyCplX
                tlbWritable = tlbWritableCplX
            }
            _cpl = newValue
        }
    }

    public internal(set) var halted = false
    public internal(set) var cycles: QWord = 0

    /// Fetch address register
    ///
    /// The fetch address register (FAR, aka MAR) stores the physical memory
    /// address of the next byte to be retrieved in the current fetch cycle.
    var far: DWord = 0       // fetch address register (FAR, aka MAR)
    var farStart: DWord = 0  // first fetch address of current cycle

    var opcode: DWord = 0  // sort of fetch data register (FDR, aka MDR)

    /// direction flag (used by string instructions)
    var df: Int32 = 0  // values 1/ -1 reflect EFLAGS.DF false/ true

    var cyclesRequested: QWord = 0
    var cyclesRemaining: QWord = 0

    let memory: MemoryIO<DWord>
    let memoryCount: DWord

    /// Translation lookaside buffer
    ///
    /// The translation lookaside buffer (TLB) maps linear addresses (LA) to
    /// physical addresses (PA). The top 20 bits of an LA represent the PD
    /// and PT indices used by the MMU to look up the PA. These 20 bits act
    /// as an index into an array containing the PA of the corresponding page.
    /// Consequently, each mapping table has 1M entries. There are four
    /// such tables, one for each combination of user, supervisor, read-only
    /// and writable page features. The PA stored in a table is XORed with
    /// its LA and then XORed again on retrieval. This is likely a design
    /// choice rather than a necessity. Another array records up to 2024
    /// mappings stored in the tables, effectively representing the logical
    /// size of the TLB.
    var tlbPages: Array<DWord>
    var tlbPagesCount: Int
    /// mapping tables
    var tlbReadonlyCplX: UnsafeMutablePointer<Int>  // supervisor, any CPL
    var tlbWritableCplX: UnsafeMutablePointer<Int>
    var tlbReadonlyCpl3: UnsafeMutablePointer<Int>  // user, CPL == 3
    var tlbWritableCpl3: UnsafeMutablePointer<Int>
    /// current mapping tables (user or supervisor)
    var tlbReadonly: UnsafeMutablePointer<Int>
    var tlbWritable: UnsafeMutablePointer<Int>

    /// Operand Size Mode
    ///
    /// The operand size mode (OSM) of an instruction defines how each status flag modified by
    /// the instruction is calculated from the source and destination operands of the instruction.
    /// Instructions with multiple operand sizes may have multiple OSM or share an OSM with some
    /// or all sizes. OSM is specific to this emulator and not part of the processor architecture,
    /// from which it was derived. There are 31 OSMs, which are encoded by integers 0 to 30.
    //                           +-------+----+----+-----------+
    //                           |  (implicit) OSM             |
    // +-------------+-----------+-------+----+----+-----------+----+----+----+----+----+----+
    // | instruction | condition | 8 bit | 16 | 32 | condition | OF | SF | ZF | AF | PF | CF |
    // +-------------+-----------+-------+----+----+-----------+----+----+----+----+----+----+
    // | AAA         |           | (i)24 |    |    |           | -  | -  | -  | TM | -  | M  |
    // | AAS         |           | (i)24 |    |    |           | -  | -  | -  | TM | -  | M  |
    // | AAD         |           |   12  |    |    |           | -  | M  | M  | -  | M  | -  |
    // | AAM         |           |   12  |    |    |           | -  | M  | M  | -  | M  | -  |
    // | DAA         |           | (i)24 |    |    |           | -  | M  | M  | TM | M  | TM |
    // | DAS         |           | (i)24 |    |    |           | -  | M  | M  | TM | M  | TM |
    // | ADC         |           |    0  |  1 |  2 | CF 0      | M  | M  | M  | M  | M  | TM |
    // | ADC         |           |    3  |  4 |  5 | CF 1      | M  | M  | M  | M  | M  | TM |
    // | ADD         |           |    0  |  1 |  2 |           | M  | M  | M  | M  | M  | M  |
    // | ADD         |           |       |    |  8 | OP 0x81,  | M  | M  | M  | M  | M  | M  |
    // |             |           |       |    |    | OP 0x83   | M  | M  | M  | M  | M  | M  |
    // | SBB         |           |    6  |  7 |  8 | CF 0      | M  | M  | M  | M  | M  | TM |
    // | SBB         |           |    9  | 10 | 11 | CF 1      | M  | M  | M  | M  | M  | TM |
    // | SUB         |           |    6  |  7 |  8 |           | M  | M  | M  | M  | M  | M  |
    // | CMP         |           |    6  |  7 |  8 |           | M  | M  | M  | M  | M  | M  |
    // | CMPS        |           |       |    |    |           | M  | M  | M  | M  | M  | M  |
    // | SCAS        |           |       |    |    |           | M  | M  | M  | M  | M  | M  |
    // | NEG         |           |       |    |    |           | M  | M  | M  | M  | M  | M  |
    // | DEC         |           |   28  | 29 | 30 |           | M  | M  | M  | M  | M  | U  |
    // | INC         |           |   25  | 26 | 27 |           | M  | M  | M  | M  | M  | U  |
    // | IMUL        |           |   21  | 22 | 23 |           | M  | -  | -  | -  | -  | M  |
    // | MUL         |           |   21  | 22 | 23 |           | M  | -  | -  | -  | -  | M  |
    // | RCL/RCR     | count  1  |   24  | 24 | 24 |           | M  | U  | U  | U  | U  | TM |
    // | RCL/RCR     | count >1  |   24  | 24 | 24 |           | -  | U  | U  | U  | U  | TM |
    // | ROL/ROR     | count  1  |   24  | 24 | 24 |           | M  | U  | U  | U  | U  | M  |
    // | ROL/ROR     | count >1  |   24  | 24 | 24 |           | -  | U  | U  | U  | U  | M  |
    // | SAR/SHR     | count  1  |   18  | 19 | 20 |           | M  | M  | M  | -  | M  | M  |
    // | SAR/SHR     | count >1  |   18  | 19 | 20 |           | -  | M  | M  | -  | M  | M  |
    // | SAL/SHL     | count  1  |   15  | 16 | 17 |           | M  | M  | M  | -  | M  | M  |
    // | SAL/SHL     | count >1  |   15  | 16 | 17 |           | -  | M  | M  | -  | M  | M  |
    // | SHLD        |           |       | 19 | 17 |           | -  | M  | M  | -  | M  | M  |
    // | SHRD        |           |       | 19 | 20 |           | -  | M  | M  | -  | M  | M  |
    // | BSF/BSR     |           |       |    | 14 |           | -  | -  | M  | -  | -  | -  |
    // | BT/BT[SRC]  |           |       | 19 | 20 |           | -  | -  | -  | -  | -  | M  |
    // | AND         |           |   12  | 13 | 14 |           | 0  | M  | M  | -  | M  | 0  |
    // | OR          |           |   12  | 13 | 14 |           | 0  | M  | M  | -  | M  | 0  |
    // | TEST        |           |   12  | 13 | 14 |           | 0  | M  | M  | -  | M  | 0  |
    // | XOR         |           |   12  | 13 | 14 |           | 0  | M  | M  | -  | M  | 0  |
    // +-------------+-----------+-------+----+----+-----------+----+----+----+----+----+----+
    //                                                                 (PM (1986), Appendix C)
    // +-------------+-----------+-------+----+----+-----------+----+----+----+----+----+----+
    // | ARPL        |           |       |    | 24 |           | -  | -  | M  | -  | -  | -  |
    // | LAR/LSL     |           |       |    | 24 |           | -  | -  | M  | -  | -  | -  |
    // | VERR/VERW   |           |       |    | 24 |           | -  | -  | M  | -  | -  | -  |
    // +-------------+-----------+-------+----+----+-----------+----+----+----+----+----+----+
    // 0 : instruction resets,
    // T : tests,
    // M : modifies flag,
    // U : leaves flag undefined,
    // - : does not affect flag.
    var osm: Int = 0
    var osmSrc: DWord = 0
    var osmDst: DWord = 0

    /// osmPreserved/ osmDstPreservedst_preserved preserve OSM/ destination of instruction
    /// before INC/ DEC but not including INC/ DEC. This is for later calculation of CF
    /// which is not modified by INC/ DEC. CF calculation after one or more
    /// successive INC/ DEC is therefore based on the values for OSM, source and
    /// destination before INC/ DEC. It is not necessary to also preserve source,
    /// since INC/ DEC do not store the implicit value 1 in `osmSrc', which therefore
    /// remains valid.
    var osmPreserved: Int = 0
    var osmDstPreserved: DWord = 0

    /// Instruction prefix register
    ///
    /// The instruction prefix register (IPR) captures the instruction prefixes
    /// of the current fetch cycle, each in its own bit. IPR is specific to
    /// this emulator and not part of the processor architecture, from which
    /// it was derived.
    // 0x0001 ES segment override prefix    (0x26)
    // 0x0002 CS segment override prefix    (0x2e)
    // 0x0003 SS segment override prefix    (0x36)
    // 0x0004 DS segment override prefix    (0x3e)
    // 0x0005 FS segment override prefix    (0x64)
    // 0x0006 GS segment override prefix    (0x65)
    // 0x0010 REPZ  string operation prefix (0xf3)
    // 0x0020 REPNZ string operation prefix (0xf2)
    // 0x0040 LOCK  signal prefix           (0xf0)
    // 0x0080 address-size override prefix  (0x67)
    // 0x0100 operand-size override prefix  (0x66)
    var ipr = InstructionPrefixRegister(0)
    var iprDefault: InstructionPrefixRegister {  // reflects D flag (PM (1986), 16.1), also part of SSB (below)
        var ipr = InstructionPrefixRegister(0)
        if !segs[.CS].shadow.isFlagRaised(.D) {
            ipr.setFlag(.addressSizeOverride)
            ipr.setFlag(.operandSizeOverride)
        }
        return ipr
    }

    lazy var instruction = Instruction(parent: self)

    /// Segments state block
    ///
    /// Variables in the segments state block (SSB) provide shorthand
    /// access to frequently used segment data.
    var csBase: LinearAddress {
        segs[.CS].shadow.base
    }
    var ssBase: LinearAddress {
        segs[.SS].shadow.base
    }
    var ssMask: DWord {  // 16/ 32 bit address size mask
        segs[.SS].shadow.isFlagRaised(.D) ? 0xFFFFFFFF : 0xFFFF
    }

    /// https://en.wikipedia.org/wiki/X86_memory_segmentation
    var x8664LongMode: Bool {
        segs[.ES].shadow.base | csBase | ssBase | segs[.DS].shadow.base == 0 && ssMask == 0xFFFFFFFF
    }
    /// end of SSB

    /// auxiliary variables for inter-method exchange
    var lax = LinearAddress(0)  // linear address exchange register
    var operation: DWord = 0  // bits 5..3 of opcode or modR/M byte
    var modRM: ModRM = 0, sib: SIB = 0
    var reg = 0, rM = 0
    var r: DWord = 0, rm: DWord = 0  // register or register/ memory by modRM
    var m: DWord = 0, m16: Word = 0  // 32/ 16 bit memory operands from memory
    var imm: DWord = 0, imm16: Word = 0, moffs: DWord = 0  // immediate/ offset operands
    var u: DWord = 0, v: DWord = 0, w: DWord = 0  // intermediate results

    typealias OpcodeDecoder = Array<OpcodeProgram>
    typealias OpcodeProgram = () throws -> Result<Resume, Never>

    enum Resume {
        case goOnFetching
        case endFetchLoop
        case endCyclesLoop
        case endOnInterrupt
    }

    var invalid: OpcodeProgram = {
        throw Interrupt(.UD)
    }
    lazy var oneByteDecoder: OpcodeDecoder = [
        //          0x0      0x1      0x2      0x3      0x4      0x5      0x6      0x7      0x8      0x9      0xa      0xb      0xc      0xd      0xe      0xf
        /* 0x000 */ Ox38,    Ox01,    Ox3a,    Ox03,    Ox3c,    Ox05,    Ox1e,    Ox1f,    Ox38,    Ox31,    Ox3a,    Ox33,    Ox3c,    Ox2d,    Ox1e,    Ox0f,
        /* 0x010 */ Ox38,    Ox31,    Ox3a,    Ox33,    Ox3c,    Ox2d,    Ox1e,    Ox1f,    Ox38,    Ox31,    Ox3a,    Ox33,    Ox3c,    Ox2d,    Ox1e,    Ox1f,
        /* 0x020 */ Ox38,    Ox31,    Ox3a,    Ox33,    Ox3c,    Ox2d,    Ox3e,    Ox27,    Ox38,    Ox31,    Ox3a,    Ox33,    Ox3c,    Ox2d,    Ox3e,    Ox2f,
        /* 0x030 */ Ox38,    Ox31,    Ox3a,    Ox33,    Ox3c,    Ox35,    Ox3e,    Ox37,    Ox38,    Ox39,    Ox3a,    Ox3b,    Ox3c,    Ox3d,    Ox3e,    Ox3f,
        /* 0x040 */ Ox47,    Ox47,    Ox47,    Ox47,    Ox47,    Ox47,    Ox47,    Ox47,    Ox4f,    Ox4f,    Ox4f,    Ox4f,    Ox4f,    Ox4f,    Ox4f,    Ox4f,
        /* 0x050 */ Ox57,    Ox57,    Ox57,    Ox57,    Ox57,    Ox57,    Ox57,    Ox57,    Ox5f,    Ox5f,    Ox5f,    Ox5f,    Ox5f,    Ox5f,    Ox5f,    Ox5f,
        /* 0x060 */ Ox60,    Ox61,    Ox62,    Ox63,    Ox65,    Ox65,    Ox66,    Ox67,    Ox68,    Ox69,    Ox6a,    Ox6b,    Ox6c,    Ox6d,    Ox6e,    Ox6f,
        /* 0x070 */ Ox70,    Ox71,    Ox72,    Ox73,    Ox74,    Ox75,    Ox76,    Ox77,    Ox78,    Ox79,    Ox7a,    Ox7b,    Ox7c,    Ox7d,    Ox7e,    Ox7f,
        /* 0x080 */ Ox82,    Ox81,    Ox82,    Ox83,    Ox84,    Ox85,    Ox86,    Ox87,    Ox88,    Ox89,    Ox8a,    Ox8b,    Ox8c,    Ox8d,    Ox8e,    Ox8f,
        /* 0x090 */ Ox90,    Ox97,    Ox97,    Ox97,    Ox97,    Ox97,    Ox97,    Ox97,    Ox98,    Ox99,    Ox9a,    Ox9b,    Ox9c,    Ox9d,    Ox9e,    Ox9f,
        /* 0x0a0 */ Oxa0,    Oxa1,    Oxa2,    Oxa3,    Oxa4,    Oxa5,    Oxa6,    Oxa7,    Oxa8,    Oxa9,    Oxaa,    Oxab,    Oxac,    Oxad,    Oxae,    Oxaf,
        /* 0x0b0 */ Oxb7,    Oxb7,    Oxb7,    Oxb7,    Oxb7,    Oxb7,    Oxb7,    Oxb7,    Oxbf,    Oxbf,    Oxbf,    Oxbf,    Oxbf,    Oxbf,    Oxbf,    Oxbf,
        /* 0x0c0 */ Oxc0,    Oxc1,    Oxc2,    Oxc3,    Oxc4,    Oxc5,    Oxc6,    Oxc7,    Oxc8,    Oxc9,    Oxca,    Oxcb,    Oxcc,    Oxcd,    Oxce,    Oxcf,
        /* 0x0d0 */ Oxd0,    Oxd1,    Oxd2,    Oxd3,    Oxd4,    Oxd5,    Oxf1,    Oxd7,    Oxdf,    Oxdf,    Oxdf,    Oxdf,    Oxdf,    Oxdf,    Oxdf,    Oxdf,
        /* 0x0e0 */ Oxe2,    Oxe2,    Oxe2,    Oxe3,    Oxe4,    Oxe5,    Oxe6,    Oxe7,    Oxe8,    Oxe9,    Oxea,    Oxeb,    Oxec,    Oxed,    Oxee,    Oxef,
        /* 0x0f0 */ Oxf0,    Oxf1,    Oxf2,    Oxf3,    Oxf4,    Oxf5,    Oxf6,    Oxf7,    Oxf8,    Oxf9,    Oxfa,    Oxfb,    Oxfc,    Oxfd,    Oxfe,    Oxff,
        ///
        /// 16 bit programs
        ///
        /* 0x100 */ Ox1fe,   Ox139,   Ox1fe,   Ox13b,   Ox1fe,   Ox13d,   Ox11e,   Ox11f,   Ox1fe,   Ox139,   Ox1fe,   Ox13b,   Ox1fe,   Ox13d,   Ox11e,   Ox10f,
        /* 0x110 */ Ox1fe,   Ox139,   Ox1fe,   Ox13b,   Ox1fe,   Ox13d,   Ox11e,   Ox11f,   Ox1fe,   Ox139,   Ox1fe,   Ox13b,   Ox1fe,   Ox13d,   Ox11e,   Ox11f,
        /* 0x120 */ Ox1fe,   Ox139,   Ox1fe,   Ox13b,   Ox1fe,   Ox13d,   Ox1fe,   Ox1fe,   Ox1fe,   Ox139,   Ox1fe,   Ox13b,   Ox1fe,   Ox13d,   Ox1fe,   Ox1fe,
        /* 0x130 */ Ox1fe,   Ox139,   Ox1fe,   Ox13b,   Ox1fe,   Ox13d,   Ox1fe,   Ox1fe,   Ox1fe,   Ox139,   Ox1fe,   Ox13b,   Ox1fe,   Ox13d,   Ox1fe,   Ox1fe,
        /* 0x140 */ Ox147,   Ox147,   Ox147,   Ox147,   Ox147,   Ox147,   Ox147,   Ox147,   Ox14f,   Ox14f,   Ox14f,   Ox14f,   Ox14f,   Ox14f,   Ox14f,   Ox14f,
        /* 0x150 */ Ox157,   Ox157,   Ox157,   Ox157,   Ox157,   Ox157,   Ox157,   Ox157,   Ox15f,   Ox15f,   Ox15f,   Ox15f,   Ox15f,   Ox15f,   Ox15f,   Ox15f,
        /* 0x160 */ Ox160,   Ox161,   Ox162,   Ox10f,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox168,   Ox169,   Ox16a,   Ox16b,   Ox1fe,   Ox16d,   Ox1fe,   Ox16f,
        /* 0x170 */ Ox17f,   Ox17f,   Ox17f,   Ox17f,   Ox17f,   Ox17f,   Ox17f,   Ox17f,   Ox17f,   Ox17f,   Ox17f,   Ox17f,   Ox17f,   Ox17f,   Ox17f,   Ox17f,
        /* 0x180 */ Ox1fe,   Ox181,   Ox1fe,   Ox183,   Ox1fe,   Ox185,   Ox1fe,   Ox187,   Ox1fe,   Ox189,   Ox1fe,   Ox18b,   Ox1fe,   Ox18d,   Ox1fe,   Ox18f,
        /* 0x190 */ Ox190,   Ox197,   Ox197,   Ox197,   Ox197,   Ox197,   Ox197,   Ox197,   Ox198,   Ox199,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,
        /* 0x1a0 */ Ox1fe,   Ox1a1,   Ox1fe,   Ox1a3,   Ox1fe,   Ox1a5,   Ox1fe,   Ox1a7,   Ox1fe,   Ox1a9,   Ox1fe,   Ox1ab,   Ox1fe,   Ox1ad,   Ox1fe,   Ox1af,
        /* 0x1b0 */ Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1bf,   Ox1bf,   Ox1bf,   Ox1bf,   Ox1bf,   Ox1bf,   Ox1bf,   Ox1bf,
        /* 0x1c0 */ Ox1fe,   Ox1c1,   Ox1c2,   Ox1c3,   Ox1c4,   Ox1c5,   Ox1fe,   Ox1c7,   Ox1c8,   Ox1c9,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,
        /* 0x1d0 */ Ox1fe,   Ox1d1,   Ox1fe,   Ox1d3,   Ox1fe,   Ox1fe,   Ox10f,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,
        /* 0x1e0 */ Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1e5,   Ox1fe,   Ox1e7,   Ox1e8,   Ox1e9,   Ox1fe,   Ox1eb,   Ox1fe,   Ox1ed,   Ox1fe,   Ox1ef,
        /* 0x1f0 */ Ox1fe,   Ox10f,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1f7,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1fe,   Ox1ff
    ]
    lazy var twoByteDecoder: OpcodeDecoder = [
        //          0x0      0x1      0x2      0x3      0x4      0x5      0x6      0x7      0x8      0x9      0xa      0xb      0xc      0xd      0xe      0xf
        /* 0x000 */ Ox0f00,  Ox0f01,  Ox0f03,  Ox0f03,  Ox0fff,  Ox0fff,  Ox0f06,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,
        /* 0x010 */ Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,
        /* 0x020 */ Ox0f20,  Ox0fff,  Ox0f22,  Ox0f23,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,
        /* 0x030 */ Ox0fff,  Ox0f31,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,
        /* 0x040 */ Ox0f4f,  Ox0f4f,  Ox0f4f,  Ox0f4f,  Ox0f4f,  Ox0f4f,  Ox0f4f,  Ox0f4f,  Ox0f4f,  Ox0f4f,  Ox0f4f,  Ox0f4f,  Ox0f4f,  Ox0f4f,  Ox0f4f,  Ox0f4f,
        /* 0x050 */ Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,
        /* 0x060 */ Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,
        /* 0x070 */ Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,
        /* 0x080 */ Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,
        /* 0x090 */ Ox0f9f,  Ox0f9f,  Ox0f9f,  Ox0f9f,  Ox0f9f,  Ox0f9f,  Ox0f9f,  Ox0f9f,  Ox0f9f,  Ox0f9f,  Ox0f9f,  Ox0f9f,  Ox0f9f,  Ox0f9f,  Ox0f9f,  Ox0f9f,
        /* 0x0a0 */ Ox0fa8,  Ox0fa9,  Ox0fa2,  Ox0fa3,  Ox0fa4,  Ox0fa5,  Ox0fff,  Ox0fff,  Ox0fa8,  Ox0fa9,  Ox0fff,  Ox0fbb,  Ox0fac,  Ox0fad,  Ox0fff,  Ox0faf,
        /* 0x0b0 */ Ox0fb0,  Ox0fb1,  Ox0fb5,  Ox0fbb,  Ox0fb5,  Ox0fb5,  Ox0fb6,  Ox0fb7,  Ox0fff,  Ox0fff,  Ox0fba,  Ox0fbb,  Ox0fbd,  Ox0fbd,  Ox0fbe,  Ox0fbf,
        /* 0x0c0 */ Ox0fc0,  Ox0fc1,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fcf,  Ox0fcf,  Ox0fcf,  Ox0fcf,  Ox0fcf,  Ox0fcf,  Ox0fcf,  Ox0fcf,
        /* 0x0d0 */ Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,
        /* 0x0e0 */ Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,
        /* 0x0f0 */ Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,  Ox0fff,
        ///
        /// 16 bit programs
        ///
        /* 0x100 */ Ox10f1b0, Ox10f1b0, Ox10f1b0, Ox10f1b0, Ox10f1c0, Ox10f1c0, Ox10f1b0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0,
        /* 0x110 */ Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0,
        /* 0x120 */ Ox10f1b0, Ox10f1c0, Ox10f1b0, Ox10f1b0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0,
        /* 0x130 */ Ox10f1c0, Ox10f1b0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0,
        /* 0x140 */ Ox10f14f, Ox10f14f, Ox10f14f, Ox10f14f, Ox10f14f, Ox10f14f, Ox10f14f, Ox10f14f, Ox10f14f, Ox10f14f, Ox10f14f, Ox10f14f, Ox10f14f, Ox10f14f, Ox10f14f, Ox10f14f,
        /* 0x150 */ Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0,
        /* 0x160 */ Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0,
        /* 0x170 */ Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1c0,
        /* 0x180 */ Ox10f18f, Ox10f18f, Ox10f18f, Ox10f18f, Ox10f18f, Ox10f18f, Ox10f18f, Ox10f18f, Ox10f18f, Ox10f18f, Ox10f18f, Ox10f18f, Ox10f18f, Ox10f18f, Ox10f18f, Ox10f18f,
        /* 0x190 */ Ox10f1b0, Ox10f1b0, Ox10f1b0, Ox10f1b0, Ox10f1b0, Ox10f1b0, Ox10f1b0, Ox10f1b0, Ox10f1b0, Ox10f1b0, Ox10f1b0, Ox10f1b0, Ox10f1b0, Ox10f1b0, Ox10f1b0, Ox10f1b0,
        /* 0x1a0 */ Ox10f1a8, Ox10f1a9, Ox10f1b0, Ox10f1a3, Ox10f1ac, Ox10f1ad, Ox10f1c0, Ox10f1c0, Ox10f1a8, Ox10f1a9, Ox10f1c0, Ox10f1bb, Ox10f1ac, Ox10f1ad, Ox10f1c0, Ox10f1af,
        /* 0x1b0 */ Ox10f1b0, Ox10f1b1, Ox10f1b5, Ox10f1bb, Ox10f1b5, Ox10f1b5, Ox10f1b6, Ox10f1c0, Ox10f1c0, Ox10f1c0, Ox10f1ba, Ox10f1bb, Ox10f1bd, Ox10f1bd, Ox10f1be, Ox10f1c0,
        /* 0x1c0 */ Ox10f1c0, Ox10f1c1, invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,
        /* 0x1d0 */ invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,
        /* 0x1e0 */ invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,
        /* 0x1f0 */ invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid,  invalid
    ]

    public init(memory: MemoryIO<DWord>, io: IsolatedIO<DWord>? = nil) {
        /// RAM configuration check
        let a = memory.ld(from: 0x00000000)
        let o = memory.ld(from: 0x000FFFF0)
        memory.st(at: 0x00000000, dword: 0xDEADBEEF)
        memory.st(at: 0x000FFFF0, dword: 0xC0DECAFE)
        assert(memory.ld(from: 0x00000000) == 0xDEADBEEF, "no RAM at address 0x00000000")
        assert(memory.ld(from: 0x000FFFF0) == 0xC0DECAFE, "no RAM at address 0x000FFFF0")
        memory.st(at: 0x00000000, dword: a)
        memory.st(at: 0x000FFFF0, dword: o)

        self.memory = memory
        memoryCount = DWord(memory.count)
        /// append buffer to store bytes of an instruction
        /// in case it exceeds current page boundary
        memory.register(bank: RAMBank<DWord>(), at: memoryCount)
        tlbPages = [DWord](repeating: 0, count: 2048)
        tlbPagesCount = 0
        tlbReadonlyCplX = .allocate(capacity: 0x100000)  // 1M entries per MT
        tlbWritableCplX = .allocate(capacity: 0x100000)
        tlbReadonlyCpl3 = .allocate(capacity: 0x100000)
        tlbWritableCpl3 = .allocate(capacity: 0x100000)
        tlbReadonly = tlbReadonlyCplX
        tlbWritable = tlbWritableCplX
        tlbReadonlyCplX.initialize(repeating: -1, count: 0x100000)
        tlbWritableCplX.initialize(repeating: -1, count: 0x100000)
        tlbReadonlyCpl3.initialize(repeating: -1, count: 0x100000)
        tlbWritableCpl3.initialize(repeating: -1, count: 0x100000)
        self.io = io
        reset()
    }
    deinit {
        tlbReadonlyCplX.deallocate()
        tlbWritableCplX.deallocate()
        tlbReadonlyCpl3.deallocate()
        tlbWritableCpl3.deallocate()
    }
}
