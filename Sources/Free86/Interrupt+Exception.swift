public enum Exception: Byte {
    case DE
    case DB
    case NonMaskableInterrupt  // PinIO
    case BP
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
    public internal(set) var id: Byte
    public internal(set) var errorCode: DWord

    public init(_ vector: Byte, errorCode: DWord = 0) {
        self.id = vector
        self.errorCode = errorCode
    }
}

extension Interrupt {
    public enum Class: Int {
        case begnin
        case contributory
        case pagefault
        case doublefault
        case none
    }
    public var `class`: Class {
        switch self.id {
        case 1, 2, 3, 4, 5, 6, 7:
            return .begnin
        case 0, 9, 10, 11, 12, 13:
            return .contributory
        case 14:
            return .pagefault
        case 8:
            return .doublefault
        default:
            return .none
        }
    }
    init(_ vector: Exception, errorCode: DWord = 0) {
        self.init(vector.rawValue, errorCode: errorCode)
    }
}

extension Interrupt: CustomStringConvertible {
    public var description: String {
        "Interrupt id \(id), error code \(errorCode)"
    }
}
