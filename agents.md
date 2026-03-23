# OpenLoco - Repository Instructions for Automated Code Changes

This repository is a preservation, reconstruction, and improvement effort of an older x86 game called `Locomotion`.
It is best understood as a reconstructed simulation executable with C++ conveniences layered on top, not as a greenfield modern C++ engine.
The code often prefers binary/layout fidelity and behavioural equivalence over abstraction, ownership clarity, or modern style.

## Scope

These instructions apply to all automated edits in this repository.
Refer to `README.md` for additional game information, context, and build instructions.

## Primary Goal

Preserve game correctness, determinism, and compatibility while making minimal, targeted changes.

## Core Rule

- Preserve legacy `.sc5` / `.sv5` save compatibility and `.dat` object compatibility.
- Prefer behaviour-preserving changes over cleanup refactors.
- Treat awkward code as potentially intentional if it mirrors original executable behaviour, memory layout, or file format layout.

## Architectural Context

This codebase is heavily influenced by reverse-engineering from original x86 assembly into C++.
As a result, many patterns that may look unusual are intentional historical, binary, or behavioural relics.

When editing code:
- Prefer behavior preservation over stylistic cleanup.
- Do **not** "normalize" odd control flow, bitwise logic, magic constants, data packing, or naming without strong functional justification.
- Treat assembly-derived patterns as potentially semantic, not accidental.
- Keep diffs small and local; avoid broad refactors unless explicitly requested.

## Hard Compatibility Requirements

### Save/Object Formats Must Not Change

The following binary formats are fixed and must remain backward/vanilla compatible:
- Save formats: `.sc5`, `.sv5`
- Object format: `.dat`

Hard rules:
- Do **not** change serialized field order, size, alignment, packing, or meaning for existing persisted structures.
- Do **not** change enum underlying sizes or bit layouts used in persisted data.
- Do **not** repurpose reserved bits/bytes in a way that breaks existing assets.
- Do **not** introduce format revisions, new headers, or mandatory migration paths.
- Do **not** introduce non-deterministic behaviour or code at any stage or for any reason.

If a feature appears to require format evolution:
- Stop and document constraints.
- Propose a compatibility-safe alternative such as runtime-only data or derived/transient state.
- Defer true new save/object format work to a dedicated future project.

## Data Layout and Serialization Safety

For any type that may touch save/object I/O:
- Assume ABI and binary layout are sensitive.
- Avoid changing struct/class layout unless explicitly proven safe.
- Avoid changing integer widths/signness used for persisted values.
- Keep bitfields/flags stable.
- Preserve endian assumptions and binary read/write contracts.

## Bespoke Coding and Engine Patterns

### 1. Memory-layout fidelity is a first-class design goal

The codebase frequently models original game memory and file layouts directly.

Observed patterns:
- `#pragma pack(push, 1)` on persisted or reverse-engineered structs.
- `static_assert(sizeof(...))` and `static_assert(offsetof(...))` used as correctness checks.
- Explicit padding fields like `pad_103`, `pad_C59C`, `pad_24`.
- Fields with raw offset/address comments such as `// 0x000146 (0x00525F5E)`.

Examples:
- `src/OpenLoco/src/Scenario/ScenarioManager.h`
- `src/OpenLoco/src/S5/S5.h`
- `src/OpenLoco/src/S5/S5GameState.h`
- `src/OpenLoco/src/S5/S5Entity.h`
- `src/OpenLoco/src/GameState.h`

Implication for agents:
- Do not reorder fields, remove padding, or replace fixed arrays with containers in persisted or interfacing structs.
- Do not "clean up" unknown fields unless their binary role is fully understood.

### 2. Unknown or partially decoded fields are intentionally retained

Many names are archaeological rather than domain-clean:
- `var_85A`, `var_846`, `var_38`, `unkRng`
- `gc_unk_68`, `sub_431E6A`, `do_67`
- comments like `// no idea why this second spacing is needed`

This is normal here. It signals that the field or function exists in the original program, but its precise meaning is still incomplete or only partially decoded.

Implication for agents:
- Do not rename these aggressively just for readability.
- Prefer adding understanding around them over replacing them.
- Assume the weird name may be preserving reverse-engineering traceability.

### 3. Original executable addresses are part of the source of truth

The repository keeps the original x86 provenance visible.

Observed patterns:
- Function comments like `// 0x00443868`.
- Tables carrying original implementation addresses, e.g. game commands store `originalAddress`.
- Global/static variables annotated with original memory addresses.

Examples:
- `src/OpenLoco/src/Ui/Windows/ScenarioSelect.cpp`
- `src/OpenLoco/src/GameCommands/GameCommands.cpp`
- `src/OpenLoco/src/GameState.h`

Implication for agents:
- These comments are not noise. They are cross-reference anchors back to the original binary/disassembly.
- Avoid deleting them in refactors.

### 4. Global state managers are a core engine pattern

The engine is not built around dependency injection or isolated subsystems. It is manager-heavy and global-state-heavy.

Common managers:
- `WindowManager`
- `ObjectManager`
- `ScenarioManager`
- `CompanyManager`
- `SceneManager`
- `StringManager`

Observed patterns:
- File-local `static` storage for engine state.
- Manager APIs returning references into global arrays or vectors.
- Cross-subsystem access from almost anywhere.

Examples:
- `src/OpenLoco/src/Objects/ObjectManager.cpp`
- `src/OpenLoco/src/World/CompanyManager.cpp`
- `src/OpenLoco/src/Scenario/ScenarioManager.cpp`

Implication for agents:
- Do not expect clean ownership graphs or explicit service boundaries.
- Shared global state is part of the architecture, not necessarily a bug.

### 5. The game state is a giant fixed-layout blob, not a modern domain model

`GameState` is a monolithic world snapshot containing arrays for companies, towns, industries, stations, entities, messages, routings, and orders.

Observed patterns:
- Large fixed-capacity arrays sized by engine limits.
- Direct indexing rather than dynamic allocation and ownership trees.
- Separate import/export structures in `S5::*` mirroring on-disk layouts.

Examples:
- `src/OpenLoco/src/GameState.h`
- `src/OpenLoco/src/S5/S5GameState.h`

Implication for agents:
- Think in terms of a deterministic fixed-size simulation state.
- Avoid replacing these structures with allocator-heavy or pointer-rich models.

### 6. Save/load architecture is serialization-first and compatibility-driven

The engine maintains explicit structures for persisted formats and converts between runtime and saved forms.

Observed patterns:
- Separate `OpenLoco::GameState` and `OpenLoco::S5::GameState` types.
- Import/export functions rather than naive binary dumping of modern runtime objects.
- Version/fix flags and raw mode flags to deal with legacy variants.

Examples:
- `src/OpenLoco/src/S5/S5.h`
- `src/OpenLoco/src/S5/S5GameState.h`

Implication for agents:
- Changes to persisted structs are high risk.
- File format fidelity outranks API elegance.

### 7. The UI system is bespoke, table-driven, and stateful

The windowing system does not resemble Qt, WinUI, or ImGui-style retained models.

Observed patterns:
- Each window module lives in a namespace with file-local `static` callbacks.
- Widgets are declared in static tables, often with a `widx` enum for indexes.
- Behaviour is wired through a `WindowEventList` of function pointers.
- `Window` contains many reused scratch/state fields, including unions and generic slots.
- Widget enabled/active state is tracked via integer bitmasks.

Examples:
- `src/OpenLoco/src/Ui/Window.h`
- `src/OpenLoco/src/Ui/WindowManager.h`
- `src/OpenLoco/src/Ui/Windows/ScenarioSelect.cpp`

Notable unusual details:
- `Window::info` is a generic `uintptr_t` or union slot and may store pointers or sentinel values.
- Fields such as `var_85A`, `var_850`, `rowInfo`, and `currentTab` are reused across unrelated window types.
- Window lifecycle often requires manual `invalidate`, `callPrepareDraw`, `callOnResize`, and `initScrollWidgets` sequencing.

Implication for agents:
- Avoid trying to turn a window into a standalone class hierarchy unless the file already follows that pattern.
- Respect the existing event-table style.

### 8. UI code can have side effects outside normal modern layering expectations

In this codebase, UI draw/update paths may perform resource or object management work if that matches original behaviour.

Example:
- `ScenarioSelect.cpp` may load or unload currency objects during drawing if the selected scenario needs a different currency object.

Implication for agents:
- Do not assume rendering is pure.
- If a side effect exists in draw/update code, verify whether it preserves original control flow before moving it.

### 9. Game commands model an x86 register ABI, not a conventional C++ command bus

The command system is intentionally shaped around the original calling convention.

Observed patterns:
- `GameCommands::registers` explicitly models `eax`, `ebx`, `ecx`, `edx`, `esi`, `edi`, and `ebp` with byte/word overlays.
- Commands are dispatched using packed register state.
- A two-phase pattern exists: estimate/check first, then apply if allowed.
- Flags live in register bytes such as `regs.bl`.
- Legacy return values are stored in shared mutable state.

Examples:
- `src/OpenLoco/src/GameCommands/GameCommands.h`
- `src/OpenLoco/src/GameCommands/GameCommands.cpp`

Notable unusual details:
- `FAILURE` is a sentinel integer, not an exception/result object.
- `LegacyReturnState` exists specifically to emulate original side-channel return values.
- Some commands retain placeholder names like `do_67` or `gc_unk_69`.

Implication for agents:
- Do not try to normalize this into a pure OO command pattern without very strong reason.
- Register packing and side-channel state are part of behavioural compatibility.

### 10. Entity/vehicle architecture is slot-based and overlay-like

Vehicles are not a single polymorphic object with owned children. They are a linked set of fixed-size entity records with different subtypes.

Observed patterns:
- Multiple related record types: `VehicleHead`, `Vehicle1`, `Vehicle2`, `VehicleBody`, `VehicleBogie`, `VehicleTail`.
- Shared leading layouts are preserved manually.
- Links use IDs or indices like `head` and `nextCarId`, not owning pointers.
- Fixed-size `Entity` slots are reused through subtype interpretation.

Examples:
- `src/OpenLoco/src/S5/S5Entity.h`
- `src/OpenLoco/src/GameCommands/Vehicles/CreateVehicle.cpp`

Implication for agents:
- Be careful with assumptions about inheritance, ownership, and lifetime.
- Entity composition is often encoded by parallel layout and IDs, not standard C++ composition.

### 11. Object loading uses fixed repositories and handles rather than modern ownership semantics

The object system is based on pre-allocated repositories keyed by object type and index.

Observed patterns:
- Compile-time repository tables per `ObjectType`.
- Arrays of raw `Object*` plus parallel metadata arrays.
- Temporary object slots for transient loads.
- Handles and headers are often more important than C++ object ownership.

Examples:
- `src/OpenLoco/src/Objects/ObjectManager.cpp`

Implication for agents:
- Do not assume smart-pointer ownership is the intended abstraction.
- Raw pointers here often reference globally managed storage.

### 12. Localisation and text formatting are custom binary pipelines

Strings are not generally passed around as rich modern formatting objects.

Observed patterns:
- `StringId`-driven localisation instead of direct text literals.
- `StringManager::getString(...)` returning mutable scratch buffers in some call paths.
- `FormatArguments` writes raw bytes into small buffers.
- `strncpy` into shared localisation buffers is common.

Examples:
- `src/OpenLoco/src/Localisation/FormatArguments.hpp`
- `src/OpenLoco/src/Localisation/StringManager.h`
- `src/OpenLoco/src/Ui/Windows/ScenarioSelect.cpp`

Implication for agents:
- Prefer existing `StringId` + `FormatArguments` flows over introducing unrelated formatting frameworks.
- Be careful with buffer sizes and shared scratch-string usage.

### 13. Graphics/resource handling is low-level and mutation-friendly

The rendering stack frequently works with image tables, recolours, sprite metadata, and direct buffer mutation.

Observed patterns:
- Global sprite tables (`G1`) loaded into arrays.
- Temporary mutation of sprite metadata to reuse draw paths.
- Image IDs plus recolour flags instead of texture/material abstractions.

Examples:
- `src/OpenLoco/src/Graphics/Gfx.cpp`
- `src/OpenLoco/src/Ui/Windows/ScenarioSelect.cpp`

Implication for agents:
- Resource manipulation may look unsafe by modern standards but can be intentional and efficient within this engine model.

### 14. Sentinels and magic values are pervasive

This codebase often uses legacy sentinel values directly instead of `std::optional`, variants, or richer wrapper types.

Common examples:
- `0xFFFFFFFF`
- `0xFFFF`
- `-1`
- `StringIds::null`
- null-like ID wrappers such as `CompanyId::null`

Examples:
- `src/OpenLoco/src/Ui/Windows/ScenarioSelect.cpp`
- `src/OpenLoco/src/GameState.h`
- many manager and entity APIs

Implication for agents:
- Preserve sentinel semantics when modifying logic.
- Be wary of off-by-one and signed/unsigned interactions; many original behaviours depend on exact comparisons.

### 15. Fixed limits and preallocation are deliberate engine choices

The engine prefers deterministic limits and preallocated storage over elastic runtime containers.

Observed patterns:
- `Limits::kMax...` constants everywhere.
- Arrays sized to maximum engine capacities.
- Some dynamic containers exist, but core simulation data remains fixed-capacity.

Implication for agents:
- Do not assume dynamic growth is acceptable for core engine structures.
- Capacity limits are often gameplay, save-format, and original-engine constraints.

### 16. Behaviour preservation often wins over elegance

There are places where comments explicitly acknowledge awkwardness or historical bugs, but the code keeps them.

Observed patterns:
- Bug-for-bug compatibility notes.
- Comments describing original behaviour even when it appears wrong.
- Manual sequencing or duplicated spacing/invalidation that exists because it matches the original game.

Examples:
- `src/OpenLoco/src/GameCommands/Vehicles/CreateVehicle.cpp`
- `src/OpenLoco/src/Ui/Windows/ScenarioSelect.cpp`

Implication for agents:
- If something looks redundant, first assume it may be preserving compatibility.
- Prefer narrow fixes over stylistic rewrites.

## Coding Style for This Project

- Follow existing local style over external style guides.
- Keep naming, idioms, and function boundaries consistent with surrounding code.
- Avoid adding abstractions that obscure mapping to original game behavior.
- Add comments only when needed to explain non-obvious behavior or constraints.
- ClangFormat can be used to format code properly before committing.

## Testing and Validation Expectations

For format-sensitive or simulation changes:
- Verify no regression in loading existing `.sc5` / `.sv5` saves and `.dat` objects.
- Ensure determinism, not randomness.
- Prefer targeted tests and practical validation over speculative refactors.
- Keep behavior parity with vanilla/original mechanics where applicable.

## Non-Goals for Routine Changes

Unless explicitly requested, do not:
- Redesign serialization pipelines.
- Rewrite large reverse-engineered subsystems for style or readability alone.
- Introduce breaking data-model changes.

## Practical Guidance for Future Agents

When changing code in this repository:
- Keep offset comments, address comments, and `static_assert` layout checks intact.
- Preserve packed structs and fixed-size arrays unless the task is explicitly about format migration.
- Avoid replacing IDs/sentinels with pointers or `std::optional` in legacy-facing structures.
- Follow existing file patterns: namespace-scoped window modules, `widx` enums, static widget tables, event tables, manager calls.
- Expect non-RAII global state and cross-subsystem coupling.
- Treat `var_*`, `pad_*`, `unk_*`, `sub_*`, and `do_*` names as meaningful reverse-engineering markers.
- Validate whether a proposed cleanup would alter save format, object format, memory layout, deterministic behaviour, or original control flow.

## Decision Heuristic

When uncertain, choose the option that:
1. Preserves binary compatibility.
2. Preserves observed behavior.
3. Minimizes code churn.

## Short Mental Model

The unusual parts of OpenLoco are usually there because they preserve one of four things:
- binary layout
- save/object compatibility
- original x86 control flow
- original gameplay behaviour
