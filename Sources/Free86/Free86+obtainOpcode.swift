extension Free86 {
    func obtainOpcode() throws {
        eip = eip &+ far &- farStart
        eipLinear = cr0.isRealOrV86Mode ? (csBase &+ eip) & 0xfffff : csBase &+ eip
        far = try tlbLookup(linear: eipLinear, writable: false)
        farStart = far
        opcode = DWord(fetch8())
        let offset = eipLinear & 0xfff
        let n = try instruction.length()
        if (Int(offset) &+ n) > 4096 {  // instruction extends page boundary
            far = memoryCount  // point FAR to buffer on top of memory
            farStart = far
            for i in 0..<n {  // copy instruction bytewise to buffer
                lax = eipLinear &+ DWord(i)  // paged memory functions expect address in LAX
                memory.st8(at: far &+ DWord(i), byte: try ld8ReadonlyCpl3())  // copy [LAX] to physical [FAR]
            }
            far = far &+ 1  // adjust FAR for upcomming fetches from buffer
        }
    }
}
