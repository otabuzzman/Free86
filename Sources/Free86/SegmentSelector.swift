extension Word: SegmentSelector {}

protocol SegmentSelector {
    var index: Self { get }
    var isGDT: Bool { get }
    var isLDT: Bool { get }
    var rpl: Self { get }
}

enum SegmentSelectorFlag: Int {
    case TI = 2
}

extension SegmentSelector where Self == Word {
    var index: Self {
        self >> 3
    }
    var isGDT: Bool {
        !isLDT
    }
    var isLDT: Bool {
        isFlagRaised(.TI)
    }
    var rpl: Self {
        self & 0b0011
    }
}

extension SegmentSelector where Self == Word {
    func isFlagRaised(_ flag: SegmentSelectorFlag) -> Bool {
        self.isBitRaised(flag.rawValue)
    }
}
