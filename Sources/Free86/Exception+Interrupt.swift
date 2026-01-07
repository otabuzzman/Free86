enum Exception: Byte {
    case DE
    case DB
    case NonMaskableInterrupt // PinIO
    case BP = 3
    case OF
    case BR
    case UD
    case NM
    case DF // error code
    case CoprocessorSegmentOverrun
    case TS // error code
    case NP // error code
    case SS // error code
    case GP // error code
    case PF // error code
    // Reserved
    case MF = 16
}

struct Interrupt: Error, Equatable {
    var vector: Byte
    var errorCode: DWord

    init(_ vector: Byte, errorCode: DWord = 0) {
        self.vector = vector
        self.errorCode = errorCode
    }
}

extension Interrupt {
    init(_ exception: Exception, errorCode: DWord = 0) {
        vector = exception.rawValue
        self.errorCode = errorCode
    }
}
