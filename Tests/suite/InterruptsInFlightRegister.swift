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
    ifr.setFlag(.NMI)
    #expect(ifr.isFlagRaised(.NMI) == true)
    #expect(ifr.isFlagRaised(.INTR) == false)
    #expect(ifr.isFlagRaised(.internal) == false)
    ifr.setFlag(.INTR)
    #expect(ifr.isRaised(.INTR) == true)
    ifr.setFlag(.INTR, .zero)
    #expect(ifr.isRaised(.INTR) == false)
}

@Test("interrupts in-flight priority checks")
func interruptsInFlightRegisterPriorityChecks() {
    var ifr = InterruptsInFlightRegister(0)
    #expect(ifr.noHigherPriority(than: .INTR) == true)
    #expect(ifr.noHigherPriority(than: .NMI) == true)
    ifr.setFlag(.internal)
    #expect(ifr.noHigherPriority(than: .internal) == true)
    ifr.setFlag(.internal, .zero)
    ifr.setFlag(.NMI)
    #expect(ifr.noHigherPriority(than: .NMI) == true)
    #expect(ifr.noHigherPriority(than: .INTR) == false)
    ifr.setFlag(.internal)
    #expect(ifr.noHigherPriority(than: .INTR) == false)
    ifr.setFlag(.INTR)
    #expect(ifr.noHigherPriority(than: .INTR) == false)
    ifr.setFlag(.internal, .zero)  // .NMI now highest (current)
    #expect(ifr.noHigherPriority(than: .INTR) == false)
    ifr.setFlag(.current, .zero)
    #expect(ifr.noHigherPriority(than: .NMI) == true)
}
