#!/bin/bash

# Resolve absolute paths
SCRIPT_DIR=$(dirname "$(realpath "$0")")
BASE_DIR=$(realpath "$SCRIPT_DIR/..")
BUILD_DIR="$BASE_DIR/build"
CLEANUP_SCRIPT="$SCRIPT_DIR/cleanup.sh"

# Default flags
OVERWRITE=false
JOBS_ARG="-j"  # Use all processors
SYSTEM_INSTALL=false

# Help message
show_help() {
    echo "Usage: ./build.sh [OPTIONS]"
    echo
    echo "Options:"
    echo "  -o, --overwrite           Remove existing build directory before building"
    echo "  -j, --jobs <number>       Specify number of processors to use (default: all available)"
    echo "  -h, --help                Display this help message"
}

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -o|--overwrite)
            OVERWRITE=true
            shift
            ;;
        -j|--jobs)
            if [[ -n "$2" && "$2" != -* ]]; then
                JOBS_ARG="-j$2"
                shift 2
            else
                JOBS_ARG="-j"
                shift
            fi
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        -s|--system-install)
            SYSTEM_INSTALL=true
            shift
            ;;
        *)
            echo "[build.sh, ERROR] Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done


# Check that DATA_PRODUCTS_CMAKE_PREFIX_PATH is set
if [[ -z $DATA_PRODUCTS_CMAKE_PREFIX_PATH ]]; then
    echo "[build.sh, ERROR] DATA_PRODUCTS_CMAKE_PREFIX_PATH is not set. Please source $SCRIPT_DIR/setenv.sh first."
   exit 1
fi

# Check that UNPACKERS_CMAKE_PREFIX_PATH is set
if [[ -z $UNPACKERS_CMAKE_PREFIX_PATH ]]; then
    echo "[build.sh, ERROR] UNPACKERS_CMAKE_PREFIX_PATH is not set. Please source $SCRIPT_DIR/setenv.sh first."
    exit 1
fi

# Check that CMAKE_PREFIX_PATH has correct paths
case ":$CMAKE_PREFIX_PATH:" in
  *":$DATA_PRODUCTS_CMAKE_PREFIX_PATH:"*)
    echo "DATA_PRODUCTS_CMAKE_PREFIX_PATH is set correctly in CMAKE_PREFIX_PATH"
    ;;
  *)
    echo "DATA_PRODUCTS_CMAKE_PREFIX_PATH needs to be added to CMAKE_PREFIX_PATH. Please source $SCRIPT_DIR/setenv.sh first."
    exit 1
    ;;
esac

case ":$CMAKE_PREFIX_PATH:" in
  *":$UNPACKERS_CMAKE_PREFIX_PATH:"*)
    echo "UNPACKERS_CMAKE_PREFIX_PATH is set correctly in CMAKE_PREFIX_PATH"
    ;;
  *)
    echo "UNPACKERS_CMAKE_PREFIX_PATH needs to be added to CMAKE_PREFIX_PATH. Please source $SCRIPT_DIR/setenv.sh first."
    exit 1
    ;;
esac

# Optionally clean build
if [ "$OVERWRITE" = true ]; then
    echo "[build.sh] Cleaning previous build with: $CLEANUP_SCRIPT"
    "$CLEANUP_SCRIPT"
fi

# Create and enter build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR" || exit 1

# Define install prefix (project root)
if [ "$SYSTEM_INSTALL" = true ]; then
    INSTALL_PREFIX="/usr/local"
else
    INSTALL_PREFIX="$BASE_DIR/install"
fi

echo "[build.sh] Running cmake with -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"
cmake -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" "$BASE_DIR"

echo "[build.sh] Building with make $JOBS_ARG"
make $JOBS_ARG

echo "[build.sh] Installing locally to $INSTALL_PREFIX"
make install

echo "[build.sh] Build complete."