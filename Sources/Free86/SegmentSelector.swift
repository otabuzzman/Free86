typealias SegmentSelector = Word

enum SegmentSelectorFlag: Int {
    case TI = 2
}

extension SegmentSelector {
    func isFlagRaised(_ flag: SegmentSelectorFlag) -> Bool {
        self.isBitRaised(flag.rawValue)
    }
    mutating func setFlag(_ flag: SegmentSelectorFlag, _ value: Int = 1) {
        self.setBit(flag.rawValue, value)
    }
}

extension SegmentSelector {
    var index: Self {
        self & ~0b0111
    }
    var isGDT: Bool {
        !isLDT
    }
    var isLDT: Bool {
        isFlagRaised(.TI)
    }
    var rpl: Int {
        Int(self & 0b0011)
    }
}
