#!/bin/bash
# Build script for OpenLoco Flatpak

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Default values
BUILD_DIR="${SCRIPT_DIR}/flatpak-build"
INSTALL_DIR="${SCRIPT_DIR}/flatpak-install"
REPO_DIR="${SCRIPT_DIR}/flatpak-repo"
ARCH="x86_64"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

function usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -h, --help          Show this help message"
    echo "  -b, --build-dir DIR  Specify build directory (default: ${BUILD_DIR})"
    echo "  -i, --install-dir DIR Specify install directory (default: ${INSTALL_DIR})"
    echo "  -r, --repo-dir DIR   Specify repo directory (default: ${REPO_DIR})"
    echo "  -a, --arch ARCH      Target architecture (default: ${ARCH})"
    echo "  --build             Build the Flatpak"
    echo "  --install           Install the Flatpak locally"
    echo "  --run               Run the Flatpak"
    echo "  --clean             Clean build and install directories"
    echo "  --all               Build, install, and run"
    echo ""
    echo "Examples:"
    echo "  $0 --all                    # Build, install, and run"
    echo "  $0 --build --install        # Build and install"
    echo "  $0 --clean                  # Clean all build files"
}

function check_dependencies() {
    echo -e "${YELLOW}Checking dependencies...${NC}"
    
    local missing=()
    
    command -v flatpak >/dev/null 2>&1 || missing+=("flatpak")
    command -v flatpak-builder >/dev/null 2>&1 || missing+=("flatpak-builder")
    command -v cmake >/dev/null 2>&1 || missing+=("cmake")
    command -v ninja >/dev/null 2>&1 || missing+=("ninja")
    command -v git >/dev/null 2>&1 || missing+=("git")
    
    if [ ${#missing[@]} -ne 0 ]; then
        echo -e "${RED}Error: Missing dependencies:${NC}"
        for dep in "${missing[@]}"; do
            echo "  - $dep"
        done
        echo ""
        echo "On Debian/Ubuntu, install with:"
        echo "  sudo apt install flatpak flatpak-builder cmake ninja-build git"
        echo ""
        echo "On Fedora:"
        echo "  sudo dnf install flatpak flatpak-builder cmake ninja-build git"
        exit 1
    fi
    
    echo -e "${GREEN}All dependencies found!${NC}"
}

function setup_flatpak() {
    echo -e "${YELLOW}Setting up Flatpak environment...${NC}"
    
    # Add Flathub repo if not already added
    if ! flatpak remote-list | grep -q flathub; then
        echo "Adding Flathub repository..."
        flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
    fi
    
    # Install required runtimes
    flatpak install --assumeyes flathub org.freedesktop.Platform//24.08
    flatpak install --assumeyes flathub org.freedesktop.Sdk//24.08
    
    echo -e "${GREEN}Flatpak environment ready!${NC}"
}

function clean() {
    echo -e "${YELLOW}Cleaning...${NC}"
    
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        echo "Removed build directory: $BUILD_DIR"
    fi
    
    if [ -d "$INSTALL_DIR" ]; then
        rm -rf "$INSTALL_DIR"
        echo "Removed install directory: $INSTALL_DIR"
    fi
    
    if [ -d "$REPO_DIR" ]; then
        rm -rf "$REPO_DIR"
        echo "Removed repo directory: $REPO_DIR"
    fi
    
    echo -e "${GREEN}Cleanup complete!${NC}"
}

function build_flatpak() {
    echo -e "${YELLOW}Building Flatpak...${NC}"
    
    mkdir -p "$BUILD_DIR"
    mkdir -p "$INSTALL_DIR"
    mkdir -p "$REPO_DIR"
    
    cd "$SCRIPT_DIR"
    
    echo "Building with manifest: com.openloco.OpenLoco.json"
    echo "Build directory: $BUILD_DIR"
    echo "Install directory: $INSTALL_DIR"
    
    # Build the Flatpak
    flatpak-builder \
        --user \
        --install-deps-from=flathub \
        --arch="$ARCH" \
        --build-dir="$BUILD_DIR" \
        --install-dir="$INSTALL_DIR" \
        --force-clean \
        "$REPO_DIR" \
        com.openloco.OpenLoco.json
    
    echo -e "${GREEN}Flatpak build complete!${NC}"
}

function install_flatpak() {
    echo -e "${YELLOW}Installing Flatpak...${NC}"
    
    # Create local repo
    flatpak build-export "$REPO_DIR" "$INSTALL_DIR"
    
    # Add local repo
    flatpak remote-add --user --if-not-exists openloco-repo "$REPO_DIR"
    
    # Install the application
    flatpak install --user --assumeyes openloco-repo com.openloco.OpenLoco
    
    echo -e "${GREEN}Flatpak installed!${NC}"
}

function run_flatpak() {
    echo -e "${YELLOW}Running OpenLoco...${NC}"
    flatpak run com.openloco.OpenLoco
}

# Parse arguments
ACTION=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)
            usage
            exit 0
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -i|--install-dir)
            INSTALL_DIR="$2"
            shift 2
            ;;
        -r|--repo-dir)
            REPO_DIR="$2"
            shift 2
            ;;
        -a|--arch)
            ARCH="$2"
            shift 2
            ;;
        --build)
            ACTION+="build "
            shift
            ;;
        --install)
            ACTION+="install "
            shift
            ;;
        --run)
            ACTION+="run "
            shift
            ;;
        --clean)
            ACTION+="clean "
            shift
            ;;
        --all)
            ACTION="clean build install run"
            shift
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            usage
            exit 1
            ;;
    esac
done

# If no action specified, show help
if [ -z "$ACTION" ]; then
    usage
    exit 1
fi

# Execute actions in order
for action in $ACTION; do
    case "$action" in
        clean)
            clean
            ;;
        build)
            check_dependencies
            setup_flatpak
            build_flatpak
            ;;
        install)
            install_flatpak
            ;;
        run)
            run_flatpak
            ;;
    esac
done

echo -e "${GREEN}Done!${NC}"
