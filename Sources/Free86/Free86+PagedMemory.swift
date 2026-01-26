extension Free86: PagedMemory {
    func ld8ReadonlyCplX() throws -> Byte {
        var hash = tlbReadonlyCplX[lax.pageTablesIndices]
        if hash == -1 {
            try translate(lax, writable: false, user: false)
            hash = tlbReadonlyCplX[lax.pageTablesIndices]
        }
        return memory.ld8(from: lax ^ DWord(hash))
    }
    func ld16ReadonlyCplX() throws -> Word {
        let hash = tlbReadonlyCplX[lax.pageTablesIndices]
        if (((hash | Int(lax)) & 1) != 0) {
            var word: Word = 0
            word.lowerHalf = Word(try ld8ReadonlyCplX())
            lax += 1
            word.upperHalf = Word(try ld8ReadonlyCplX())
            lax -= 1
            return word
        }
        return memory.ld16(from: lax ^ DWord(hash))
    }
    func ldReadonlyCplX() throws -> DWord {
        let hash = tlbReadonlyCplX[lax.pageTablesIndices]
        if (((hash | Int(lax)) & 3) != 0) {
            var dword: DWord = 0
            dword = DWord(try ld16ReadonlyCplX())
            lax += 2
            dword |= DWord(try ld16ReadonlyCplX()) << 16
            lax -= 3
            return dword
        }
        return memory.ld(from: lax ^ DWord(hash))
    }
    func ld64ReadonlyCplX() throws -> QWord {
        let hash = tlbReadonlyCplX[lax.pageTablesIndices]
        if (((hash | Int(lax)) & 3) != 0) {
            var qword: QWord = 0
            qword = QWord(try ldReadonlyCplX())
            lax += 4
            qword |= QWord(try ldReadonlyCplX()) << 32
            lax -= 7
            return qword
        }
        return memory.ld(from: lax ^ DWord(hash))
    }
    func ld8ReadonlyCpl3() throws -> Byte {
        if cr0.isRealOrV86Mode {
            return memory.ld8(from: lax)
        }
        var hash = tlbReadonly[lax.pageTablesIndices]
        if hash == -1 {
            try translate(lax, writable: false, user: cpl == 3)
            hash = tlbReadonly[lax.pageTablesIndices]
        }
        return memory.ld8(from: lax ^ DWord(hash))
    }
    func ld16ReadonlyCpl3() throws -> Word {
        if cr0.isRealOrV86Mode {
            return memory.ld16(from: lax)
        }
        let hash = tlbReadonly[lax.pageTablesIndices]
        if (((hash | Int(lax)) & 1) != 0) {
            var word: Word = 0
            word.lowerHalf = Word(try ld8ReadonlyCpl3())
            lax += 1
            word.upperHalf = Word(try ld8ReadonlyCpl3())
            lax -= 1
            return word
        }
        return memory.ld16(from: lax ^ DWord(hash))
    }
    func ldReadonlyCpl3() throws -> DWord {
        if cr0.isRealOrV86Mode {
            return memory.ld(from: lax)
        }
        let hash = tlbReadonly[lax.pageTablesIndices]
        if (((hash | Int(lax)) & 3) != 0) {
            var dword: DWord = 0
            dword = DWord(try ld16ReadonlyCpl3())
            lax += 2
            dword |= DWord(try ld16ReadonlyCpl3()) << 16
            lax -= 3
            return dword
        }
        return memory.ld(from: lax ^ DWord(hash))
    }
    func ld8WritableCpl3() throws -> Byte {
        if cr0.isRealOrV86Mode {
            return memory.ld8(from: lax)
        }
        var hash = tlbWritable[lax.pageTablesIndices]
        if hash == -1 {
            try translate(lax, writable: true, user: cpl == 3)
            hash = tlbWritable[lax.pageTablesIndices]
        }
        return memory.ld8(from: lax ^ DWord(hash))
    }
    func ld16WritableCpl3() throws -> Word {
        if cr0.isRealOrV86Mode {
            return memory.ld16(from: lax)
        }
        let hash = tlbWritable[lax.pageTablesIndices]
        if (((hash | Int(lax)) & 1) != 0) {
            var word: Word = 0
            word.lowerHalf = Word(try ld8WritableCpl3())
            lax += 1
            word.upperHalf = Word(try ld8WritableCpl3())
            lax -= 1
            return word
        }
        return memory.ld16(from: lax ^ DWord(hash))
    }
    func ldWritableCpl3() throws -> DWord {
        if cr0.isRealOrV86Mode {
            return memory.ld(from: lax)
        }
        let hash = tlbWritable[lax.pageTablesIndices]
        if (((hash | Int(lax)) & 3) != 0) {
            var dword: DWord = 0
            dword = DWord(try ld16WritableCpl3())
            lax += 2
            dword |= DWord(try ld16WritableCpl3()) << 16
            lax -= 3
            return dword
        }
        return memory.ld(from: lax ^ DWord(hash))
    }
    func st8WritableCplX(byte: Byte) throws {
        var hash = tlbWritableCplX[lax.pageTablesIndices]
        if hash == -1 {
            try translate(lax, writable: true, user: false)
            hash = tlbWritableCplX[lax.pageTablesIndices]
        }
        memory.st8(at: lax ^ DWord(hash), byte: byte)
    }
    func st16WritableCplX(word: Word) throws {
        let hash = tlbWritableCplX[lax.pageTablesIndices]
        if (((hash | Int(lax)) & 1) != 0) {
            try st8WritableCplX(byte: Byte(word.lowerHalf))
            lax += 1
            try st8WritableCplX(byte: Byte(word.upperHalf))
            lax -= 1
        } else {
            memory.st16(at: lax ^ DWord(hash), word: word)
        }
    }
    func stWritableCplX(dword: DWord) throws {
        let hash = tlbWritableCplX[lax.pageTablesIndices]
        if (((hash | Int(lax)) & 3) != 0) {
            try st16WritableCplX(word: Word(dword))
            lax += 2
            try st16WritableCplX(word: Word(dword >> 16))
            lax -= 3
        } else {
            memory.st(at: lax ^ DWord(hash), dword: dword)
        }
    }
    func st8WritableCpl3(byte: Byte) throws {
        if cr0.isRealOrV86Mode {
            memory.st8(at: lax, byte: byte)
        }
        var hash = tlbWritable[lax.pageTablesIndices]
        if hash == -1 {
            try translate(lax, writable: true, user: cpl == 3)
            hash = tlbWritable[lax.pageTablesIndices]
        }
        memory.st8(at: lax ^ DWord(hash), byte: byte)
    }
    func st16WritableCpl3(word: Word) throws {
        if cr0.isRealOrV86Mode {
            memory.st16(at: lax, word: word)
        }
        let hash = tlbWritable[lax.pageTablesIndices]
        if (((hash | Int(lax)) & 1) != 0) {
            try st8WritableCpl3(byte: Byte(word.lowerHalf))
            lax += 1
            try st8WritableCpl3(byte: Byte(word.upperHalf))
            lax -= 1
        } else {
            memory.st16(at: lax ^ DWord(hash), word: word)
        }
    }
    func stWritableCpl3(dword: DWord) throws {
        if cr0.isRealOrV86Mode {
            memory.st(at: lax, dword: dword)
        }
        let hash = tlbWritable[lax.pageTablesIndices]
        if (((hash | Int(lax)) & 3) != 0) {
            try st16WritableCpl3(word: Word(truncatingIfNeeded: dword))
            lax += 2
            try st16WritableCpl3(word: Word(truncatingIfNeeded: dword >> 16))
            lax -= 3
        } else {
            memory.st(at: lax ^ DWord(hash), dword: dword)
        }
    }
}
