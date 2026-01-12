extension Free86 {
    mutating func fetchDecodeExecute(cycles: QWord) throws {
        cyclesLoop:
        repeat {  // cycles (actually instructions)
            fetchLoop:
            while true {  // loop over instruction bytes (fetch)
                let opcodeDecoder: OpcodeDecoder
                if true {
                    // opcode = fetch()
                    opcodeDecoder = twoByteDecoder
                } else {
                    opcodeDecoder = oneByteDecoder
                }
                let opcodeProgram = opcodeDecoder[opcode]
                switch try opcodeProgram() {
                case .success(let resume):
                    switch resume {
                    case .goOnFetching:
                        continue fetchLoop
                    case .endFetchLoop:
                        break fetchLoop
                    case .endCyclesLoop:
                        break cyclesLoop
                    }
                }
            }
            cyclesRemaining -= 1
        } while cyclesRemaining > 0
    }
}
