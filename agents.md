# `agents.md` — Repository Instructions for Automated Code Changes

## Scope
These instructions apply to all automated edits in this repository.

## Primary Goal
Preserve game correctness and compatibility while making minimal, targeted changes.

## Architectural Context (Important)
This codebase is heavily influenced by reverse-engineering from original x86 assembly into C++.
As a result, many patterns that may look unusual are intentional historical/behavioral relics.

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
- Propose a compatibility-safe alternative (runtime-only data, derived/transient state, etc.).
- Defer true "new save format" / "new object format" work to dedicated future projects.

## Data Layout and Serialization Safety
For any type that may touch save/object I/O:
- Assume ABI and binary layout are sensitive.
- Avoid changing struct/class layout unless explicitly proven safe.
- Avoid changing integer widths/signness used for persisted values.
- Keep bitfields/flags stable.
- Preserve endian assumptions and binary read/write contracts.

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
- Rewrite large reverse-engineered subsystems for style/readability alone.
- Introduce breaking data-model changes.

## Decision Heuristic
When uncertain, choose the option that:
1. Preserves binary compatibility.
2. Preserves observed behavior.
3. Minimizes code churn.