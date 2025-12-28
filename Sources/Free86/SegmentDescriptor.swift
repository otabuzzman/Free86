struct SegmentDescriptor {
    enum UpperBits {
        static let g = 23 // granularity, 1 = limit in 4 kB units
        static let d = 22 // 0 = 16-bit segment, 1 = 32-bit segment
        static let p = 15 // 1 = segment is present
        static let s = 12 // 0 = system segment
    }
    var upper: DWord
    var lower: DWord
}

extension SegmentDescriptor {
    var base: DWord {
        lower.upperHalf |
        (upper & 0x0000_00FF) << 16 |
        upper & 0xFF00_0000
    }
    var limit: DWord {
        let value = lower.lowerHalf | upper & 0x000F_0000
        return upper.isBitSet(UpperBits.g) ? (value << 12) | 0xfff : value
    }
    var type: DWord {
        (upper & 0x0000_0F00) >> 8
    }
    var dpl: DWord {
        (upper & 0x0000_6000) >> 13
    }
    var isSystemSegment: Bool {
        !upper.isBitSet(UpperBits.s)
    }
}
