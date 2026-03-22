extension Free86 {
    public func fetchDecodeExecute(cycles: QWord) async throws {
        do {
            if halted {
                if await INTR.pending && eflags.isFlagRaised(.IF) {
                    halted = false
                } else {
                    return
                }
            }
            cyclesRequested = cycles
            cyclesRemaining = cycles
            far = 0
            farStart = 0
            if let interrupt = self.interrupt {
                try raiseInterrupt(interrupt.id, interrupt.errorCode, false, false, 0)
            }
            if await INTR.pending && eflags.isFlagRaised(.IF) {
                let id = try await INTR.probe()
                try raiseInterrupt(id, 0, true, false, 0)
            }
        cyclesLoop:
            repeat {  // cycles (actually instructions)
                try obtainOpcode()
                ipr = iprDefault
                let operandSizeOverride = InstructionPrefixRegisterFlag.operandSizeOverride.rawValue
                opcode.setBit(operandSizeOverride, ipr.isFlagRaised(.operandSizeOverride) ? 1 : 0)
            fetchLoop:
                while true {  // loop over instruction bytes (fetch)
                    if ipr.isFlagRaised(.lockSignal) {
                        switch opcode {
                        case 0x00,  // ADD
                            0x01,  // ADD
                            0x08,  // OR
                            0x09,  // OR
                            0x10,  // ADC
                            0x11,  // ADC
                            0x18,  // SBB
                            0x19,  // SBB
                            0x20,  // AND
                            0x21,  // AND
                            0x28,  // SUB
                            0x29,  // SUB
                            0x30,  // XOR
                            0x31,  // XOR
                            0x80,  // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, -)
                            0x81,  // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, -)
                            0x82,  // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, -)
                            0x83,  // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, -)
                            0x86,  // XCHG
                            0x87,  // XCHG
                            0xf6,  // G3 (-, -, NOT, NEG, -, -, -, -)
                            0xf7,  // G3 (-, -, NOT, NEG, -, -, -, -)
                            0xfe,  // G4 (INC, DEC, -, -, -, -, -, -)
                            0xff,  // G5 (INC, DEC, -, -, -, -, -, -)
                            0x0f,  // 2-byte instruction escape
                            /// 16 bit
                            0x101,  // ADD
                            0x109,  // OR
                            0x111,  // ADC
                            0x119,  // SBB
                            0x121,  // AND
                            0x129,  // SUB
                            0x131,  // XOR
                            0x181,  // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, -)
                            0x183,  // G1 (ADD, OR, ADC, SBB, AND, SUB, XOR, -)
                            0x187,  // XCHG
                            0x1f7,  // G3 (-, -, NOT, NEG, -, -, -, -)
                            0x1ff,  // G5 (INC, DEC, -, -, -, -, -, -)
                            0x10f:  // 2-byte instruction escape
                            break
                        default:
                            throw Interrupt(.UD)
                        }
                    }
                    let result = try oneByteDecoder[opcode]()
                    switch result {
                    case .success(let resume):
                        switch resume {
                        case .goOnFetching:
                            continue fetchLoop
                        case .endFetchLoop:
                            break fetchLoop
                        case .endCyclesLoop:
                            break cyclesLoop
                        case .endOnInterrupt:
                            if await INTR.pending && eflags.isFlagRaised(.IF) {
                                break cyclesLoop
                            } else {
                                break fetchLoop
                            }
                        }
                    }
                }
                cyclesRemaining -= 1
            } while cyclesRemaining > 0
            self.cycles = self.cycles &+ cyclesRequested &- cyclesRemaining
            eip = eip &+ far &- farStart
        } catch let interrupt as Interrupt {
            self.cycles = self.cycles &+ cyclesRequested &- cyclesRemaining
            throw interrupt
        }
    }
}
