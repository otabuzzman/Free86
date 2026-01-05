import Testing
@testable import Free86

@Test("trigger single pin")
func triggerSinglePin() async throws {
    var INTR = Pin<Bool>()
    // trigger/ probe
    try await INTR.trigger(true)
    #expect(try await INTR.probe() == true)

    // multiple trigger ok due to default option .allowMultipleTriggers
    try await INTR.trigger(false)
    try await INTR.trigger(true)

    // multiple trigger throw without .allowMultipleTriggers
    INTR = Pin<Bool>(false, options: [])
    try await INTR.trigger(true)
    await #expect(throws: Pin<Bool>.Event.probeIsPending) {
        try await INTR.trigger(true)
    }
    #expect(try await INTR.probe() == true)

    // multiple probes throw
    await #expect(throws: Pin<Bool>.Event.noPendingProbe) {
        try await INTR.probe()
    }
}

@Test("trigger multiple pins")
func triggerMultiplePins() async throws {
    var DB8 = Pin<Byte>()
    // trigger/ probe
    try await DB8.trigger(0x42)
    #expect(try await DB8.probe() == 0x42)

    // multiple trigger ok due to default option .allowMultipleTriggers
    try await DB8.trigger(0x55)
    try await DB8.trigger(0xAA)

    // multiple trigger throw without .allowMultipleTriggers
    DB8 = Pin<Byte>(16, options: [])
    try await DB8.trigger(32)
    await #expect(throws: Pin<Byte>.Event.probeIsPending) {
        try await DB8.trigger(32)
    }
    #expect(try await DB8.probe() == 32)

    // multiple probes throw
    await #expect(throws: Pin<Byte>.Event.noPendingProbe) {
        try await DB8.probe()
    }
}
