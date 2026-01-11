typealias OpcodeDecoder = Array<OpcodeProgram>

typealias OpcodeProgram = () throws -> Result<OpcodeProgramResult, Never>

enum OpcodeProgramResult {
    case goOnFetching
    case endFetchLoop
    case endCyclesLoop
}
