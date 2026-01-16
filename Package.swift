// swift-tools-version: 6.2
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "Free86",
    products: [
        // Products define the executables and libraries a package produces, making them visible to other packages.
        .executable(
            name: "linos",
            targets: ["linos"]
        ),
        .executable(
            name: "test386",
            targets: ["test386"]
        ),
        .library(
            name: "Free86",
            targets: ["Free86"],
        ),
    ],
    targets: [
        // Targets are the basic building blocks of a package, defining a module or a test suite.
        // Targets can depend on other targets in this package and products from dependencies.
        .executableTarget(
            name: "linos",
            dependencies: ["Free86"],
            sources: [
                "main.swift",
            ],
            swiftSettings: [
                .define("DEBUG", .when(configuration: .debug)),
            ],
        ),
        .executableTarget(
            name: "test386",
            dependencies: ["Free86"],
            sources: [
                "main.swift",
            ],
            swiftSettings: [
                .define("DEBUG", .when(configuration: .debug)),
            ],
        ),
        .target(
            name: "Free86",
            dependencies: [],
            sources: [
                "DataTypes.swift",
                "SegmentDescriptor.swift",
                "SegmentSelector.swift",
                "SegmentRegister.swift",
                "LogicalAddress+Pointers.swift",
                "LinearAddress.swift",
                "PageTableEntry.swift",
                "ModRM+SIB.swift",
                "GeneralRegister.swift",
                "ExtendedFlags.swift",
                "ControlRegister.swift",
                "Exception+Interrupt.swift",
                "InstructionPrefixRegister.swift",
                "Free86.swift",
                "Free86+fetchDecodeExecute.swift",
                "Free86+OneByteOpcode16Programs.swift",
                "Free86+OneByteOpcodePrograms.swift",
                "Free86+TwoByteOpcode16Programs.swift",
                "Free86+TwoByteOpcodePrograms.swift",
                "MemoryIO+PhysicalAddress.swift",
                "PagedMemory+DirectMemory.swift",
                "Instruction.swift",
                "Instruction+OneByteOpcodePrograms.swift",
                "Instruction+TwoByteOpcodePrograms.swift",
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
                "SegmentRegister.swift",
                "LinearAddress.swift",
                "PageTableEntry.swift",
                "ModRM+SIB.swift",
                "GeneralRegister.swift",
                "ExtendedFlags.swift",
                "ControlRegister.swift",
                "Exception+Interrupt.swift",
                "InstructionPrefixRegister.swift",
                "MemoryIO+PhysicalAddress.swift",
                "PinIO.swift",
           ],
            swiftSettings: [
                .define("DEBUG", .when(configuration: .debug)),
            ],
        ),
    ]
)
