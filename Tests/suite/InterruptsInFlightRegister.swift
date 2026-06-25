import Testing
@testable import Free86

@Test("interrupts in-flight register flags positions")
func interruptsInFlightRegisterFlagsPositions() {
    #expect(InterruptsInFlightRegister.Name.internal.rawValue == 0)
    #expect(InterruptsInFlightRegister.Name.NMI.rawValue == 1)
    #expect(InterruptsInFlightRegister.Name.INTR.rawValue == 2)
}


@Test("interrupts in-flight register set/ check flags")
func interruptsInFlightRegisterCheckFlags() {
    var ifr = InterruptsInFlightRegister()
    ifr.increment(.NMI)
    #expect(ifr.isRaised(.NMI) == true)
    #expect(ifr.isRaised(.INTR) == false)
    #expect(ifr.isRaised(.internal) == false)
    ifr.increment(.INTR)
    #expect(ifr.isRaised(.INTR) == true)
    ifr.decrement(.INTR)
    #expect(ifr.isRaised(.INTR) == false)
}

@Test("interrupts in-flight priority checks")
func interruptsInFlightRegisterPriorityChecks() {
    var ifr = InterruptsInFlightRegister()
    #expect(ifr.noHigherPriority(than: .INTR) == true)
    #expect(ifr.noHigherPriority(than: .NMI) == true)
    ifr.increment(.internal)
    #expect(ifr.noHigherPriority(than: .internal) == true)
    ifr.decrement(.internal)
    ifr.increment(.NMI)
    #expect(ifr.noHigherPriority(than: .NMI) == true)
    #expect(ifr.noHigherPriority(than: .INTR) == false)
    ifr.increment(.internal)
    #expect(ifr.noHigherPriority(than: .INTR) == false)
    ifr.increment(.INTR)
    #expect(ifr.noHigherPriority(than: .INTR) == false)
    ifr.decrement(.internal)  // .NMI now highest (current)
    #expect(ifr.noHigherPriority(than: .INTR) == false)
    ifr.decrement(.current)
    #expect(ifr.noHigherPriority(than: .INTR) == true)
}
