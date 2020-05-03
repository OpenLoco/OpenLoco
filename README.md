# OpenLoco
An open source re-implementation of Chris Sawyer's Locomotion. A construction and management simulation video game that simulates running a transport company.

---

![](https://user-images.githubusercontent.com/604665/55420349-1a2aea00-5577-11e9-87da-78fe5cdb09e1.png)

# Contents
- 1 - [Introduction](#1-introduction)
- 2 - [Downloading the game (pre-built)](#2-downloading-the-game-pre-built)
- 3 - [Building the game](#3-building-the-game)
  - 3.1 - [Building prerequisites](#31-building-prerequisites)
  - 3.2 - [Compiling and running](#32-compiling-and-running)
- 4 - [Licence](#5-licence)
- 5 - [More information](#6-more-information)

---

### Build Status
|             | Windows | Linux / Mac | Download |
|-------------|---------|-------------|----------|
| **master**  | ![CI](https://github.com/OpenLoco/OpenLoco/workflows/CI/badge.svg) | [![Travis CI](https://travis-ci.org/OpenLoco/OpenLoco.svg?branch=master)](https://travis-ci.org/OpenLoco/OpenLoco) | [![GitHub release](https://img.shields.io/github/release/OpenLoco/OpenLoco.svg)](https://github.com/OpenLoco/OpenLoco/releases) |

### Chat

Feel free to join our Discord server to talk about developing the game, or for help getting it to run.

[![Discord](https://img.shields.io/discord/689445672390361176)](https://discord.gg/vEuNRHD)


# 1 Introduction

**OpenLoco** is an open-source re-implementation of Chris Sawyer's Locomotion (CSL). CSL is the spiritual successor to Transport Tycoon and OpenLoco aims to improve the game similar to how [OpenTTD](http://openttd.org) improved Transport Tycoon and [OpenRCT2](http://openrct2.io) improved RollerCoaster Tycoon.

Chris Sawyer's Locomotion was written by Chris Sawyer in x86 assembly building on top of his RollerCoaster Tycoon 2 engine. Much of the code is 1:1 with RollerCoaster Tycoon 2. This means that only select areas of the game such as the update logic should be written. The engine code such as audio, drawing and the window system should be left alone as it would only repeat the work that has already been done for OpenRCT2. It is more beneficial to share as much code as possible between OpenRCT2 and OpenLoco.

---

# 2 Downloading the game (pre-built)

OpenLoco requires original files of Chris Sawyer's Locomotion to play. It can be bought at either [Steam](http://store.steampowered.com/app/356430/) or [GOG.com](https://www.gog.com/game/chris_sawyers_locomotion).

The latest release can be found on [GitHub](https://github.com/OpenLoco/OpenLoco/releases).

---

# 3 Building the game

## 3.1 Building prerequisites

OpenLoco requires original files of Chris Sawyer's Locomotion to play. It can be bought at either [Steam](http://store.steampowered.com/app/356430/) or [GOG.com](https://www.gog.com/game/chris_sawyers_locomotion).

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

## 3.2 Compiling and running
### Windows:
1. Check out the repository. This can be done using [GitHub Desktop](https://desktop.github.com) or [other tools](https://help.github.com/articles/which-remote-url-should-i-use).
2. Open a new Developer Command Prompt for VS 2019, then navigate to the repository (e.g. `cd C:\GitHub\OpenLoco`).
3. Run `msbuild openloco.sln /t:restore;build`
4. Run `mklink /D bin\data ..\data` or `xcopy data bin\data /EIY`
5. Run the game, `bin\openloco`

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

# 4 Licence
**OpenLoco** is licensed under the MIT License.

---

# 5 More information
- [GitHub](https://github.com/OpenLoco/OpenLoco)
- [TT-Forums](https://www.tt-forums.net)
- [Locomotion subreddit](https://www.reddit.com/r/locomotion/)
