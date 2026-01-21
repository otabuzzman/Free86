extension Free86: PagedMemory {
    func ld8FromReadonly() {
        var ld8FromReadonly: () -> DWord = {
        }
    }
    func ld16FromReadonly() {
        var ld16FromReadonly: () -> DWord = {
        }
    }
    func ldFromReadonly() {
        var ldFromReadonly: () -> DWord = {
        }
    }
    func ld8FromUserReadonly() {
        var ld8FromUserReadonly: () -> DWord = {
        }
    }
    func ld16FromUserReadonly() {
        var ld16FromUserReadonly: () -> DWord = {
        }
    }
    func ldFromUserReadonly() {
        var ldFromUserReadonly: () -> DWord = {
        }
    }
    func ld8FromUserWritable() {
        var ld8FromUserWritable: () -> DWord = {
        }
    }
    func ld16FromUserWritable() {
        var ld16FromUserWritable: () -> DWord = {
        }
    }
    func ldFromUserWritable() {
        var ldFromUserWritable: () -> DWord = {
        }
    }
    func st8InReadonly() {
        var st8InReadonly: () -> DWord = {
        }
    }
    func st16InReadonly() {
        var st16InReadonly: () -> DWord = {
        }
    }
    func stInReadonly() {
        var stInReadonly: () -> DWord = {
        }
    }
    func st8InUserReadonly() {
        var st8InUserReadonly: () -> DWord = {
        }
    }
    func st16InUserReadonly() {
        var st16InUserReadonly: () -> DWord = {
        }
    }
    func stInUserReadonly() {
        var stInUserReadonly: () -> DWord = {
        }
    }
}

extension Free86: PagedMemory {
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
