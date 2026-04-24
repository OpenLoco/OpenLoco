# OpenLoco
An open source re-implementation of Chris Sawyer's Locomotion. A construction and management simulation video game that simulates running a transport company.

---

![](https://user-images.githubusercontent.com/604665/55420349-1a2aea00-5577-11e9-87da-78fe5cdb09e1.png)

# Contents
- 1 - [Introduction](#1-introduction)
- 2 - [Downloading the game (pre-built)](#2-downloading-the-game-pre-built)
- 3 - [Contributing](#3-contributing)
- 3.1 - [Development workflow](#31-development-workflow)
- 3.2 - [Project architecture and reverse-engineered design](#32-project-architecture-and-reverse-engineered-design)
- 4 - [Compiling the game](#4-compiling-the-game)
  - 4.1 - [Building prerequisites](#41-building-prerequisites)
  - 4.2 - [Compiling and running](#42-compiling-and-running)
- 4.3 - [Running tests](#43-running-tests)
- 5 - [Licence](#5-licence)
- 6 - [More information](#6-more-information)

---

### Build Status
|             | Windows / Linux / MacOS | Download |
|-------------|-----------------|----------|
| **master**  | ![CI](https://github.com/OpenLoco/OpenLoco/workflows/CI/badge.svg) | [![GitHub release](https://img.shields.io/github/release/OpenLoco/OpenLoco.svg)](https://github.com/OpenLoco/OpenLoco/releases) |

### Chat

Feel free to join our Discord server to talk about developing the game, or for help getting it to run.

[![Discord](https://img.shields.io/discord/689445672390361176)](https://discord.gg/vEuNRHD)


# 1 Introduction

**OpenLoco** is an open-source re-implementation of *Chris Sawyer's Locomotion* (CSL), the spiritual successor to Transport Tycoon. OpenLoco aims to improve the game similar to how [OpenTTD](https://openttd.org) improved *Transport Tycoon*, and [OpenRCT2](https://openrct2.io) improved *RollerCoaster Tycoon*.

CSL was originally written in x86 assembly, building on top of the RollerCoaster Tycoon 2 engine. However, the engine has changed substantially enough that OpenLoco currently does not share its codebase with OpenRCT2.

As of December 2025, we have completed reimplementing the game in C++. It is now our goal to get a solid multiplayer experience working in OpenLoco. It is also our goal to increase the map and vehicle limits. However, until creating a new save format (NSF), we are bound to the limits imposed by the CSL save format (SV5/SC5).

---

# 2 Downloading the game (pre-built)

The latest releases can be [downloaded from GitHub](https://github.com/OpenLoco/OpenLoco/releases). Releases are currently provided for Windows, Linux and MacOS.
For BSD distributions, we currently do not provide any builds. Please refer to the next section to compile the game manually. For macOS the builds are unsigned and unnotarised. This means that Gatekeeper may block them.

Please note that OpenLoco requires the asset files of the original Chris Sawyer's Locomotion to play the game.
It can be bought at e.g. [Steam](https://store.steampowered.com/app/356430/) or [GOG.com](https://www.gog.com/game/chris_sawyers_locomotion).

---

# 3 Contributing

We warmly welcome any contributions to the project, e.g. for C++ code (game implementation, bug fixes, features) or localisation (new translations).
Please have a look at our [issues for newcomers](https://github.com/OpenLoco/OpenLoco/issues?q=is%3Aopen+label%3A%22good+first+issue%22%2C%22hacktoberfest%22).

For code contributions, please stick to our [code style](https://github.com/OpenLoco/OpenLoco/wiki/Coding-Style).
You can use `clang-format` to apply these guidelines automatically.

In addition to the code style guide, please keep changes small and behaviour-preserving. The OpenLoco project is reverse-engineered from x86 assembly, so unusual patterns may be intentional and tied to compatibility with the original game.
Repository prose, comments, and documentation should use British English spellings where practical.

## 3.1 Development workflow

For routine code changes:
- keep diffs small and local
- prefer behaviour preservation over cleanup refactors
- build the project before submitting changes
- run the relevant automated tests where possible
- preserve compatibility with legacy `.sc5` / `.sv5` save files and `.dat` objects

If a proposed change appears to require changing a binary format, serialised layout, or engine limit tied to the legacy save format, prefer a compatibility-safe runtime alternative instead, or postpone the change until we have finished developing the New-Object-Format and New-Save-Format.

## 3.2 Project architecture and reverse-engineered design

Key design decisions and patterns:
- binary and file-layout fidelity often take priority over idiomatic modern C++ abstractions
- packed structs, explicit padding, offset comments, and `static_assert(sizeof(...))` / `static_assert(offsetof(...))` checks are often part of compatibility guarantees
- save/load code is compatibility-driven; runtime structures and on-disk `S5` structures are kept deliberately close to legacy formats
- the engine is manager-heavy and global-state-heavy, with systems such as `WindowManager`, `ObjectManager`, `ScenarioManager`, `CompanyManager`, `SceneManager`, and `StringManager`
- core simulation state is largely fixed-capacity and deterministic rather than dynamically modelled
- the UI is bespoke and table-driven, using widget arrays, `widx` enums, `WindowEventList`, bitmask state, and reusable window scratch fields
- game commands intentionally model legacy x86 register-style calling conventions rather than a purely object-oriented command bus
- entity and vehicle state is slot-based and layout-sensitive, using IDs and linked records instead of modern ownership-heavy composition
- localisation and rendering use custom low-level pipelines such as `StringId`, `FormatArguments`, sprite/image tables, and temporary resource mutation

When working in these areas, do not assume unusual code can be immediately modernised. Preserving the original control flow, memory layout, and gameplay behaviour is always the correct choice.

---

# 4 Compiling the game

If you would like to contribute code to OpenLoco, please follow the instructions below to get started compiling the game.

If you just want to play the game, you can just [download the latest release](https://github.com/OpenLoco/OpenLoco/releases) from GitHub.
Releases are provided for Windows (64 & 32-bit) / Linux (64-bit only) / MacOS (arm64 only).

## 4.1 Building prerequisites

The following libraries/dependencies are used:
- [libpng](http://www.libpng.org/pub/png/libpng.html)
- [libzip](https://libzip.org)
- [OpenAL](https://www.openal.org/)
- [SDL3](https://github.com/libsdl-org/SDL/releases)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) (fetched during CMake generation)
- [fmt](https://github.com/fmtlib/fmt) (fetched during CMake generation)
- [sfl](https://github.com/slavenf/sfl-library) (fetched during CMake generation)
- [breakpad](https://github.com/google/breakpad) (only required on Windows)
- [tbb](https://www.intel.com/content/www/us/en/developer/tools/oneapi/onetbb.html) (only required if using libstdc++)

### Windows
- 10 / 11
- [Visual Studio 2022](https://www.visualstudio.com/vs/community/)
  - Desktop development with C++ (ensure MFC component is selected)
  - Dependencies are managed with [vcpkg](https://github.com/Microsoft/vcpkg)

### Linux
- cmake 3.22+
- make or ninja
- If building x86-32 then 32-bit versions of the libraries mentioned above
- Dependencies can optionally be managed by vcpkg

### MacOS
- cmake 3.22+
- make or ninja
- Libraries mentioned above installed to standard paths using e.g. Homebrew
- Dependencies can optionally be managed by vcpkg

---

## 4.2 Compiling and running

### Windows:

1. Check out the repository. This can be done using [GitHub Desktop](https://desktop.github.com) or [other tools](https://help.github.com/articles/which-remote-url-should-i-use).
2. With VS 2022 use the "Open a local folder" option to start the project file generation. This may take some time as it downloads dependencies.
3. After successful generation of the project files open "build/windows/openloco.sln".
4. Select a config Debug or Release and run Build -> Build Solution.
4. Run the game from "build/windows/<config>/OpenLoco.exe" or within VS.

Alternatively using CMake use the following commands.

1. Run `cmake --preset windows`
2. Run `cmake --build --preset windows-release`

### Linux:

The standard CMake build procedure is to install the required libraries, then:
```
cmake --preset posix
cmake --build --preset posix-release
```

Note: If building x86-32 installing some packages can be problematic. To work around this, you can use [our Docker images](https://github.com/OpenLoco/openloco-docker-build) for compilation.
    
Note: Due to issues with distro yaml-cpp packages, its source release is downloaded during CMake generation.

### MacOS

The standard CMake build procedure is to install the required libraries, then:
```
cmake --preset posix
cmake --build --preset posix-release
```

Note: Only arm64 builds have been tested.

## 4.3 Running tests

After building, the automated tests can be run with `ctest`.

### Windows

From the repository root:
```
ctest --test-dir build/windows -C Debug --output-on-failure
```

For Release builds:
```
ctest --test-dir build/windows -C Release --output-on-failure
```

### Linux / MacOS

From the repository root:
```
ctest --test-dir build/posix -C Debug --output-on-failure
```

For Release builds:
```
ctest --test-dir build/posix -C Release --output-on-failure
```

---

# 5 Licence
**OpenLoco** is licensed under the MIT License.

---

# 6 More information
- [GitHub](https://github.com/OpenLoco/OpenLoco)
- [TT-Forums](https://www.tt-forums.net)
- [Locomotion subreddit](https://www.reddit.com/r/locomotion/)
