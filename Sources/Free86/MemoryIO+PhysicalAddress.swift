/// only byte-type IO ports permitted for memory-mapped IO since memory is also byte-typed
class IOPortBank<A: PhysicalAddress, P: IOPort>: IsolatedIO<A, P>, MemoryBank where P.Element == Byte {
    override subscript(addr: A) -> P.Element {
        get {
             P.Element(super[addr & A(A.bankOffsetMask)])
        }
        set(iodata) {
             super[addr & A(A.bankOffsetMask)] = P.Element(iodata)
        }
    }
}

class IsolatedIO<A: PhysicalAddress, P: IOPort> {
    private var mapping: [A: P] = [:]
    
    subscript(addr: A) -> P.Element {
        get {
            guard let port = mapping[addr] else { return .zero }
            return port.rd()
        }
        set(iodata) {
            guard let port = mapping[addr] else { return }
            port.wr(iodata)
        }
    }
    
    func register(port: P, at addr: A) {
        mapping.updateValue(port, forKey: addr)
    }
}

protocol IOPort {
    associatedtype Element: UnsignedInteger
    func rd() -> Element
    func wr(_ iodata: Element)
}

public class Memory<A: PhysicalAddress> {
    private var slot: [AnyBank<A>]
    
    public init<B: MemoryBank>(defaultBank: B) where B.Address == A {
        slot = [AnyBank<A>](repeating: AnyBank<A>(defaultBank), count: 1024 * 1024)
    }
    
    public subscript(addr: A) -> Byte {
        get {
            let index = Int(addr >> A.bankIndexShift)
            let offset = addr & A(A.bankOffsetMask)
            return slot[index][offset]
        }
        set(byte) {
            let index = Int(addr >> A.bankIndexShift)
            let offset = addr & A(A.bankOffsetMask)
            slot[index][offset] = byte
        }
    }
    
    public func register<B: MemoryBank>(bank: B, at addr: A) where B.Address == A {
        let index = Int(addr >> A.bankIndexShift)
        slot[index] = AnyBank<A>(bank)
    }
}

/// type-erasing adapter for any type implementing MemoryBank
class AnyBank<A: PhysicalAddress> {
    private let _get: (A) -> Byte
    private let _set: (A, Byte) -> Void

    subscript(addr: A) -> Byte {
        get { _get(addr) }
        set(byte) { _set(addr, byte) }
    }

    init<B: MemoryBank>(_ bank: B) where B.Address == A {
        var _bank = bank
        _get = { addr in _bank[addr] }
        _set = { addr, byte in _bank[addr] = byte }
    }
}

public protocol MemoryBank {
    associatedtype Address: PhysicalAddress
    subscript(addr: Address) -> Byte { get set }
}

public class DefaultBank<A: PhysicalAddress>: MemoryBank {
    public init() { }

    public subscript(addr: A) -> Byte {
        get { .zero }
        set { }
    }
}

public class RAMBank<A: PhysicalAddress>: MemoryBank {
    private var bank: [Byte]
    
    public init(fill: Byte = .zero) {
        bank = [Byte](repeating: fill, count: A.bankSize)
    }
    
    public subscript(addr: A) -> Byte {
        get { bank[Int(addr) & A.bankOffsetMask] }
        set(byte) { bank[Int(addr) & A.bankOffsetMask] = byte }
    }
}

class ROMBank<A: PhysicalAddress>: MemoryBank {
    private var bank: [Byte]
    
    init(bytes: [Byte]) {
        let count = min(bytes.count, A.bankSize)
        bank = [Byte](repeating: .zero, count: A.bankSize)
        bank.replaceSubrange(0..<count, with: bytes.prefix(count))
    }
    
    subscript(addr: A) -> Byte {
        get { bank[Int(addr) & A.bankOffsetMask] }
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
    public static var bankOffsetMask: Int { 0x00fff }
}

extension QWord: PhysicalAddress {
    public static var bankSize: Int { 0x20000 }  // 128 kB
    public static var bankIndexShift: Int { 17 }
    public static var bankOffsetMask: Int { 0x1ffff }
}
