enum Exception: Byte {
    case DE
    case DB
    case NonMaskableInterrupt  // PinIO
    case BP = 3
    case OF
    case BR
    case UD
    case NM
    case DF  // error code
    case CoprocessorSegmentOverrun
    case TS  // error code
    case NP  // error code
    case SS  // error code
    case GP  // error code
    case PF  // error code
    case MF = 16  // 80486
    case AC  // 80486
}

public struct Interrupt: Error, Equatable {
    public private(set) var id: Byte
    var errorCode: DWord

    init(_ vector: Byte, errorCode: DWord = 0) {
        self.id = vector
        self.errorCode = errorCode
    }
}

extension Interrupt {
    init(_ exception: Exception, errorCode: DWord = 0) {
        self.init(exception.rawValue, errorCode: errorCode)
    }
    var description: String {
        "Interrupt id \(id), error code \(errorCode))"
    }
}
