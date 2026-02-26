extension Free86 {
    func obtainOpcode() {
        eip = eip &+ far &- farStart
        eipLinear = cr0.isRealOrV86Mode() ? (csBase &+ eip) & 0xfffff : csBase &+ eip
        far = farStart = tlbLookup(eipLinear, 0)
        opcode = fetch8()
        let pageOffset = eipLinear & 0xfff
        let n = instructionLength(opcode)
        if (pageOffset &+ n) > 4096 {  // instruction extends page boundary
            far = farStart = memoryCount  // point FAR to buffer on top of memory
            for u in 0..<n {  // copy instruction bytewise to buffer
                lax = eipLinear &+ u  // paged memory functions expect address in LAX
                st8Direct(far &+ u, try ld8ReadonlyCpl3())  // copy [LAX] to physical [FAR]
            }
            far = far &+ 1  // adjust FAR for upcomming fetches from buffer
        }
    }
    func instructionLength(_ opcode: DWord) -> Int {
    }
    func modRMBytesNumber() -> Int {
    }
}
