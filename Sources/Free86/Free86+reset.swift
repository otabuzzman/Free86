extension Free86 {
    func reset() {
        /// chip state (PM (1986), 10.1, Intel 64 IA-32 SDM, Vol. 3A, 11.1.1)
        for r in 0..<regs.count {
            regs[r] = .init(0)
        }
        eflags = 0x2

        eip = 0xFFF0

        for s in 0..<segs.count {
            segs[s] = .init(0, .init(0, 0, .zero, 0))
        }
        segs[.CS] = .init(0, .init(0xFFFF0000, 0, .CodeExOnly, 0))
        idt = .init(0, .init(0x03FF, 0, .zero, 0))

        cr0 = 0
        cr0.setFlag(.ET)  // 80387 present (Vol. 3A, p. 2-16)

        /// emulator state
        halted = false
    }
}
