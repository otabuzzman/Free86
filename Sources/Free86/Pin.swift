actor Pin<Signal>  {
    private var signal: Signal
    private var options: [Option]

    private var pending = false

    enum Event: Error {
        case probeIsPending
        case noPendingProbe
    }

    enum Option {
        case allowMultipleTriggers
    }

    init(_ signal: Signal, options: [Option]) {
        self.signal = signal
        self.options = options
    }

    func trigger(_ signal: Signal) throws {
        if pending && !options.contains(.allowMultipleTriggers) {
            throw Event.probeIsPending
        }
        self.signal = signal
        pending = true
    }
    
    func probe() throws -> Signal {
        if !pending {
            throw Event.noPendingProbe
        }
        pending = false
        return signal
    }
}

extension Pin where Signal == Bool {
    init(_ signal: Signal = false) {
        self.init(signal, options: [.allowMultipleTriggers])
    }
}

extension Pin where Signal: UnsignedInteger {
    init(_ signal: Signal = .zero) {
        self.init(signal, options: [.allowMultipleTriggers])
    }
}
