struct Free86 {
    var opcode: DWord
    
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

    lazy var invalid: OpcodeProgram = {
        throw Interrupt(.UD)
    }

    lazy var oneByteDecoder: OpcodeDecoder = [invalid]
    lazy var twoByteDecoder: OpcodeDecoder = []
}
