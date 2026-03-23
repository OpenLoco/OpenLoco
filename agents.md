# OpenLoco Agent Runtime Notes

This file contains agent-specific operational notes. Repository architecture, coding conventions, developer workflow, and general build instructions belong in `README.md`.

## Scope

These notes apply to automated agents working in this repository.

## Agent Runtime Environment

- Primary local shell: `pwsh.exe`
- Workspace root in the current environment: `build/windows/`
- Visual Studio workspace/build integration is available in the current environment
- Local build/test tooling available in the current environment includes CMake, MSBuild/Visual Studio build integration, and `ctest`

## CI/CD Tools Available to the Agent

- No direct CI/CD service control is documented here
- GitHub Actions is used by the repository, but agents should assume local workspace access only unless explicit service credentials or endpoints are provided

## Environment Variables and Secrets

- No secrets or privileged environment variables are documented in this file
- Agents should assume only normal local workspace access unless the user explicitly provides additional credentials or service configuration

## Non-Interactive Build and Test Commands

Use repository-root commands that work in non-interactive agent runs.

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

## Agent-Specific API Endpoints or Service Configuration

- No agent-specific API endpoints are documented here
- No external service configuration is documented here beyond normal repository remotes

## MCP Server Configuration

- No MCP server configuration is documented here
- No MCP-specific permissions or service resource limits are documented here

## Practical Agent Guidance

- Use `README.md` for repository architecture, reverse-engineered engine patterns, coding conventions, build instructions, and development workflow
- Prefer non-interactive commands when validating changes
- Use British English spellings in repository prose and documentation unless quoting external names or proper nouns
- Keep changes minimal and preserve legacy compatibility unless the user explicitly requests otherwise
