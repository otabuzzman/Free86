struct Instruction {
    mutating func length(opcode: Int) throws -> Int {
        var n = 0
        fetchLoop:
        while true {  // loop over instruction bytes (fetch)
            let result = try oneByteDecoder[opcode]()
            switch result {
            case .success(let resume):
                switch resume {
                case .goOnFetching:
                    continue fetchLoop
                case .endFetchLoop:
                    fallthrough
                default:
                    break fetchLoop
               }
            }
        }
        return n
    }

    var invalid: OpcodeProgram = {
        throw Interrupt(.UD)
    }
    lazy var oneByteDecoder: OpcodeDecoder = [
    ]
    lazy var twoByteDecoder: OpcodeDecoder = [
    ]
}
