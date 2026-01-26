extension Free86 {
    var cr0: CR0 {
        get { _cr0 }
        set {
            /// must flush tlb on change of flags 31, 16, or 0
            var mask: CR0 = 0
            mask.setFlag(.PG)
            mask.setFlag(.WP)
            mask.setFlag(.PE)
            if newValue & mask != _cr0 & mask {
                tlbFlush()
            }
            _cr0 = newValue
            _cr0.setFlag(.ET) // keep bit 4 raised (80387 present)
        }
    }
    var cr3: CR3 {
        get { _cr3 }
        set {
            if cr0.isPagingEnabled {
                tlbFlush()
            }
            _cr3 = newValue
        }
    }
    var cpl: Int {
        get { _cpl }
        set {
            if newValue == 3 {
                tlbReadonly = tlbReadonlyCpl3
                tlbWritable = tlbWritableCpl3
            } else {
                tlbReadonly = tlbReadonlyCplX
                tlbWritable = tlbWritableCplX
            }
            _cpl = newValue
        }
    }
}
