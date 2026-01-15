struct Free86 {
    var eflags: EFlags

    /// interrupt pins and data bus low byte
    let INTR = PinIO<Bool>()
    let DB8 = PinIO<Byte>()  // INTR vector
    let NMI = PinIO<Bool>()
    let RESET = PinIO<Bool>()

    ///   Fetch address register
    ///
    ///   The fetch address register (FAR, aka MAR) stores the physical memory
    ///   address of the next byte to be retrieved in the current fetch cycle.
    var far: DWord        // fetch address register (FAR, aka MAR)
    var far_start: DWord  // first fetch address of current cycle

    var opcode: Int
    
    var cyclesRequested: QWord
    var cyclesRemaining: QWord
    
    /// Instruction prefix register
    
    /// The instruction prefix register (IPR) captures the instruction prefixes
    /// of the current retrieval cycle, each in its own bit. IPR is specific to
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
    var ipr: InstructionPrefixRegister
    var ipr_default: InstructionPrefixRegister  // reflects D flag (PM (1986), 16.1)
                                                // also belongs to the SSB (below)

    typealias OpcodeDecoder = Array<OpcodeProgram>
    typealias OpcodeProgram = () throws -> Result<Resume, Never>

    enum Resume {
        case goOnFetching
        case endFetchLoop
        case endCyclesLoop
        case endCyclesLoopOnInterrupt
    }

    var invalid: OpcodeProgram = {
        throw Interrupt(.UD)
    }
    lazy var oneByteDecoder: OpcodeDecoder = [
        //          0x0      0x1      0x2      0x3      0x4      0x5      0x6      0x7      0x8      0x9      0xa      0xb      0xc      0xd      0xe      0xf
        /* 0x000 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x010 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x020 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x030 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x040 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x050 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x060 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x070 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x080 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x090 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x0a0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x0b0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x0c0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x0d0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x0e0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x0f0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        ///
        /// 16 bit programs
        ///
        /* 0x100 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x110 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x120 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x130 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x140 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x150 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x160 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x170 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x180 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x190 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x1a0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x1b0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x1c0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x1d0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x1e0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x1f0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid
    ]
    lazy var twoByteDecoder: OpcodeDecoder = [
        //          0x0      0x1      0x2      0x3      0x4      0x5      0x6      0x7      0x8      0x9      0xa      0xb      0xc      0xd      0xe      0xf
        /* 0x000 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x010 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x020 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x030 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x040 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x050 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x060 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x070 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x080 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x090 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x0a0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x0b0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x0c0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x0d0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x0e0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x0f0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        ///
        /// 16 bit programs
        ///
        /* 0x100 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x110 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x120 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x130 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x140 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x150 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x160 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x170 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x180 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x190 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x1a0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x1b0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x1c0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x1d0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x1e0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x1f0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid
    ]
}
