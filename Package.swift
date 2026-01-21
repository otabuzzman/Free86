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
                "Free86.swift",
                "Free86+fetchDecodeExecute.swift",
                "OpcodePrograms/Free86+oneByte16.swift",
                "OpcodePrograms/Free86+oneByte.swift",
                "OpcodePrograms/Free86+twoByte16.swift",
                "OpcodePrograms/Free86+twoByte.swift",
                "OpcodePrograms/Instruction+oneByte.swift",
                "OpcodePrograms/Instruction+twoByte.swift",
                "MachineDataTypes.swift",
                "GeneralRegister.swift",
                "SegmentRegister.swift",
                "SegmentSelector.swift",
                "SegmentDescriptor.swift",
                "ExtendedFlagsRegister.swift",
                "ControlRegister.swift",
                "Exception+Interrupt.swift",
                "Instruction.swift",
                "ModRM+SIB.swift",
                "InstructionPrefixRegister.swift",
                "LogicalAddress+Pointers.swift",
                "LinearAddress.swift",
                "MemoryIO+PhysicalAddress.swift",
                "PinIO.swift",
                "DirectMemory.swift",
                "MemoryIO+DirectMemory.swift",
                "Free86+DirectMemory.swift",
                "PagedMemory.swift",
                "Free86+PagedMemory.swift",
                "PageTableEntry.swift",
                "TranslationLookasideBuffer.swift",
                "Free86+TranslationLookasideBuffer.swift",
            ],
            swiftSettings: [
                .define("DEBUG", .when(configuration: .debug)),
            ],
        ),
        .testTarget(
            name: "suite",
            dependencies: ["Free86"],
            sources: [
                "MachineDataTypes.swift",
                "GeneralRegister.swift",
                "SegmentRegister.swift",
                "SegmentSelector.swift",
                "SegmentDescriptor.swift",
                "ExtendedFlagsRegister.swift",
                "ControlRegister.swift",
                "Exception+Interrupt.swift",
                "ModRM+SIB.swift",
                "InstructionPrefixRegister.swift",
                "LinearAddress.swift",
                "MemoryIO+PhysicalAddress.swift",
                "PinIO.swift",
                "MemoryIO+DirectMemory.swift",
                "PageTableEntry.swift",
           ],
            swiftSettings: [
                .define("DEBUG", .when(configuration: .debug)),
            ],
        ),
    ]
)
