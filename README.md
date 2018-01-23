# OpenLoco
An open source re-implementation of Chris Sawyer's Locomotion. A construction and management simulation video game that simulates running a transport company.

---

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
| **master**  | [![AppVeyor](https://ci.appveyor.com/api/projects/status/fpq6ptmma5d3e389?svg=true)](https://ci.appveyor.com/project/IntelOrca/OpenLoco) | [![Travis CI](https://travis-ci.org/OpenRCT2/OpenLoco.svg?branch=master)](https://travis-ci.org/OpenRCT2/OpenLoco) | [![GitHub release](https://img.shields.io/github/release/OpenRCT2/OpenLoco.svg)](https://github.com/OpenRCT2/OpenLoco/releases) |

### Chat
You only need a GitHub or Twitter account to access these channels.

If you want to help *make* the game, join the developer channel.

If you need help, want to talk to the developers, or just want to stay up to date then join the non-developer channel for your language.

| Language | Non Developer | Developer |
|----------|---------------|-----------|
| English | [![Gitter](https://img.shields.io/badge/gitter-general-blue.svg)](https://gitter.im/OpenRCT2/OpenRCT2/non-dev) | [![Gitter](https://img.shields.io/badge/gitter-development-yellowgreen.svg)](https://gitter.im/OpenRCT2/OpenLoco) |
---

# 1 Introduction

**OpenLoco** is an open-source re-implementation of Chris Sawyer's Locomotion (CSL). CSL is the spiritual successor to Transport Tycoon and OpenLoco aims to improve the game similar to how [OpenTTD](http://openttd.org) improved Transport Tycoon and [OpenRCT2](http://openrct2.website) improved RollerCoaster Tycoon.

Chris Sawyer's Locomotion was written by Chris Sawyer in x86 assembly building on top of his RollerCoaster Tycoon 2 engine. Much of the code is 1:1 with RollerCoaster Tycoon 2. This means that only select areas of the game such as the update logic should be written. The engine code such as audio, drawing and the window system should be left alone as it would only repeat the work that has already been done for OpenRCT2. It is more beneficial to share as much code as possible between OpenRCT2 and OpenLoco.

---

# 2 Downloading the game (pre-built)

OpenLoco requires original files of Chris Sawyer's Locomotion to play. It can be bought at either [Steam](http://store.steampowered.com/app/356430/) or [GOG.com](https://www.gog.com/game/chris_sawyers_locomotion).

The latest release can be found on [GitHub](https://github.com/OpenRCT2/OpenLoco/releases).

---

# 3 Building the game

## 3.1 Building prerequisites

OpenLoco requires original files of Chris Sawyer's Locomotion to play. It can be bought at either [Steam](http://store.steampowered.com/app/356430/) or [GOG.com](https://www.gog.com/game/chris_sawyers_locomotion).

### Windows:
- 7 / 8 / 10
- [Visual Studio 2017](https://www.visualstudio.com/vs/community/) update 5.5+
  - Desktop development with C++
  - Windows 10 SDK (10.0.14393.0)
- [SDL2](https://www.libsdl.org/download-2.0.php)
  - Development Libraries

### Linux:
OpenLoco can not currently be built on Linux without a cross compiler. Pull requests are welcome to fix building for Linux. There are many function calls still to Win32 APIs. These require 'hooking' so that they can be replaced with cross-platform implementations. Scripts for linking in the loco.exe binary sections and remapping the address space can be found in the OpenRCT2 code base.

### macOS:
See Linux.

---

## 3.2 Compiling and running
### Windows:
1. Check out the repository. This can be done using [GitHub Desktop](https://desktop.github.com) or [other tools](https://help.github.com/articles/which-remote-url-should-i-use).
2. Copy SDL2 into a sub directory called lib under the root, alternatively use [vcpkg](https://github.com/microsoft/vcpkg).
3. Open a new Developer Command Prompt for VS 2017, then navigate to the repository (e.g. `cd C:\GitHub\OpenRCT2`).
4. Run `msbuild openloco.sln`.
5. Run the game, `bin\openloco`

### Linux:
The standard CMake build procedure is to install the required libraries, then:
```
mkdir build
cd build
cmake ../
make
```

---

# 4 Licence
**OpenLoco** is licensed under the MIT License.

---

# 5 More information
- [GitHub](https://github.com/OpenRCT2/OpenLoco)
- [TT-Forums](https://www.tt-forums.net)
- [Locomotion subreddit](https://www.reddit.com/r/locomotion/)
