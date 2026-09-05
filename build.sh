#!/bin/bash

# Image to use
IMAGE="zmkfirmware/zmk-build-arm:stable"

# Volume for caching west modules to speed up builds
CACHE_VOLUME="zmk-build-cache"

# Create cache volume if it doesn't exist
docker volume create $CACHE_VOLUME > /dev/null

# Function to build a specific target
build_target() {
    local shield=$1
    local board=$2
    local artifact_name=$3
    local extra_args=$4

    echo "========================================================"
    echo "Building $artifact_name"
    echo "Shield: $shield"
    echo "Board:  $board"
    echo "Args:   $extra_args"
    echo "========================================================"

    docker run --rm \
        -v $(pwd):/workspace/host_repo \
        -v $CACHE_VOLUME:/workspace/build_env \
        -w /workspace/build_env \
        $IMAGE \
        /bin/bash -c "
            set -e
            
            # Prepare workspace directory in volume
            mkdir -p /workspace/build_env/zmk-config
            
            # Copy source files from host to volume
            # We use cp because rsync is not available in the image
            # We only copy what's needed to avoid overwriting the west workspace structure if it exists
            cp -r /workspace/host_repo/config /workspace/build_env/zmk-config/
            cp -r /workspace/host_repo/boards /workspace/build_env/zmk-config/
            cp /workspace/host_repo/build.yaml /workspace/build_env/zmk-config/
            
            cd /workspace/build_env/zmk-config
            
            # Setup workspace if not exists
            if [ ! -d ".west" ]; then
                echo 'Initializing West workspace...'
                # Initialize west using config/west.yml as manifest
                # This makes /workspace/build_env/zmk-config the workspace root
                west init -l config
                west update
            else
                # Update modules to ensure we have latest dependencies
                # echo 'Updating West modules...'
                # west update
                :
            fi
            
            # Export Zephyr CMake package (needed for every fresh container)
            west zephyr-export
            
            # Build
            echo 'Starting build...'
            west build -s zmk/app -p -b $board -- -DSHIELD=$shield -DZMK_CONFIG="/workspace/build_env/zmk-config/config" -DBOARD_ROOT="/workspace/build_env/zmk-config" $extra_args
            
            # Copy artifacts back to host
            mkdir -p /workspace/host_repo/build_artifacts
            cp build/zephyr/zmk.uf2 /workspace/host_repo/build_artifacts/$artifact_name.uf2
            echo 'Build complete: build_artifacts/$artifact_name.uf2'
        "
}

# Main execution
if [ "$1" == "all" ]; then
    # Build everything defined in build.yaml
    build_target "kb_1_left" "xiao_ble//zmk" "kb_1_left_central" "-DCONFIG_ZMK_SPLIT_ROLE_CENTRAL=y -DSNIPPET='studio-rpc-usb-uart split-central input-trackball input-listener'"
    build_target "kb_1_right" "xiao_ble//zmk" "kb_1_right_peripheral" "-DSNIPPET=studio-rpc-usb-uart"
    build_target "kb_1_left" "xiao_ble//zmk" "kb_1_left_peripheral" "-DSNIPPET=studio-rpc-usb-uart"
    build_target "kb_1_right" "xiao_ble//zmk" "kb_1_right_central" "-DCONFIG_ZMK_SPLIT_ROLE_CENTRAL=y -DSNIPPET='studio-rpc-usb-uart split-central input-trackball input-listener'"
    build_target "settings_reset" "xiao_ble//zmk" "settings_reset" ""
elif [ "$1" == "left" ]; then
    build_target "kb_1_left" "xiao_ble//zmk" "kb_1_left_central" "-DCONFIG_ZMK_SPLIT_ROLE_CENTRAL=y -DSNIPPET='studio-rpc-usb-uart split-central input-trackball input-listener'"
elif [ "$1" == "right" ]; then
    build_target "kb_1_right" "xiao_ble//zmk" "kb_1_right_peripheral" "-DSNIPPET=studio-rpc-usb-uart"
elif [ "$1" == "reset" ]; then
    build_target "settings_reset" "xiao_ble//zmk" "settings_reset" ""
elif [ -z "$1" ]; then
    echo "No arguments provided. Building default set (left_peripheral, right_central, settings_reset)..."
    build_target "kb_1_left" "xiao_ble//zmk" "kb_1_left_peripheral" "-DSNIPPET=studio-rpc-usb-uart"
    build_target "kb_1_right" "xiao_ble//zmk" "kb_1_right_central" "-DCONFIG_ZMK_SPLIT_ROLE_CENTRAL=y -DSNIPPET='studio-rpc-usb-uart split-central input-trackball input-listener'"
    build_target "settings_reset" "xiao_ble//zmk" "settings_reset" ""
else
    echo "Usage: $0 {all|left|right|reset}"
    echo "  all:   Build all main targets"
    echo "  left:  Build left central"
    echo "  right: Build right peripheral"
    echo "  reset: Build settings_reset firmware"
    exit 1
fi
