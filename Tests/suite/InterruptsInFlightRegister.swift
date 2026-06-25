import Testing
@testable import Free86

@Test("interrupts in-flight register flags positions")
func interruptsInFlightRegisterFlagsPositions() {
    #expect(InterruptsInFlightRegisterFlag.internal.rawValue == 0)
    #expect(InterruptsInFlightRegisterFlag.NMI.rawValue == 1)
    #expect(InterruptsInFlightRegisterFlag.INTR.rawValue == 2)
    #expect(InterruptsInFlightRegisterFlag.software.rawValue == 3)
}


@Test("interrupts in-flight register set/ check flags")
func interruptsInFlightRegisterCheckFlags() {
    var ifr = InterruptsInFlightRegister(0b1010)
    #expect(ifr.isFlagRaised(.software) == true)
    #expect(ifr.isFlagRaised(.NMI) == true)
    #expect(ifr.isFlagRaised(.INTR) == false)
    #expect(ifr.isFlagRaised(.internal) == false)
    ifr.setFlag(.INTR)
    #expect(ifr.isFlagRaised(.INTR) == true)
    ifr.setFlag(.INTR, .zero)
    #expect(ifr.isFlagRaised(.INTR) == false)
}

@Test("interrupts in-flight priority checks",
    arguments: [
        (ifr: 0b0000 as InterruptsInFlightRegister, flag: InterruptsInFlightRegisterFlag.software, expected: false),
        (ifr: 0b0001 as InterruptsInFlightRegister, flag: InterruptsInFlightRegisterFlag.internal, expected: false),
        (ifr: 0b0010 as InterruptsInFlightRegister, flag: InterruptsInFlightRegisterFlag.NMI, expected: false),
        (ifr: 0b0010 as InterruptsInFlightRegister, flag: InterruptsInFlightRegisterFlag.INTR, expected: true),
        (ifr: 0b1000 as InterruptsInFlightRegister, flag: InterruptsInFlightRegisterFlag.software, expected: false),
        (ifr: 0b0011 as InterruptsInFlightRegister, flag: InterruptsInFlightRegisterFlag.software, expected: true),
        (ifr: 0b1010 as InterruptsInFlightRegister, flag: InterruptsInFlightRegisterFlag.software, expected: true),
        (ifr: 0b1111 as InterruptsInFlightRegister, flag: InterruptsInFlightRegisterFlag.software, expected: true)
    ]
)
func interruptsInFlightRegisterPriorityChecks(tuple: (ifr: InterruptsInFlightRegister, flag: InterruptsInFlightRegisterFlag, expected: Bool)) {
    #expect(tuple.ifr.isActivePriority(tuple.flag) == tuple.expected)
}
