extension Free86 {
    func fetchDecodeExecute(cycles: QWord) async throws {
        cyclesLoop:
        repeat {  // cycles (actually instructions)
            fetchLoop:
            while true {  // loop over instruction bytes (fetch)
                if ipr.isFlagRaised(.lockSignal) {
                    if ipr.isFlagRaised(.operandSizeOverride) {
                        switch opcode {
                            case 0x101,  // ADD
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
                    } else {
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
                                0x0f:  // 2-byte instruction escape
                                break
                            default:
                                throw Interrupt(.UD)
                        }
                    }
                }
                var result: OpcodeDecoder
                if ipr.isFlagRaised(.operandSizeOverride) {
                    result = try oneByte16Decoder[Int(opcode)]()
                } else {
                    result = try oneByteDecoder[Int(opcode)]()
                }
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
                        if eflags.isFlagRaised(.IF) {
                            let irq = try await INTR.probe()
                            if irq {
                                break cyclesLoop
                            } else {
                                break fetchLoop
                            }
                        }
                    }
                }
            }
            cyclesRemaining -= 1
        } while cyclesRemaining > 0
    }
}
