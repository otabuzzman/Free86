extension Free86: PagedMemory {
    func ld8FromReadonly() {
        let ld8FromReadonly: () throws -> Byte = { [self] in
            try translate(lax, writable: false, user: false)
            let hash = tlbReadonlyCplX[lax.pageTablesIndices]
            return memory.ld8(from: lax ^ DWord(hash))
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
        let ld8FromUserReadonly: () throws -> Byte = { [self] in
            try translate(lax, writable: false, user: cpl == 3)
            let hash = tlbReadonly[lax.pageTablesIndices]
            return memory.ld8(from: lax ^ DWord(hash))
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
        let ld8FromUserWritable: () throws -> Byte = { [self] in
            try translate(lax, writable: true, user: cpl == 3)
            let hash = tlbWritable[lax.pageTablesIndices]
            return memory.ld8(from: lax ^ DWord(hash))
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
    func st8InWritable(byte: Byte) {
        let st8InWritable: (Byte) throws -> () = { [self] byte in
            try translate(lax, writable: true, user: false)
            let hash = tlbWritableCplX[lax.pageTablesIndices]
            memory.st8(at: lax ^ DWord(hash), byte: byte)
        }
    }
    func st16InWritable(word: Word) {
        let st16InWritable: (Word) -> () = { word in
        }
    }
    func stInWritable(dword: DWord) {
        let stInWritable: (DWord) -> () = { dword in
        }
    }
    func st8InUserWritable(byte: Byte) {
        let st8InUserWritable: (Byte) throws -> () = { [self] byte in
            try translate(lax, writable: true, user: cpl == 3)
            let hash = tlbWritable[lax.pageTablesIndices]
            memory.st8(at: lax ^ DWord(hash), byte: byte)
        }
    }
    func st16InUserWritable(word: Word) {
        let st16InUserWritable: (Word) -> () = { word in
        }
    }
    func stInUserWritable(dword: DWord) {
        let stInUserWritable: (DWord) -> () = { dword in
        }
    }
}
