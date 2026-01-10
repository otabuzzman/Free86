// swift-tools-version: 6.2
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "Free86",
    products: [
        // Products define the executables and libraries a package produces, making them visible to other packages.
        .library(
            name: "Free86",
            targets: ["Free86"],
        ),
    ],
    targets: [
        // Targets are the basic building blocks of a package, defining a module or a test suite.
        // Targets can depend on other targets in this package and products from dependencies.
        .target(
            name: "Free86",
            dependencies: [],
            sources: [
                "DataTypes.swift",
                "SegmentDescriptor.swift",
                "SegmentSelector.swift",
                "SegmentRegister.swift",
                "LogicalAddress.swift",
                "LinearAddress.swift",
                "PhysicalAddress.swift",
                "PageTableEntry.swift",
                "ModRM+SIB.swift",
                "GeneralRegister.swift",
                "ExtendedFlags.swift",
                "ControlRegister.swift",
                "Exception+Interrupt.swift",
                "InstructionPrefixRegister.swift",
                "Free86.swift",
                "Free86+Fedex.swift",
                "PinIO.swift",
            ],
            swiftSettings: [
                .define("DEBUG", .when(configuration: .debug)),
            ],
        ),
        .testTarget(
            name: "suite",
            dependencies: ["Free86"],
            sources: [
                "DataTypes.swift",
                "SegmentDescriptor.swift",
                "SegmentSelector.swift",
                "LinearAddress.swift",
                "PageTableEntry.swift",
                "ModRM+SIB.swift",
                "GeneralRegister.swift",
                "ExtendedFlags.swift",
                "ControlRegister.swift",
                "Exception+Interrupt.swift",
                "InstructionPrefixRegister.swift",
                "PinIO.swift",
           ],
            swiftSettings: [
                .define("DEBUG", .when(configuration: .debug)),
            ],
        ),
    ]
)
