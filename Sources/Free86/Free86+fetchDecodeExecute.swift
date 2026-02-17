extension Free86 {
    func fetchDecodeExecute(cycles: QWord) async throws {
        cyclesLoop:
        repeat {  // cycles (actually instructions)
            fetchLoop:
            while true {  // loop over instruction bytes (fetch)
                let result = try oneByteDecoder[Int(opcode)]()
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
