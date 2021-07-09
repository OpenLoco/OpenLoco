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

**OpenLoco** is an open-source re-implementation of Chris Sawyer's Locomotion (CSL). CSL is the spiritual successor to Transport Tycoon and OpenLoco aims to improve the game similar to how [OpenTTD](https://openttd.org) improved Transport Tycoon and [OpenRCT2](https://openrct2.io) improved RollerCoaster Tycoon.

Chris Sawyer's Locomotion was originally written by Chris Sawyer in x86 assembly, building on top of his RollerCoaster Tycoon 2 engine. Much of the code is 1:1 with RollerCoaster Tycoon 2. However, the engine has changed substantially enough that OpenLoco currently does not share its codebase with OpenRCT2.

Recent implementation efforts have focussed on re-implementing the UI, so that the data structures involved may be better understood. Once this is done, it is our hope the game logic will be easier to understand and implement.

---

# 2 Downloading the game (pre-built)

The latest releases can be [downloaded from GitHub](https://github.com/OpenLoco/OpenLoco/releases). Releases are currently provided for Windows and macOS (32-bit only).
For Linux and BSD distributions, we currently do not provide any builds. Please refer to the next section to compile the game manually.

Please note that OpenLoco requires the asset files of the original Chris Sawyer's Locomotion to play the game.
It can be bought at e.g. [Steam](https://store.steampowered.com/app/356430/) or [GOG.com](https://www.gog.com/game/chris_sawyers_locomotion).

---

# 3 Contributing

We warmly welcome any contributions to the project, e.g. for C++ code (game implementation, bug fixes, features) or localisation (new translations).
Please have a look at our [issues for newcomers](https://github.com/OpenLoco/OpenLoco/labels/good%20first%20issue).

For code contributions, please stick to our [code style](https://github.com/OpenLoco/OpenLoco/wiki/Coding-Style).
You can use `clang-format` to apply these guidelines automatically.

---

# 4 Compiling the game

If you would like to contribute code to OpenLoco, please follow the instructions below to get started compiling the game.
Alternatively, we have platform-specific guides for [Ubuntu](https://github.com/OpenLoco/OpenLoco/wiki/Building-on-Ubuntu) and [macOS](https://github.com/OpenLoco/OpenLoco/wiki/Building-on-macOS).

If you just want to play the game, you can just [download the latest release](https://github.com/OpenLoco/OpenLoco/releases) from GitHub.
Releases are currently provided for Windows and macOS (32-bit only).

## 4.1 Building prerequisites

Regardless of platform, the following libraries/dependencies are required:
- [libpng](http://www.libpng.org/pub/png/libpng.html)
- [libzip](https://libzip.org)
- [SDL2](https://www.libsdl.org/download-2.0.php)
- [SDL2_mixer](https://www.libsdl.org/projects/SDL_mixer/)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)

### Windows
- 7 / 8 / 10
- [Visual Studio 2019](https://www.visualstudio.com/vs/community/)
  - Desktop development with C++
  - Dependencies are managed with [vcpkg](https://github.com/Microsoft/vcpkg)

### Linux / macOS
- cmake
- make or ninja
- 32-bit versions of the libraries mentioned above

---

## 4.2 Compiling and running
### Windows:
1. Check out the repository. This can be done using [GitHub Desktop](https://desktop.github.com) or [other tools](https://help.github.com/articles/which-remote-url-should-i-use).
2. Open a new Developer Command Prompt for VS 2019, then navigate to the repository (e.g. `cd C:\GitHub\OpenLoco`).
3. Run `msbuild openloco.sln /t:restore;build`
4. Run `mklink /D bin\data ..\data` or `xcopy data bin\data /EIY`
5. Run `mklink openloco.exe bin\` or `copy openloco.exe bin\`
6. Run the game, `bin\openloco`

### Linux / macOS:
The standard CMake build procedure is to install the required libraries, then:
```
mkdir build
cd build
CXXFLAGS="-m32" cmake ..
make
```

Running the game will need the data directory from the root of the source code next to the binary. Assuming you're in `$SRC/build`, 
```
ln -s ../data
OR
cp -r ../data ./data 
```
---

# 5 Licence
**OpenLoco** is licensed under the MIT License.

---

# 6 More information
- [GitHub](https://github.com/OpenLoco/OpenLoco)
- [TT-Forums](https://www.tt-forums.net)
- [Locomotion subreddit](https://www.reddit.com/r/locomotion/)
