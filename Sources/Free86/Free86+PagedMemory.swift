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
}
