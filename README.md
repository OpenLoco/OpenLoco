# OpenLoco
An open source re-implementation of Chris Sawyer's Locomotion. A construction and management simulation video game that simulates running a transport company.

---

![](https://user-images.githubusercontent.com/604665/55420349-1a2aea00-5577-11e9-87da-78fe5cdb09e1.png)

# Contents
- 1 - [Introduction](#1-introduction)
- 2 - [Downloading the game (pre-built)](#2-downloading-the-game-pre-built)
- 3 - [Contributing](#3-contributing)
- 4 - [Compiling the game](#4-compiling-the-game)
  - 4.1 - [Building prerequisites](#41-building-prerequisites)
  - 4.2 - [Compiling and running](#42-compiling-and-running)
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
For BSD distributions, we currently do not provide any builds. Please refer to the next section to compile the game manually. For macOS the builds are unsigned and unnotarized. This means that Gatekeeper may block them.

Please note that OpenLoco requires the asset files of the original Chris Sawyer's Locomotion to play the game.
It can be bought at e.g. [Steam](https://store.steampowered.com/app/356430/) or [GOG.com](https://www.gog.com/game/chris_sawyers_locomotion).

---

# 3 Contributing

We warmly welcome any contributions to the project, e.g. for C++ code (game implementation, bug fixes, features) or localisation (new translations).
Please have a look at our [issues for newcomers](https://github.com/OpenLoco/OpenLoco/issues?q=is%3Aopen+label%3A%22good+first+issue%22%2C%22hacktoberfest%22).

For code contributions, please stick to our [code style](https://github.com/OpenLoco/OpenLoco/wiki/Coding-Style).
You can use `clang-format` to apply these guidelines automatically.

---

# 4 Compiling the game

If you would like to contribute code to OpenLoco, please follow the instructions below to get started compiling the game.

If you just want to play the game, you can just [download the latest release](https://github.com/OpenLoco/OpenLoco/releases) from GitHub.
Releases are provided for Windows (32-bit only) / Linux (64-bit only) / MacOS (arm64 only).

## 4.1 Building prerequisites

The following libraries/dependencies are used:
- [libpng](http://www.libpng.org/pub/png/libpng.html)
- [libzip](https://libzip.org)
- [OpenAL](https://www.openal.org/)
- [SDL2](https://www.libsdl.org/download-2.0.php)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) (fetched during CMake generation)
- [fmt](https://github.com/fmtlib/fmt) (fetched during CMake generation)
- [sfl](https://github.com/slavenf/sfl-library) (fetched during CMake generation)
- [breakpad](https://github.com/google/breakpad) (only required on Windows)

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

---

# 5 Licence
**OpenLoco** is licensed under the MIT License.

---

# 6 More information
- [GitHub](https://github.com/OpenLoco/OpenLoco)
- [TT-Forums](https://www.tt-forums.net)
- [Locomotion subreddit](https://www.reddit.com/r/locomotion/)
