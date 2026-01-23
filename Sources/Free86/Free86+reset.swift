extension Free86 {
    func reset() {
        /// chip state (Intel 64 IA-32 SDM, Vol. 3A, 11.1.1)
        regs = .init(repeating: GeneralRegister(0), count: 8)
        eflags = 0x2

        eip = 0xFFF0

        segs = .init(repeating: SegmentRegister(), count: 7)
        segs[.CS].descriptorCache.base = 0xFFFF0000
        idt = SegmentRegister()
        idt.descriptorCache.base = 0x03FF

        cr0 = 0
        cr0.setFlag(.ET)  // 80387 present (Vol. 3A, p. 2-16)
        /// emulator state
        halted = false
    }
}
