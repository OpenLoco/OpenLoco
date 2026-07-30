# OpenLoco Flatpak Distribution

This directory contains the files needed to build and distribute OpenLoco as a Flatpak.

## Files

- `com.openloco.OpenLoco.json` - Flatpak application manifest
- `com.openloco.OpenLoco.appdata.xml` - AppStream metadata for software centers
- `openloco.desktop` - Desktop entry file (used by Flatpak)
- `build-flatpak.sh` - Build script to simplify Flatpak creation

## Requirements

To build the Flatpak, you need:

- **flatpak** - Flatpak package manager
- **flatpak-builder** - Flatpak build tool
- **cmake** (>= 3.22) - Build system
- **ninja** - Build system backend
- **git** - Version control system

### Installing Dependencies

#### Debian/Ubuntu
```bash
sudo apt install flatpak flatpak-builder cmake ninja-build git
```

#### Fedora
```bash
sudo dnf install flatpak flatpak-builder cmake ninja-build git
```

#### Arch Linux
```bash
sudo pacman -S flatpak flatpak-builder cmake ninja git
```

## Quick Start

The easiest way to build, install, and run OpenLoco Flatpak is:

```bash
./build-flatpak.sh --all
```

This will:
1. Clean any previous build
2. Build the Flatpak
3. Install it locally
4. Run OpenLoco

## Manual Build Process

### 1. Setup Flatpak Environment

Add Flathub repository:
```bash
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```

Install the required runtimes:
```bash
flatpak install flathub org.freedesktop.Platform//24.08
flatpak install flathub org.freedesktop.Sdk//24.08
```

### 2. Build the Flatpak

```bash
flatpak-builder \
    --user \
    --install-deps-from=flathub \
    --arch=x86_64 \
    --build-dir=flatpak-build \
    --install-dir=flatpak-install \
    flatpak-repo \
    com.openloco.OpenLoco.json
```

### 3. Export and Install

```bash
# Export to a local repository
flatpak build-export flatpak-repo flatpak-install

# Add the local repository
flatpak remote-add --user openloco-repo flatpak-repo

# Install the application
flatpak install --user openloco-repo com.openloco.OpenLoco
```

### 4. Run OpenLoco

```bash
flatpak run com.openloco.OpenLoco
```

## Building for Distribution

To create a Flatpak bundle that can be distributed:

```bash
flatpak build-bundle flatpak-repo openloco.flatpak com.openloco.OpenLoco
```

This will create a `.flatpak` file that users can install with:

```bash
flatpak install --user openloco.flatpak
```

## Updating the Flatpak

When updating OpenLoco to a new version:

1. Update the `version` field in the manifest
2. Update the `release` section in the AppStream metadata
3. Update the SHA256 checksums if dependencies have changed
4. Rebuild and test

## Debugging

To run with verbose output:
```bash
flatpak run --verbose com.openloco.OpenLoco
```

To enter the application's sandbox for debugging:
```bash
flatpak run --command=sh com.openloco.OpenLoco
```

## Notes

- OpenLoco requires the asset files from the original Chris Sawyer's Locomotion game
- These can be purchased from Steam or GOG.com
- The Flatpak will look for assets in `~/.local/share/chris-sawyer-locomotion/`
- Users can also configure the path in the game settings

## Permissions

The Flatpak has the following permissions:

- **Display**: Wayland and X11 support
- **Audio**: PulseAudio for sound
- **D-Bus**: Session bus for system integration
- **Filesystem**: 
  - `xdg-config` for configuration
  - `xdg-data` (read-only) for application data
  - `xdg-save` for save files
  - `~/.local/share/chris-sawyer-locomotion` for game assets (create if needed)
- **Hardware**: DRI for GPU acceleration
