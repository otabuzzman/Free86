import Testing
@testable import Free86

@Test("cpu exceptions")
func cpuExceptions() throws {
    #expect(throws: Interrupt(0)) { throw Interrupt(.DE) }
    #expect(throws: Interrupt(1)) { throw Interrupt(.DB) }
    #expect(throws: Interrupt(2)) { throw Interrupt(.NonMaskableInterrupt) }
    #expect(throws: Interrupt(3)) { throw Interrupt(.BP) }
    #expect(throws: Interrupt(4)) { throw Interrupt(.OF) }
    #expect(throws: Interrupt(5)) { throw Interrupt(.BR) }
    #expect(throws: Interrupt(6)) { throw Interrupt(.UD) }
    #expect(throws: Interrupt(7)) { throw Interrupt(.NM) }
    #expect(throws: Interrupt(8)) { throw Interrupt(.DF) }
    #expect(throws: Interrupt(9)) { throw Interrupt(.CoprocessorSegmentOverrun) }
    #expect(throws: Interrupt(10)) { throw Interrupt(.TS) }
    #expect(throws: Interrupt(11)) { throw Interrupt(.NP) }
    #expect(throws: Interrupt(12)) { throw Interrupt(.SS) }
    #expect(throws: Interrupt(13)) { throw Interrupt(.GP) }
    #expect(throws: Interrupt(14)) { throw Interrupt(.PF) }
    #expect(throws: Interrupt(16)) { throw Interrupt(.MF) }
}

@Test("software interrupts")
func softwareInterrupts() throws {
    #expect(throws: Interrupt(0)) { throw Interrupt(0) }
    #expect(throws: Interrupt(255)) { throw Interrupt(255) }
    
    #expect(throws: Interrupt(42, errorCode: 0xDEADBEEF)) { throw Interrupt(42, errorCode: 0xDEADBEEF) }
}
