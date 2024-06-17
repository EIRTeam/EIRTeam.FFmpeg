#!/bin/bash

# Exit on error
set -e

# User defined variables
TARGET=${1:-"all"}
PLATFORM=${2:-"linux"}
SCONS_VERSION=${3:-"4.4.0"}
FFMPEG_URL=${4:-"https://github.com/EIRTeam/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-linux64-lgpl-godot.tar.xz"}
FFMPEG_RELATIVE_PATH=${5:-"ffmpeg-master-latest-linux64-lgpl-godot"}
SCONS_FLAGS=${6:-"debug_symbols=no"}
SETUP=${7:-"true"}

# Fixed variables
SCONS_CACHE_DIR="scons-cache"
SCONS_CACHE_LIMIT="7168"
BUILD_DIR="gdextension_build"

if [ "${SETUP}" == "true" ]; then

    echo "Setting up to build for ${TARGET} with SCons ${SCONS_VERSION}"

    echo "Setting up FFmpeg. Source: ${FFMPEG_URL}"
    # Download FFmpeg
    wget -q --show-progress -O ffmpeg.tar.xz ${FFMPEG_URL}
    # Extract FFmpeg
    tar -xf ffmpeg.tar.xz
    echo "FFmpeg downloaded and extracted."

    # Ensure submodules are up to date
    git submodule update --init --recursive

    echo "Setting up SCons"
    # Setup virtual environment
    python -m venv venv
    source venv/bin/activate
    # Upgrade pip
    pip install --upgrade pip
    # Install SCons
    pip install scons==${SCONS_VERSION}
    echo "SCons $(scons --version) installed."
fi

if [ "${TARGET}" == "all" ]; then
    for target in "editor" "template_release" "template_debug"; do
        ${0} ${target} ${PLATFORM} ${SCONS_VERSION} ${FFMPEG_URL} ${FFMPEG_RELATIVE_PATH} ${SCONS_FLAGS} "false"
    done
    exit 0
fi


echo "Building ${TARGET}..."
# Setup environment variables
export SCONS_FLAGS="${SCONS_FLAGS}"
export SCONS_CACHE="${SCONS_CACHE_DIR}"
export SCONS_CACHE_LIMIT="${SCONS_CACHE_LIMIT}"
export FFMPEG_PATH="${PWD}/${FFMPEG_RELATIVE_PATH}"

# Enter build directory
pushd ${BUILD_DIR}
# Build
scons platform=linux target=${TARGET} ffmpeg_path=${FFMPEG_PATH} ${SCONS_FLAGS}
# Show build results
ls -R build/addons/ffmpeg

echo "Build completed. Cleaning up..."

# Exit build directory
popd
# Exit virtual environment
deactivate
# Remove the ffmpeg tarball
rm ffmpeg.tar.xz
