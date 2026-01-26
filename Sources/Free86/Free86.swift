class Free86 {
    /// interrupt pins and data bus low byte
    let INTR = PinIO<Bool>()
    let DB8 = PinIO<Byte>()  // INTR vector
    let NMI = PinIO<Bool>()
    let RESET = PinIO<Bool>()

    /// EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    var regs: [GeneralRegister] = .init(repeating: .zero, count: 8)
    var eflags: EFlags = 0

    var eip: DWord = 0

    /// ES, CS, SS, DS, FS, GS, LDT, TR
    var segs: [SegmentRegister] = .init(repeating: .init(0, .init(0)), count: 7)
    var gdt = SegmentRegister(0, .init(0))  // GDT register
    var ldt = SegmentRegister(0, .init(0))  // LDT register
    var tr = SegmentRegister(0, .init(0))  // task register
    var idt = SegmentRegister(0, .init(0))  // IDT register

    var _cr0: CR0 = 0
    var cr2: LinearAddress = 0
    var _cr3: CR3 = 0
    var cr4: DWord = 0  // 80486

    var _cpl: Int = 0  // current privilege level register

    var halted = false
    var cycles: QWord = 0

    ///   Fetch address register
    ///
    ///   The fetch address register (FAR, aka MAR) stores the physical memory
    ///   address of the next byte to be retrieved in the current fetch cycle.
    var far: DWord = 0        // fetch address register (FAR, aka MAR)
    var farStart: DWord = 0  // first fetch address of current cycle

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
    var tlbReadonlyCplX: UnsafeMutablePointer<Int>  // supervisor, any CPL
    var tlbWritableCplX: UnsafeMutablePointer<Int>
    var tlbReadonlyCpl3: UnsafeMutablePointer<Int>  // user, CPL == 3
    var tlbWritableCpl3: UnsafeMutablePointer<Int>
    /// current mapping tables (user or supervisor)
    var tlbReadonly: UnsafeMutablePointer<Int>
    var tlbWritable: UnsafeMutablePointer<Int>

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
    var ipr: InstructionPrefixRegister = 0
    var iprDefault: InstructionPrefixRegister {  // reflects D flag (PM (1986), 16.1), also part of SSB (below)
        var ipr = InstructionPrefixRegister(0)
        if segs[.CS].hidden.isFlagRaised(.D) {
            ipr.setFlag(.addressSizeOverride)
            ipr.setFlag(.operandSizeOverride)
        }
        return ipr
    }

    ///   Segments state block
    ///
    ///   Variables in the segments state block (SSB) provide shorthand
    ///   access to frequently used segment data.
    var csBase: LinearAddress {
        segs[.CS].hidden.base
    }
    var ssBase: LinearAddress {
        segs[.SS].hidden.base
    }
    var ssMask: DWord {  // 16/ 32 bit address size mask
        segs[.SS].hidden.isFlagRaised(.D) ? 0xFFFFFFFF : 0xFFFF
    }

    /// https://en.wikipedia.org/wiki/X86_memory_segmentation
    var x8664LongMode: Bool {
        segs[.ES].hidden.base | csBase | ssBase | segs[.DS].hidden.base == 0 && ssMask == 0xFFFFFFFF
    }
    /// end of SSB

    /// auxiliary variables for inter-method exchange
    var lax: LinearAddress = 0  // linear address exchange register

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
        /// RAM configuration check
        memory.st(at: 0x00000000, dword: 0xDEADBEEF)
        memory.st(at: 0x000FFFF0, dword: 0xC0DECAFE)
        assert(memory.ld(from: 0x00000000) == 0xDEADBEEF, "no RAM at address 0x00000000")
        assert(memory.ld(from: 0x000FFFF0) == 0xC0DECAFE, "no RAM at address 0x000FFFF0")

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
        tlbReadonlyCplX.initialize(repeating: -1, count: 0x10000)
        tlbWritableCplX.initialize(repeating: -1, count: 0x10000)
        tlbReadonlyCpl3.initialize(repeating: -1, count: 0x10000)
        tlbWritableCpl3.initialize(repeating: -1, count: 0x10000)
        reset()
    }
    deinit {
        tlbReadonlyCplX.deallocate()
        tlbWritableCplX.deallocate()
        tlbReadonlyCpl3.deallocate()
        tlbWritableCpl3.deallocate()
    }
}
