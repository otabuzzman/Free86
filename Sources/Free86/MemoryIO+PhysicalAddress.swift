/// only byte-type IO ports permitted for memory-mapped IO since memory is also byte-typed
class IOPortBank<A: PhysicalAddress, P: IOPort>: IsolatedIO<A, P>, MemoryBank where P.Element == Byte {
    override subscript(address: A) -> P.Element {
        get {
             P.Element(super[address & A(A.bankOffsetMask)])
        }
        set(iodata) {
             super[address & A(A.bankOffsetMask)] = P.Element(iodata)
        }
    }
}

class IsolatedIO<A: PhysicalAddress> {
    private var mapping: [A: AnyIOPort] = [:]

    subscript<T: UnsignedInteger>(address: A) -> T {
        get {
            guard let port = mapping[address] else { return .zero }
            return port.rd()
        }
        set(iodata) {
            guard let port = mapping[address] else { return }
            port.wr(iodata)
        }
    }

    func register<P: IOPort>(port: P, at address: A) {
        mapping.updateValue(AnyIOPort(port), forKey: address)
    }
}

/// type-erasing adapter for any type implementing IOPort
class AnyIOPort {
    private let _rd: () -> Any
    private let _wr: (Any) -> Void

    init<P: IOPort>(_ port: P) {
        _rd = { port.rd() }
        _wr = { iodata in
            guard let _iodata = iodata as? P.Element else { return }
            port.wr(_iodata)
        }
    }

    func rd<T: UnsignedInteger>() -> T {
        guard let iodata = _rd() as? T else { return .zero }
        return iodata
    }

    func wr<T: UnsignedInteger>(_ iodata: T) {
        _wr(iodata)
    }
}

protocol IOPort {
    associatedtype Element: UnsignedInteger
    func rd() -> Element
    func wr(_ iodata: Element)
}

public class MemoryIO<A: PhysicalAddress> {
    public private(set) var count = 0

    private var slot: [AnyBank<A>]

    public init<B: MemoryBank>(defaultBank: B) where B.Address == A {
        slot = [AnyBank<A>](repeating: AnyBank<A>(defaultBank), count: 1024 * 1024)
    }

    public subscript(address: A) -> Byte {
        get {
            let index = Int(address >> A.bankIndexShift)
            let offset = address & A(A.bankOffsetMask)
            return slot[index][offset]
        }
        set(byte) {
            let index = Int(address >> A.bankIndexShift)
            let offset = address & A(A.bankOffsetMask)
            slot[index][offset] = byte
        }
    }

    public func register<B: MemoryBank>(bank: B, at address: A) where B.Address == A {
        let index = Int(address >> A.bankIndexShift)
        slot[index] = AnyBank<A>(bank)
        count += A.bankSize
    }
}

/// type-erasing adapter for any type implementing MemoryBank
class AnyBank<A: PhysicalAddress> {
    private let _get: (A) -> Byte
    private let _set: (A, Byte) -> Void

    subscript(address: A) -> Byte {
        get { _get(address) }
        set(byte) { _set(address, byte) }
    }

    init<B: MemoryBank>(_ bank: B) where B.Address == A {
        var _bank = bank
        _get = { address in _bank[address] }
        _set = { address, byte in _bank[address] = byte }
    }
}

public protocol MemoryBank {
    associatedtype Address: PhysicalAddress
    subscript(address: Address) -> Byte { get set }
}

public class DefaultBank<A: PhysicalAddress>: MemoryBank {
    public init() { }

    public subscript(address: A) -> Byte {
        get { .zero }
        set { }
    }
}

public class RAMBank<A: PhysicalAddress>: MemoryBank {
    private var bank: [Byte]

    public init(fill: Byte = .zero) {
        bank = [Byte](repeating: fill, count: A.bankSize)
    }
    
    public subscript(address: A) -> Byte {
        get { bank[Int(address) & A.bankOffsetMask] }
        set(byte) { bank[Int(address) & A.bankOffsetMask] = byte }
    }
}

class ROMBank<A: PhysicalAddress>: MemoryBank {
    private var bank: [Byte]

    init(bytes: [Byte]) {
        let count = min(bytes.count, A.bankSize)
        bank = [Byte](repeating: .zero, count: A.bankSize)
        bank.replaceSubrange(0..<count, with: bytes.prefix(count))
    }

    subscript(address: A) -> Byte {
        get { bank[Int(address) & A.bankOffsetMask] }
        set { }
    }
}

public protocol PhysicalAddress: UnsignedInteger {
    static var bankSize: Int { get }
    static var bankOffsetMask: Int { get }
    static var bankIndexShift: Int { get }
}

extension DWord: PhysicalAddress {
    public static var bankSize: Int { 0x01000 }  // 4 kB
    public static var bankIndexShift: Int { 12 }
    public static var bankOffsetMask: Int { 0x00FFF }
}

extension QWord: PhysicalAddress {
    public static var bankSize: Int { 0x20000 }  // 128 kB
    public static var bankIndexShift: Int { 17 }
    public static var bankOffsetMask: Int { 0x1FFFF }
}
