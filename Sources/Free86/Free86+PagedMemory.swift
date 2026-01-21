extension Free86: PagedMemory {
    func ld8FromReadonly() {
        let ld8FromReadonly: () -> Byte = {
             0
        }
    }
    func ld16FromReadonly() {
        let ld16FromReadonly: () -> Word = {
             0
        }
    }
    func ldFromReadonly() {
        let ldFromReadonly: () -> DWord = {
             0
        }
    }
    func ld8FromUserReadonly() {
        let ld8FromUserReadonly: () -> Byte = {
             0
        }
    }
    func ld16FromUserReadonly() {
        let ld16FromUserReadonly: () -> Word = {
             0
        }
    }
    func ldFromUserReadonly() {
        let ldFromUserReadonly: () -> DWord = {
             0
        }
    }
    func ld8FromUserWritable() {
        let ld8FromUserWritable: () -> Byte = {
             0
        }
    }
    func ld16FromUserWritable() {
        let ld16FromUserWritable: () -> Word = {
             0
        }
    }
    func ldFromUserWritable() {
        let ldFromUserWritable: () -> DWord = {
             0
        }
    }
    func st8InReadonly(byte: Byte) {
        let st8InReadonly: (Byte) -> () = { byte in
        }
    }
    func st16InReadonly(word: Word) {
        let st16InReadonly: (Word) -> () = { word in
        }
    }
    func stInReadonly(dword: DWord) {
        let stInReadonly: (DWord) -> () = { dword in
        }
    }
    func st8InUserReadonly(byte: Byte) {
        let st8InUserReadonly: (Byte) -> () = { byte in
        }
    }
    func st16InUserReadonly(word: Word) {
        let st16InUserReadonly: (Word) -> () = { word in
        }
    }
    func stInUserReadonly(dword: DWord) {
        let stInUserReadonly: (DWord) -> () = { dword in
        }
    }

    func translate(_ linear: LinearAddress, writable: Bool, user: Bool) throws {
        if !cr0.isPagingEnabled {
            tlbUpdate(linear: linear & ~0xfff, with: linear & ~0xfff, writable: true, user: false)
            return
        }
        /// paging enabled
        var errorCode: DWord = 0
        let pdeAddress = cr3.pageDirectoryBase + linear.pageDirectoryIndex
        var pde: PageTableEntry = ld(from: pdeAddress)
        if pde.isPresent {
            let pteAddress = pde.pageFrameAddress + linear.pageTableIndex
            var pte: PageTableEntry = ld(from: pteAddress)
            if pde.isPresent {
                let supervisor = !user
                let pxe = pde & pte
                /// error: user request and page not user-accessible
                if user && !pxe.isUser {
                    errorCode.setFlag(.P)
                }
                /// error: writable request and page not writable and not supervisor or (supervisor and) WP is set
                if writable && !pxe.isWritable && (!supervisor || cr0.isFlagRaised(.WP)) {
                    errorCode.setFlag(.P)
                }
                if errorCode == 0 {
                    if !pde.accessed {
                        pde.setFlag(.A)
                        st(at: pdeAddress, dword: pde)
                    }
                    let isBlankPage = writable && !pte.isDirty
                    if isBlankPage && !pte.accessed {
                        pte.setFlag(.A)
                        if isBlankPage {
                            pte.setFlag(.D)
                        }
                        st(at: pteAddress, dword: pte)
                    }
                    var wFlag = false
                    var uFlag = false
                    if pte.isDirty && (pxe.isWritable || supervisor) {
                        wFlag = true
                    }
                    if pxe.isUser {
                        uFlag = true
                    }
                    tlbUpdate(linear: linear & ~0xfff, with: pte & ~0xfff, writable: wFlag, user: uFlag)
                    return
                }
            }
        }
        /// page fault
        cr2 = linear
        if writable {
            errorCode.setFlag(.W)
        }
        if user {
            errorCode.setFlag(.U)
        }
        throw Interrupt(.PF, errorCode: errorCode)
    }
}
