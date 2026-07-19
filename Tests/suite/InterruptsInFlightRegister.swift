import Testing
@testable import Free86

@Test("interrupts in-flight register flags positions")
func interruptsInFlightRegisterFlagsPositions() {
    #expect(InterruptsInFlightRegisterFlag.FV.rawValue == 7)
    #expect(InterruptsInFlightRegisterFlag.NMI.rawValue == 10)
}


@Test("interrupts in-flight register set/ check flags")
func interruptsInFlightRegisterCheckFlags() {
    var ifr = InterruptsInFlightRegister()
    ifr.setFlag(.FV)
    #expect(ifr.isFlagRaised(.FV) == true)
    ifr.setFlag(.NMI)
    #expect(ifr.isFlagRaised(.NMI) == true)
    #expect(ifr.fex == 0)
    ifr.fex = 4
    #expect(ifr.fex == 4)
    ifr.fex = 2
    #expect(ifr.fex == 2)
    #expect(ifr.isFlagRaised(.NMI) == true)
    ifr.setFlag(.FV, .zero)
    #expect(ifr.isFlagRaised(.FV) == false)
    #expect(ifr.fex == 2)
}
