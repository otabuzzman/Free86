typealias OpcodeDecoder = Array<OpcodeProgram>
typealias OpcodeProgram = () throws -> Result<Resume, Never>

enum Resume {
    case goOnFetching
    case endFetchLoop
    case endCyclesLoop
    case endCyclesLoopOnInterrupt
}
