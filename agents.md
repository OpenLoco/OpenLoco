# OpenLoco Agent Runtime Notes

This file contains agent-specific operational notes. Repository architecture, coding conventions, developer workflow, and general build instructions belong in `README.md`.

## Agent Runtime Environment

- Local build/test tooling available in the current environment includes CMake, MSBuild/Visual Studio build integration, and `ctest`

### Configure and build

Windows:
- `cmake --preset windows`
- `cmake --build --preset windows-debug`
- `cmake --build --preset windows-release`

POSIX:
- `cmake --preset posix`
- `cmake --build --preset posix-debug`
- `cmake --build --preset posix-release`

### Test execution

Windows:
- `ctest --test-dir build/windows -C Debug --output-on-failure`
- `ctest --test-dir build/windows -C Release --output-on-failure`

POSIX:
- `ctest --test-dir build/posix -C Debug --output-on-failure`
- `ctest --test-dir build/posix -C Release --output-on-failure`

## Operations Requiring Human Intervention

- Acquiring or validating original game asset files requires human action
- Release signing, notarisation, publishing, and credentialled release operations require human-managed credentials/processes
- Any change that would alter `.sc5`, `.sv5`, or `.dat` compatibility is banned

## Practical Agent Guidance

- Use `README.md` for repository architecture, reverse-engineered engine patterns, coding conventions, build instructions, and development workflow
- Prefer non-interactive commands when validating changes
- Use British English spellings in repository prose and documentation unless quoting external names or proper nouns
- Keep changes minimal and preserve legacy compatibility unless the user explicitly requests otherwise
