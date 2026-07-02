import Testing
@testable import Free86

@Test("interrupts in-flight register flags positions")
func interruptsInFlightRegisterFlagsPositions() {
    #expect(InterruptsInFlightRegisterFlag.internal.rawValue == 0)
    #expect(InterruptsInFlightRegisterFlag.NMI.rawValue == 1)
    #expect(InterruptsInFlightRegisterFlag.INTR.rawValue == 2)
}


@Test("interrupts in-flight register set/ check flags")
func interruptsInFlightRegisterCheckFlags() {
    var ifr = InterruptsInFlightRegister()
    #expect(ifr.current == nil)
    ifr.setFlag(.NMI)
    #expect(ifr.current == .NMI)
    #expect(ifr.isFlagRaised(.NMI) == true)
    #expect(ifr.isFlagRaised(.INTR) == false)
    #expect(ifr.isFlagRaised(.internal) == false)
    ifr.setFlag(.INTR)
    #expect(ifr.current == .NMI)
    #expect(ifr.isFlagRaised(.INTR) == true)
    ifr.setFlag(.INTR, .zero)
    #expect(ifr.isFlagRaised(.INTR) == false)
}
