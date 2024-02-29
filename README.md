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
|             | Windows / Linux | Download |
|-------------|-----------------|----------|
| **master**  | ![CI](https://github.com/OpenLoco/OpenLoco/workflows/CI/badge.svg) | [![GitHub release](https://img.shields.io/github/release/OpenLoco/OpenLoco.svg)](https://github.com/OpenLoco/OpenLoco/releases) |

### Chat

Feel free to join our Discord server to talk about developing the game, or for help getting it to run.

[![Discord](https://img.shields.io/discord/689445672390361176)](https://discord.gg/vEuNRHD)


# 1 Introduction

**OpenLoco** is an open-source re-implementation of *Chris Sawyer's Locomotion* (CSL), the spiritual successor to Transport Tycoon. OpenLoco aims to improve the game similar to how [OpenTTD](https://openttd.org) improved *Transport Tycoon*, and [OpenRCT2](https://openrct2.io) improved *RollerCoaster Tycoon*.

CSL was originally written in x86 assembly, building on top of the RollerCoaster Tycoon 2 engine. However, the engine has changed substantially enough that OpenLoco currently does not share its codebase with OpenRCT2.

The reimplementation efforts of OpenLoco are gradual, aiming to ultimately rewrite the entire game in C++. In the earlier years of the project, the in-game UI has been completely reimplemented, and most of the underlying data and object structures have been uncovered. Recent efforts have focussed on reimplementing the game (command) logic. Once this has been completed, it is our goal to get a solid multiplayer experience working in OpenLoco. It is also our goal to increase the map and vehicle limits. However, until all logic has been reimplemented, we are bound to the limits imposed by the CSL save format (SV5/SC5).

---

# 2 Downloading the game (pre-built)

The latest releases can be [downloaded from GitHub](https://github.com/OpenLoco/OpenLoco/releases). Releases are currently provided only for Windows.
For Linux and BSD distributions, we currently do not provide any builds. Please refer to the next section to compile the game manually. For macOS, we recommend [using Wine](https://github.com/OpenLoco/OpenLoco/wiki/Running-OpenLoco-on-macOS).

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
Alternatively, we have platform-specific guides for [Ubuntu](https://github.com/OpenLoco/OpenLoco/wiki/Building-on-Ubuntu) and [macOS](https://github.com/OpenLoco/OpenLoco/wiki/Building-on-macOS).

If you just want to play the game, you can just [download the latest release](https://github.com/OpenLoco/OpenLoco/releases) from GitHub.
Releases are currently only provided for Windows (32-bit only).

## 4.1 Building prerequisites

The following libraries/dependencies are required:
- [libpng](http://www.libpng.org/pub/png/libpng.html)
- [libzip](https://libzip.org)
- [OpenAL](https://www.openal.org/)
- [SDL2](https://www.libsdl.org/download-2.0.php)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) (fetched during CMake generation)
- [fmt](https://github.com/fmtlib/fmt) (fetched during CMake generation)
- [breakpad](https://github.com/google/breakpad) (only required on Windows)

### Windows
- 7 / 8 / 10 / 11
- [Visual Studio 2022](https://www.visualstudio.com/vs/community/)
  - Desktop development with C++ (ensure MFC component is selected)
  - Dependencies are managed with [vcpkg](https://github.com/Microsoft/vcpkg)

### Linux
- cmake 3.22+
- make or ninja
- 32-bit versions of the libraries mentioned above

---

## 4.2 Compiling and running

Note: The game can currently only be built for 32-bit architectures.

### Windows:

1. Check out the repository. This can be done using [GitHub Desktop](https://desktop.github.com) or [other tools](https://help.github.com/articles/which-remote-url-should-i-use).
2. With VS 2022 use the "Open a local folder" option to start the project file generation. This may take some time as it downloads dependencies.
3. After successful generation of the project files open "build/windows-msvc/openloco.sln".
4. Select a config Debug or Release and run Build -> Build Solution.
4. Run the game from "build/windows-msvc/<config>/OpenLoco.exe" or within VS.

Alternatively using CMake use the following commands.

1. Run `cmake --preset windows-msvc`
2. Run `cmake --build --preset windows-msvc-release`

### Linux:

The standard CMake build procedure is to install the required libraries, then:
```
cmake --preset linux
cmake --build --preset linux-release
```

Installing some packages can be problematic on desktop AMD64 distributions. To work around this, you can use [our Docker images](https://github.com/OpenLoco/openloco-docker-build) for compilation.
    
Note: Due to issues with distro yaml-cpp packages, its source release is downloaded during CMake generation.

Running the game will need the data directory from the root of the source code next to the binary. Assuming you're in `$SRC/build`, 
```
ln -s ../data
OR
cp -r ../data ./data 
```

### MacOS

For technical reasons OpenLoco can only be built as 32-bit x86 application, for which Apple dropped support in Mac OS 10.15. We can't provide MacOS builds at this time.

---

# 5 Licence
**OpenLoco** is licensed under the MIT License.

---

# 6 More information
- [GitHub](https://github.com/OpenLoco/OpenLoco)
- [TT-Forums](https://www.tt-forums.net)
- [Locomotion subreddit](https://www.reddit.com/r/locomotion/)
