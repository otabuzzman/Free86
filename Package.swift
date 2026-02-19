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
                "Free86.swift",  // no tests
                "Free86+reset.swift",
                "Free86+eflags.swift",  // no tests
                "Free86+setter.swift",
                "Free86+fetchDecodeExecute.swift",  // no tests
                "OpcodePrograms/Free86+oneByte16.swift",  // no tests
                "OpcodePrograms/Free86+oneByte.swift",    // no tests
                "OpcodePrograms/Free86+twoByte16.swift",  // no tests
                "OpcodePrograms/Free86+twoByte.swift",    // no tests
                "OpcodePrograms/Instruction+oneByte.swift",  // no tests
                "OpcodePrograms/Instruction+twoByte.swift",  // no tests
                "AuxiliaryPrograms/Free86+dataMovement.swift",  // no tests
                "AuxiliaryPrograms/Free86+binaryArithmethic.swift",   // no tests
                "AuxiliaryPrograms/Free86+decimalArithmethic.swift",  // no tests
                "AuxiliaryPrograms/Free86+bitShiftTestRotate.swift",  // no tests
                "AuxiliaryPrograms/Free86+stringManipulation.swift",  // no tests
                "AuxiliaryPrograms/Free86+controlTransfer.swift",  // no tests
                "AuxiliaryPrograms/Free86+miscellaneous.swift",    // no tests
                "LookupTables/Free86+LUT.swift",  // no tests
                "MachineDataTypes.swift",
                "GeneralRegister.swift",
                "SegmentRegister.swift",
                "Free86+segmentRegister.swift",  // no tests
                "SegmentSelector.swift",
                "SegmentDescriptor.swift",
                "ExtendedFlagsRegister.swift",
                "ControlRegister.swift",
                "Exception+Interrupt.swift",
                "Instruction.swift",  // no tests
                "ModRM+SIB.swift",
                "InstructionPrefixRegister.swift",
                "LogicalAddress+Pointers.swift",  // no tests
                "LinearAddress.swift",
                "MemoryIO+PhysicalAddress.swift",
                "PinIO.swift",
                "DirectMemory.swift",  // protocol, no explicit tests
                "MemoryIO+DirectMemory.swift",
                "PagedMemory.swift",   // protocol, no explicit tests
                "Free86+PagedMemory.swift",
                "PageTableEntry.swift",
                "TranslationLookasideBuffer.swift",  // protocol, no explicit tests
                "Free86+TranslationLookasideBuffer.swift",
                "Free86+segmentTranslation.swift",  // no tests
            ],
            swiftSettings: [
                .define("DEBUG", .when(configuration: .debug)),
            ],
        ),
        .testTarget(
            name: "suite",
            dependencies: ["Free86"],
            sources: [
                "Free86+reset.swift",
                "Free86+setter.swift",
                "Free86+SSB.swift",
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
                "Free86+PagedMemory.swift",
                "PageTableEntry.swift",
                "Free86+TranslationLookasideBuffer.swift",
           ],
            swiftSettings: [
                .define("DEBUG", .when(configuration: .debug)),
            ],
        ),
    ]
)
