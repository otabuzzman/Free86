class Free86 {
    var eflags: EFlags = 0

    /// interrupt pins and data bus low byte
    let INTR = PinIO<Bool>()
    let DB8 = PinIO<Byte>()  // INTR vector
    let NMI = PinIO<Bool>()
    let RESET = PinIO<Bool>()

    var cr0: CR0 = 0
    var cr2: DWord = 0
    var cr3: CR3 = 0
    var cr4: DWord = 0 // 80486

    var cpl: DWord = 0 // current privilege level register

    var halted = false

    var cycles: QWord = 0

    ///   Fetch address register
    ///
    ///   The fetch address register (FAR, aka MAR) stores the physical memory
    ///   address of the next byte to be retrieved in the current fetch cycle.
    var far: DWord = 0        // fetch address register (FAR, aka MAR)
    var far_start: DWord = 0  // first fetch address of current cycle

    var opcode: Int = 0

    var dpl: Int = 0  // descriptor privilege level
    var rpl: Int = 0  // requested privilege level
    var iopl: Int = 0  // IO privilege level

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
    var tlbReadOnlyCplX: UnsafeMutablePointer<Int>  // supervisor, any CPL
    var tlbWritableCplX: UnsafeMutablePointer<Int>
    var tlbReadOnlyCpl3: UnsafeMutablePointer<Int>  // user, CPL == 3
    var tlbWritableCpl3: UnsafeMutablePointer<Int>
    /// current mapping tables (user or supervisor)/
    var tlbReadOnly: UnsafeMutablePointer<Int>
    var tlbWritable: UnsafeMutablePointer<Int>

    /// Instruction prefix register

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
    var ipr: InstructionPrefixRegister = 0
    var ipr_default: InstructionPrefixRegister = 0  // reflects D flag (PM (1986), 16.1)
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

    init(memory: MemoryIO<DWord>) {
        self.memory = memory
        memoryCount = DWord(memory.count)
        /// append buffer to store bytes of an instruction
        /// in case it exceeds current page boundary
        memory.register(bank: RAMBank<DWord>(), at: memoryCount)
        tlbPages = [DWord](repeating: 0, count: 2048)
        tlbPagesCount = 0
        tlbReadOnlyCplX = .allocate(capacity: 0x100000)  // 1M entries per MT
        tlbWritableCplX = .allocate(capacity: 0x100000)
        tlbReadOnlyCpl3 = .allocate(capacity: 0x100000)
        tlbWritableCpl3 = .allocate(capacity: 0x100000)
        tlbReadOnly = tlbReadOnlyCplX
        tlbWritable = tlbWritableCplX
    }

    deinit {
        tlbReadOnlyCplX.deallocate()
        tlbWritableCplX.deallocate()
        tlbReadOnlyCpl3.deallocate()
        tlbWritableCpl3.deallocate()
    }
}
