struct Instruction {
    var parent: Free86
    var _length = 0
    mutating func length() throws -> Int {
        _length = 0
        fetchLoop:
        while true {  // loop over instruction bytes (fetch)
            let result = try oneByteDecoder[Int(parent.opcode)]()
            switch result {
            case .success(let resume):
                switch resume {
                case .goOnFetching:
                    continue fetchLoop
                case .endFetchLoop:
                    break fetchLoop
                default:
                    assert(false, "fatal error")
                }
            }
        }
        return _length
    }

    typealias Resume = Free86.Resume  // convenience shortener for opcode programs definitions

    var invalid: Free86.OpcodeProgram = {
        throw Interrupt(.UD)
    }
    lazy var oneByteDecoder: Free86.OpcodeDecoder = [
        //          0x0      0x1      0x2      0x3      0x4      0x5      0x6      0x7      0x8      0x9      0xa      0xb      0xc      0xd      0xe      0xf
        /* 0x000 */ Oxff,    Oxff,    Oxff,    Oxff,    Oxeb,    Oxe9,    Oxfd,    Oxfd,    Oxff,    Oxff,    Oxff,    Oxff,    Oxeb,    Oxe9,    Oxfd,    Ox0f,
        /* 0x010 */ Oxff,    Oxff,    Oxff,    Oxff,    Oxeb,    Oxe9,    Oxfd,    Oxfd,    Oxff,    Oxff,    Oxff,    Oxff,    Oxeb,    Oxe9,    Oxfd,    Oxfd,
        /* 0x020 */ Oxff,    Oxff,    Oxff,    Oxff,    Oxeb,    Oxe9,    Oxf3,    Oxfd,    Oxff,    Oxff,    Oxff,    Oxff,    Oxeb,    Oxe9,    Oxf3,    Oxfd,
        /* 0x030 */ Oxff,    Oxff,    Oxff,    Oxff,    Oxeb,    Oxe9,    Oxf3,    Oxfd,    Oxff,    Oxff,    Oxff,    Oxff,    Oxeb,    Oxe9,    Oxf3,    Oxfd,
        /* 0x040 */ Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,
        /* 0x050 */ Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,
        /* 0x060 */ Oxfd,    Oxfd,    Oxff,    Oxff,    Oxf3,    Oxf3,    Ox66,    Ox67,    Oxe9,    Oxc7,    Oxeb,    Oxc6,    Oxfd,    Oxfd,    Oxfd,    Oxfd,
        /* 0x070 */ Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,
        /* 0x080 */ Oxc6,    Oxc7,    Oxc6,    Oxc6,    Oxff,    Oxff,    Oxff,    Oxff,    Oxff,    Oxff,    Oxff,    Oxff,    Oxff,    Oxff,    Oxff,    Oxff,
        /* 0x090 */ Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxea,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,
        /* 0x0a0 */ Oxa3,    Oxa3,    Oxa3,    Oxa3,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxeb,    Oxe9,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,
        /* 0x0b0 */ Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxe9,    Oxe9,    Oxe9,    Oxe9,    Oxe9,    Oxe9,    Oxe9,    Oxe9,
        /* 0x0c0 */ Oxc6,    Oxc6,    Oxca,    Oxfd,    Oxff,    Oxff,    Oxc6,    Oxc7,    Oxc8,    Oxfd,    Oxca,    Oxfd,    Oxfd,    Oxeb,    Oxfd,    Oxfd,
        /* 0x0d0 */ Oxff,    Oxff,    Oxff,    Oxff,    Oxeb,    Oxeb,    Oxf1,    Oxfd,    Oxff,    Oxff,    Oxff,    Oxff,    Oxff,    Oxff,    Oxff,    Oxff,
        /* 0x0e0 */ Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxeb,    Oxe9,    Oxe9,    Oxea,    Oxeb,    Oxfd,    Oxfd,    Oxfd,    Oxfd,
        /* 0x0f0 */ Oxf3,    Oxf1,    Oxf3,    Oxf3,    Oxfd,    Oxfd,    Oxf6,    Oxf7,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxfd,    Oxff,    Oxff
    ]
    lazy var twoByteDecoder: Free86.OpcodeDecoder = [
        //          0x0      0x1      0x2      0x3      0x4      0x5      0x6      0x7      0x8      0x9      0xa      0xb      0xc      0xd      0xe      0xf
        /* 0x000 */ Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc7,  Ox0fc7,  Ox0fcf,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0f0f,  
        /* 0x010 */ Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  
        /* 0x020 */ Ox0fc1,  Ox0fc7,  Ox0fc1,  Ox0fc1,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  
        /* 0x030 */ Ox0fc7,  Ox0fcf,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  
        /* 0x040 */ Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  
        /* 0x050 */ Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  
        /* 0x060 */ Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  
        /* 0x070 */ Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  
        /* 0x080 */ Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  Ox0f8f,  
        /* 0x090 */ Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  
        /* 0x0a0 */ Ox0fcf,  Ox0fcf,  Ox0fcf,  Ox0fc1,  Ox0fba,  Ox0fc1,  Ox0fc7,  Ox0fc7,  Ox0fcf,  Ox0fcf,  Ox0fc7,  Ox0fc1,  Ox0fba,  Ox0fc1,  Ox0fc7,  Ox0fc1,  
        /* 0x0b0 */ Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc7,  Ox0fc7,  Ox0fba,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  Ox0fc1,  
        /* 0x0c0 */ Ox0fc1,  Ox0fc1,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fc7,  Ox0fcf,  Ox0fcf,  Ox0fcf,  Ox0fcf,  Ox0fcf,  Ox0fcf,  Ox0fcf,  Ox0fcf,  
        /* 0x0d0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x0e0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid,
        /* 0x0f0 */ invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid, invalid
    ]
}
