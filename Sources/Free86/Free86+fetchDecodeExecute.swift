extension Free86 {
    mutating func fetchDecodeExecute(cycles: QWord) throws {
        cyclesLoop:
        repeat {  // cycles (actually instructions)
            fetchLoop:
            while true {  // loop over instruction bytes (fetch)
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
                    }
                }
            }
            cyclesRemaining -= 1
        } while cyclesRemaining > 0
    }
}
